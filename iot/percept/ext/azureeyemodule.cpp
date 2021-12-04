#include <Python.h>
#include <assert.h>
#include <numpy/arrayobject.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <map>

#include "usbcamera.hh"

#define __STDC_WANT_LIB_EXT1__ 1

#include <VPUBlockTypes.h>
#include <XLink.h>
#include <fcntl.h>
#include <mxIfMemoryWriteBlock.h>

#include <CameraStub.hpp>
#include <vector>

#include "XLinkLog.h"
#include "XLinkMacros.h"
#include "XLinkPrivateFields.h"
#include "XLinkStringUtils.h"
#include "mxIf.h"
#include "mxIfCameraBlock.h"
#include "mxIfMemoryHandle.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
}

#define mvLogDefaultLevelSet(MVLOG_ERROR)
#define mvLogLevelSet(MVLOG_ERROR)

FILE *tmp;
char *mvcmdFile;
char *in_file;
void *camera_pointer = nullptr;
std::shared_ptr<mxIf::CameraBlock> m_cam;
std::shared_ptr<mxIf::InferBlock> infer;
char *out_file;
char *blob_file;
char *fname;
bool isRunning = false;
bool isOpen = false;
unsigned int inferenceSize;
uint32_t inferenceType;
bool camOn = false;
int width;
int height;

class CaptureMetadata
{
public:
  std::string deviceName;
  struct buffer *buffers = NULL;
  unsigned char *dst = nullptr;
  unsigned int width = 0;
  unsigned int height = 0;
  unsigned int samples = 10;
  unsigned int n_buffers = 0;
  int fd = -1;
  CaptureMetadata(char *device, size_t size, int fd, unsigned int n_buffers,
                  unsigned int width, unsigned int height)
  {
    for (int i = 0; i < size; i++)
    {
      deviceName = deviceName + device[i];
    }
    this->n_buffers = n_buffers;
    this->fd = fd;
    this->deviceName = deviceName;
    this->width = width;
    this->height = height;
    this->dst = (unsigned char *)malloc(width * height * 3 * sizeof(char));
  }

  void printBufferAddress()
  {
    for (int i = 0; i < this->n_buffers; i++)
    {
      printf("Address [%i]: %p\n", i, this->buffers[i].start);
    }
  }

  void clear() { free(this->dst); }
};

std::vector<CaptureMetadata> devices;

std::string randomString(int len)
{
  std::string tmp;
  const char alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyz";

  for (int i = 0; i < len; ++i)
  {
    tmp += alphabet[rand() % (sizeof(alphabet) - 1)];
  }

  return tmp;
}

int convertVideo(const char *in_filename, const char *out_filename)
{
  av_log_set_level(AV_LOG_QUIET);
  // Input AVFormatContext and Output AVFormatContext
  AVFormatContext *input_format_context = avformat_alloc_context();
  AVPacket pkt;

  int ret, i;
  int frame_index = 0;
  av_register_all();
  // Input
  if ((ret = avformat_open_input(&input_format_context, in_filename, NULL,
                                 NULL)) < 0)
  {
    printf("Could not open input file.");
    return 1;
  }
  if ((ret = avformat_find_stream_info(input_format_context, 0)) < 0)
  {
    printf("Failed to retrieve input stream information");
    return 1;
  }
  AVFormatContext *output_format_context = avformat_alloc_context();
  AVPacket packet;
  int stream_index = 0;
  int *streams_list = NULL;
  int number_of_streams = 0;
  int fragmented_mp4_options = 0;
  avformat_alloc_output_context2(&output_format_context, NULL, NULL,
                                 out_filename);
  if (!output_format_context)
  {
    fprintf(stderr, "Could not create output context\n");
    ret = AVERROR_UNKNOWN;
    return 1;
  }
  AVOutputFormat *fmt = av_guess_format(0, out_filename, 0);
  output_format_context->oformat = fmt;
  number_of_streams = input_format_context->nb_streams;
  streams_list =
      (int *)av_mallocz_array(number_of_streams, sizeof(*streams_list));

  if (!streams_list)
  {
    ret = AVERROR(ENOMEM);
    return 1;
  }
  for (i = 0; i < input_format_context->nb_streams; i++)
  {
    AVStream *out_stream;
    AVStream *in_stream = input_format_context->streams[i];
    AVCodecParameters *in_codecpar = in_stream->codecpar;
    if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
    {
      streams_list[i] = -1;
      continue;
    }
    streams_list[i] = stream_index++;

    out_stream = avformat_new_stream(output_format_context, NULL);
    if (!out_stream)
    {
      fprintf(stderr, "Failed allocating output stream\n");
      ret = AVERROR_UNKNOWN;
      return 1;
    }
    ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
    if (ret < 0)
    {
      fprintf(stderr, "Failed to copy codec parameters\n");
      return 1;
    }
  }
  av_dump_format(output_format_context, 0, out_filename, 1);
  // unless it's a no file (we'll talk later about that) write to the disk
  // (FLAG_WRITE) but basically it's a way to save the file to a buffer so you
  // can store it wherever you want.
  if (!(output_format_context->oformat->flags & AVFMT_NOFILE))
  {
    ret = avio_open(&output_format_context->pb, out_filename, AVIO_FLAG_WRITE);
    if (ret < 0)
    {
      fprintf(stderr, "Could not open output file '%s'", out_filename);
      return 1;
    }
  }
  AVDictionary *opts = NULL;
  ret = avformat_write_header(output_format_context, &opts);
  if (ret < 0)
  {
    fprintf(stderr, "Error occurred when opening output file\n");
    return 1;
  }
  int n = 0;

  while (1)
  {
    AVStream *in_stream, *out_stream;
    ret = av_read_frame(input_format_context, &packet);
    if (ret < 0)
      break;
    in_stream = input_format_context->streams[packet.stream_index];
    if (packet.stream_index >= number_of_streams ||
        streams_list[packet.stream_index] < 0)
    {
      av_packet_unref(&packet);
      continue;
    }
    packet.stream_index = streams_list[packet.stream_index];

    out_stream = output_format_context->streams[packet.stream_index];
    // out_stream->time_base = {1, 30};
    /* copy packet */

    // AVRational time_base;
    // out_stream->time_base = time_base;
    // out_stream->time_base.den = 34;
    // out_stream->time_base.num = 1;

    out_stream->codec->time_base.num = 1;
    out_stream->codec->time_base.den = 30;

    // packet.pts = av_rescale_q_rnd(
    //     packet.pts, in_stream->time_base, out_stream->time_base,
    //     static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    // packet.dts = av_rescale_q_rnd(
    //     packet.dts, in_stream->time_base, out_stream->time_base,
    //     static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    // packet.duration = av_rescale_q(packet.duration, in_stream->time_base,
    //                                out_stream->time_base);

    packet.pts = n * 3000;
    packet.dts = n * 3000;
    packet.duration = 3000;

    packet.pos = -1;

    ret = av_interleaved_write_frame(output_format_context, &packet);
    if (ret < 0)
    {
      fprintf(stderr, "Error muxing packet\n");
      break;
    }
    av_packet_unref(&packet);
    n++;
  }
  // https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13
  av_write_trailer(output_format_context);
  avformat_close_input(&input_format_context);
  /* close output */
  if (output_format_context &&
      !(output_format_context->oformat->flags & AVFMT_NOFILE))
    avio_closep(&output_format_context->pb);
  avformat_free_context(output_format_context);
  av_freep(&streams_list);
  if (ret < 0 && ret != AVERROR_EOF)
  {
    fprintf(stderr, "Error occurred\n");
    return 1;
  }
  return 0;
}

void *record(void *par)
{
  isRunning = true;

  if (camOn == false)
  {
    auto cam_mode = mxIf::CameraBlock::CamMode::CamMode_720p;
    m_cam.reset(new mxIf::CameraBlock(cam_mode));
    m_cam->Start();
    camOn = true;
  }

  std::string f = randomString(16);
  fname = (char *)malloc(16 + 5 + 5 + 1);
  strcpy(fname, "/tmp/");
  strcat(fname, f.c_str());
  strcat(fname, ".h264");

  // fname = tmpnam(NULL);
  tmp = fopen(fname, "wb");

  while (true)
  {
    if (!isRunning)
    {
      break;
    }
    mxIf::MemoryHandle h264_hndl =
        m_cam->GetNextOutput(mxIf::CameraBlock::Outputs::H264);
    uint8_t *pBuf = (uint8_t *)malloc(h264_hndl.bufSize);
    assert(nullptr != pBuf);
    h264_hndl.TransferTo(pBuf);

    fwrite(pBuf, sizeof(uint8_t), h264_hndl.bufSize, tmp);

    free(pBuf);
    m_cam->ReleaseOutput(mxIf::CameraBlock::Outputs::H264, h264_hndl);
  }
  fclose(tmp);
  int res = convertVideo(fname, out_file);
  if (remove(fname) != 0)
  {
    printf("Deleting temporary file failed\n");
  }
  if (res != 0)
  {
    printf("Converting video failed\n");
    exit(1);
  }
  free(out_file);
  free(fname);
  return 0;
}

// This thread must run during inference,
// otherwise inference hangs after a while
void *h264Thread(void *par)
{
  // mxIf::CameraBlock *camera_block = (mxIf::CameraBlock *)par;
  while (isOpen)
  {
    mxIf::MemoryHandle h264_hndl =
        m_cam->GetNextOutput(mxIf::CameraBlock::Outputs::H264);
    uint8_t *pBuf = (uint8_t *)malloc(h264_hndl.bufSize);
    assert(nullptr != pBuf);
    h264_hndl.TransferTo(pBuf);
    free(pBuf);
  }
  return 0;
}

// void *pullThread(void *par)
// {
//   // char *out = (char *)par;
//   isRunning = true;
//   // tmp = tmpfile();
//   if (camOn == false)
//   {
//     auto cam_mode = mxIf::CameraBlock::CamMode::CamMode_720p;
//     m_cam.reset(new mxIf::CameraBlock(cam_mode));
//     m_cam->Start();
//     camOn = true;
//     pthread_t threadH264;
//     int ret = pthread_create(&threadH264, NULL, h264Thread, NULL);
//   }
//   FILE *tmp;

//   while (isRunning)
//   {
//     mxIf::MemoryHandle bgr_hndl =
//         m_cam->GetNextOutput(mxIf::CameraBlock::Outputs::BGR);
//     std::map<std::string, mxIf::InferIn> inferIn;
//     auto info = infer->get_info();
//     currentWidth = bgr_hndl.width;
//     currentHeight = bgr_hndl.height;
//     currentImage = (uint8_t *)malloc(bgr_hndl.bufSize);
//     currentSize = bgr_hndl.bufSize;
//     assert(nullptr != currentImage);
//     bgr_hndl.TransferTo(currentImage);
//     mxIf::InferIn inferReq = {
//         bgr_hndl, mxIf::PREPROCESS_ROI{0, 0, bgr_hndl.width,
//         bgr_hndl.height}};
//     for (const auto &input_name : info.first)
//     {
//       inferIn.insert(std::make_pair(input_name, inferReq));
//     }

//     infer->Enqueue(inferIn);

//     std::map<std::string, mxIf::MemoryHandle> nn_out =
//     infer->GetNextOutput();

//     const auto &output_shapes = infer->get_output_shapes();
//     for (const auto &output_shape : output_shapes)
//     {
//       inferenceType = static_cast<std::uint32_t>(output_shape.data_type);
//     }
//     // printf("Output datatype: %d\n", type);
//     auto output_names = infer->get_info().second;
//     for (const std::string &output_name : output_names)
//     {
//       auto nn_hndl = nn_out[output_name];
//       inferenceOutput = (uint8_t *)malloc(nn_hndl.bufSize);
//       assert(nullptr != inferenceOutput);
//       nn_hndl.TransferTo(inferenceOutput);
//       inferenceSize = nn_hndl.bufSize;
//     }

//     while (hold)
//     {
//     }
//     free(currentImage);
//     // Not freeing this will cause a memory leak, however, otherwise the
//     // inference output is garbage
//     free(inferenceOutput);

//     m_cam->ReleaseOutput(mxIf::CameraBlock::Outputs::BGR, bgr_hndl);
//   }
//   if (tmp)
//   {
//     fclose(tmp);
//   }
//   free(blob_file);
//   // if (store)
//   // {
//   //   int res = convertVideo(in_file, out_file);
//   //   if (res != 0)
//   //   {
//   //     printf("Converting video failed\n");
//   //     exit(1);
//   //   }
//   //   free(out_file);
//   // }
//   return 0;
// }

static PyObject *method_startrecording(PyObject *self, PyObject *args)
{
  char *out;
  if (!PyArg_ParseTuple(args, "s", &out))
  {
    return NULL;
  }
  out_file = (char *)malloc(strlen(out) + 1);
  strcpy(out_file, out);
  pthread_t tid;
  pthread_create(&tid, NULL, record, NULL);
  return Py_BuildValue("");
}

static PyObject *method_stoprecording(PyObject *self, PyObject *args)
{
  // if (tmp == NULL)
  // {
  //   printf("Temporary h264 packets file is not available");
  //   PyErr_SetString(PyExc_TypeError, "Temporary h264 packets file is not
  //   available"); return NULL;
  // }
  isRunning = false;
  return Py_BuildValue("");
}

static PyObject *method_prepareeye(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, "s", &mvcmdFile))
  {
    return NULL;
  }

  mvLogLevelSet(MVLOG_ERROR);
  mvLogDefaultLevelSet(MVLOG_ERROR);
  mxIf::Boot(mvcmdFile);

  isOpen = true;
  // auto cam_mode = mxIf::CameraBlock::CamMode::CamMode_720p;
  // m_cam.reset(new mxIf::CameraBlock(cam_mode));
  // m_cam->Start();

  return Py_BuildValue("");
}

static PyObject *method_closedevice(PyObject *self, PyObject *args)
{
  camOn = false;
  sleep(1);
  mxIf::Reset();
  return Py_BuildValue("");
}

static PyObject *method_stopinference(PyObject *self, PyObject *args)
{
  captureStop();
  for (CaptureMetadata device : devices)
  {
    if (device.deviceName != "/camera1")
    {
      deviceUninit(device.n_buffers, device.buffers);
      deviceClose(device.fd);
      // device.clear();
    }
  }
  isRunning = false;
  isOpen = false;
  return Py_BuildValue("");
}

static PyObject *method_getinference(PyObject *self, PyObject *args)
{
  char *model_path;
  int returnImage;
  int currentWidth;
  int currentHeight;
  uint8_t *currentImage;
  unsigned int currentSize;
  uint8_t *inferenceOutput;

  if (!PyArg_ParseTuple(args, "i", &returnImage))
  {
    return NULL;
  }

  PyObject *resultList = PyList_New(devices.size());

  int index = 0;
  if (camOn == false)
  {
    auto cam_mode = mxIf::CameraBlock::CamMode::CamMode_720p;
    m_cam.reset(new mxIf::CameraBlock(cam_mode));
    m_cam->Start();
    camOn = true;
    pthread_t threadH264;
    int ret = pthread_create(&threadH264, NULL, h264Thread, NULL);
  }

  for (CaptureMetadata device : devices)
  {
    std::map<std::string, mxIf::InferIn> inferIn;
    auto info = infer->get_info();

    mxIf::MemoryHandle bgr_hndl =
        m_cam->GetNextOutput(mxIf::CameraBlock::Outputs::BGR);

    if (device.deviceName == "/camera1")
    {
      currentWidth = bgr_hndl.width;
      currentHeight = bgr_hndl.height;
      currentImage = (uint8_t *)malloc(bgr_hndl.bufSize);
      currentSize = bgr_hndl.bufSize;
      assert(nullptr != currentImage);
      bgr_hndl.TransferTo(currentImage);

      mxIf::InferIn inferReq = {
          bgr_hndl,
          mxIf::PREPROCESS_ROI{0, 0, bgr_hndl.width, bgr_hndl.height}};
      for (const auto &input_name : info.first)
      {
        inferIn.insert(std::make_pair(input_name, inferReq));
      }
      infer->Enqueue(inferIn);
    }
    else
    {
      // printf("%s\n", device.deviceName.c_str());
      // device.printBufferAddress();

      for (int i = 0; i < device.samples; i++)
      {
        usbCapture(device.fd, device.dst, device.n_buffers, &device.buffers,
                   device.width, device.height);
      }

      // mxIf::MemoryHandle out_handle;
      // std::shared_ptr<mxIf::MemoryWriteBlock> m_writer;
      // m_writer.reset(new mxIf::MemoryWriteBlock);
      // mxIf::MemoryHandle tensor_hndl;
      // tensor_hndl.pBuf = (uint8_t *)device.dst;
      // tensor_hndl.bufSize = device.width * device.height * 3 * sizeof(char);
      // tensor_hndl.type = mxIf::MemoryHandle::Types::LocalMem;
      // currentWidth = tensor_hndl.width;
      // currentHeight = tensor_hndl.height;
      // currentImage = (uint8_t *)malloc(tensor_hndl.bufSize);
      // currentSize = tensor_hndl.bufSize;
      // assert(nullptr != currentImage);
      // tensor_hndl.TransferTo(currentImage);
      // out_handle = m_writer->Write(tensor_hndl);

      // mxIf::InferIn inferRe = {out_handle, mxIf::PREPROCESS_ROI{0, 0,
      // tensor_hndl.width, tensor_hndl.height}};

      // for (const auto &input_name : info.first)
      // {
      //   inferIn.insert(std::make_pair(input_name, inferRe));
      // }

      // free(bgr_hndl.pBuf);
      void *tmp = bgr_hndl.pBuf;
      bgr_hndl.pBuf = (uint8_t *)device.dst;
      bgr_hndl.bufSize = device.width * device.height * 3 * sizeof(char);
      bgr_hndl.width = device.width;
      bgr_hndl.height = device.height;
      currentWidth = bgr_hndl.width;
      currentHeight = bgr_hndl.height;
      // uint8_t *img = (uint8_t *)malloc(bgr_hndl.bufSize);
      // if (!img)
      // {
      //   PyErr_SetString(PyExc_TypeError, "Memory allocation for image
      //   failed"); return NULL;
      // }
      // currentImage = img;
      currentSize = bgr_hndl.bufSize;
      // assert(nullptr != currentImage);
      // bgr_hndl.TransferTo(currentImage);
      currentImage = (uint8_t *)device.dst;

      mxIf::InferIn inferReq = {
          bgr_hndl,
          mxIf::PREPROCESS_ROI{0, 0, bgr_hndl.width, bgr_hndl.height}};

      for (const auto &input_name : info.first)
      {
        inferIn.insert(std::make_pair(input_name, inferReq));
      }

      infer->Enqueue(inferIn);
      bgr_hndl.pBuf = tmp;

      // tensor_writer.Release(raw_tensor_rmt_hndl);
    }

    std::map<std::string, mxIf::MemoryHandle> nn_out = infer->GetNextOutput();

    const auto &output_shapes = infer->get_output_shapes();
    std::vector<int> inferenceTypes;
    for (const auto &output_shape : output_shapes)
    {
      inferenceType = static_cast<std::uint32_t>(output_shape.data_type);
      inferenceTypes.push_back(inferenceType);
    }
    // printf("Output datatype: %d\n", type);
    auto output_names = infer->get_info().second;
    unsigned int infIndex = 0;
    unsigned int resultsCount = output_names.size();
    PyObject *inferenceResults = PyList_New(resultsCount);
    for (const std::string &output_name : output_names)
    {
      auto nn_hndl = nn_out[output_name];

      // printf("NN: name=%s size=%d; seqNo=%ld; ts=%ld\n", output_name.data(),
      //        nn_hndl.bufSize, nn_hndl.seqNo, nn_hndl.ts);

      inferenceOutput = (uint8_t *)malloc(nn_hndl.bufSize);
      assert(nullptr != inferenceOutput);
      nn_hndl.TransferTo(inferenceOutput);
      inferenceSize = nn_hndl.bufSize;
      int inType = inferenceTypes[infIndex];
      PyObject *in = Py_BuildValue("y#", inferenceOutput, inferenceSize);
      if (inType == 0)
      {
        PyObject *inf = PyArray_FromBuffer(in, PyArray_DescrFromType(NPY_FLOAT16), inferenceSize / 2, 0);
        PyList_SetItem(inferenceResults, infIndex, inf);
      }
      else if (inType == 1)
      {
        PyObject *inf = PyArray_FromBuffer(in, PyArray_DescrFromType(NPY_UINT8), inferenceSize, 0);
        PyList_SetItem(inferenceResults, infIndex, inf);
      }
      else if (inType == 2)
      {
        PyObject *inf = PyArray_FromBuffer(in, PyArray_DescrFromType(NPY_INT32), inferenceSize / 4, 0);
        PyList_SetItem(inferenceResults, infIndex, inf);
      }
      else if (inType == 3)
      {
        PyObject *inf = PyArray_FromBuffer(in, PyArray_DescrFromType(NPY_FLOAT32), inferenceSize / 4, 0);
        PyList_SetItem(inferenceResults, infIndex, inf);
      }
      else if (inType == 4)
      {
        PyObject *inf = PyArray_FromBuffer(in, PyArray_DescrFromType(NPY_INT8), inferenceSize, 0);
        PyList_SetItem(inferenceResults, infIndex, inf);
      }
      else
      {
      }
      Py_DECREF(in);
      free(inferenceOutput);
      infIndex++;
    }
    if (returnImage == 1)
    {
      npy_intp dims[3] = {3, currentHeight, currentWidth};
      PyObject *res = PyArray_SimpleNew(3, dims, NPY_UINT8);
      memcpy(PyArray_DATA(res), currentImage, currentSize);
      if (device.deviceName == "/camera1")
      {
        free(currentImage);
      }
      PyObject *result = Py_BuildValue("(OO)", inferenceResults, res);
      Py_DECREF(res);
      PyList_SetItem(resultList, index, result);
      // Py_DECREF of "result" necessary?
    }
    else
    {
      if (device.deviceName == "/camera1")
      {
        free(currentImage);
      }
      PyObject *result = Py_BuildValue("O", inferenceResults);
      PyList_SetItem(resultList, index, result);
      // Py_DECREF of "result" necessary?
    }

    m_cam->ReleaseOutput(mxIf::CameraBlock::Outputs::BGR, bgr_hndl);

    index++;
  }

  return resultList;

  // if (returnImage == 1)
  // {
  //   npy_intp dims[3] = {3, currentHeight, currentWidth};
  //   PyObject *res = PyArray_SimpleNew(3, dims, NPY_UINT8);
  //   memcpy(PyArray_DATA(res), currentImage, currentSize);
  //   free(inferenceOutput);
  //   free(currentImage);
  //   PyObject *result = Py_BuildValue("(y#iO)", inferenceOutput,
  //   inferenceSize, inferenceType, res); Py_DECREF(res); return result;
  //   // return Py_BuildValue("(y#iO)", inferenceOutput, inferenceSize,
  //   inferenceType, res);
  // }
  // else
  // {
  //   free(inferenceOutput);
  //   free(currentImage);
  //   return Py_BuildValue("(y#i)", inferenceOutput, inferenceSize,
  //                        inferenceType);
  // }
}

static PyObject *method_getframe(PyObject *self, PyObject *args)
{
  if (camOn == false)
  {
    auto cam_mode = mxIf::CameraBlock::CamMode::CamMode_720p;
    m_cam.reset(new mxIf::CameraBlock(cam_mode));
    m_cam->Start();
    camOn = true;
    pthread_t threadH264;
    int ret = pthread_create(&threadH264, NULL, h264Thread, NULL);
  }
  int width;
  int height;
  uint8_t *pBuf = nullptr;
  uint32_t size;

  mxIf::MemoryHandle bgr_hndl =
      m_cam->GetNextOutput(mxIf::CameraBlock::Outputs::BGR);
  width = bgr_hndl.width;
  height = bgr_hndl.height;

  pBuf = (uint8_t *)malloc(bgr_hndl.bufSize);
  assert(nullptr != pBuf);
  bgr_hndl.TransferTo(pBuf);
  size = bgr_hndl.bufSize;

  m_cam->ReleaseOutput(mxIf::CameraBlock::Outputs::BGR, bgr_hndl);

  npy_intp dims[3] = {3, height, width};
  PyObject *res = PyArray_SimpleNew(3, dims, NPY_UINT8);
  memcpy(PyArray_DATA(res), pBuf, size);
  free(pBuf);
  return res;

  // return Py_BuildValue("(y#ii)", pBuf, size, width, height);
}

static PyObject *method_startinference(PyObject *self, PyObject *args)
{
  char *out;
  PyObject *input_src = NULL;
  // if (!PyArg_ParseTuple(args, "s", &out))
  // {
  //   return NULL;
  // }

  // static char *kwlist[] = {"out", "input_src", NULL};

  if (!PyArg_ParseTuple(args, "sO", &out, &input_src))
  {
    return NULL;
  }

  if (input_src != Py_None)
  {
    int listItems = PyList_Size(input_src);
    for (int i = 0; i < listItems; i++)
    {
      PyObject *pItem = PyList_GetItem(input_src, i);
      // if (!PyObject_TypeCheck(pItem, &PyBaseString_Type))
      // {
      //   PyErr_SetString(PyExc_TypeError, "List items must be strings");
      //   return NULL;
      // }
      PyObject *encodedString =
          PyUnicode_AsEncodedString(pItem, "UTF-8", "strict");
      if (encodedString)
      {
        char *videoSrc = PyBytes_AsString(encodedString);
        if (strcmp(videoSrc, "/camera1") == 0)
        {
          CaptureMetadata cap(videoSrc, strlen(videoSrc), -1, 0, 0, 0);
          devices.push_back(cap);
        }
        else
        {
          unsigned int width = 1280;
          unsigned int height = 720;
          unsigned int n_buffers = 0;
          struct buffer *buffers = NULL;
          int fd = deviceOpen(videoSrc);
          struct dimensions newDim = deviceInit(videoSrc, fd, width, height);
          n_buffers = mmapInit(videoSrc, n_buffers, &buffers, fd);
          width = newDim.width;
          height = newDim.height;
          captureStart(fd, n_buffers);
          CaptureMetadata cap(videoSrc, strlen(videoSrc), fd, n_buffers, width,
                              height);
          cap.buffers = buffers;
          devices.push_back(cap);
          // cap.printBufferAddress();
        }
      }
    }
  }

  blob_file = (char *)malloc(strlen(out) + 1);
  strcpy(blob_file, out);
  // mxIf::InferBlock infer_block = mxIf::CreateInferBlock(blob_file);
  infer.reset(new mxIf::InferBlock(blob_file));

  // pthread_t tid;
  // pthread_create(&tid, NULL, pullThread, NULL);
  return Py_BuildValue("");
}

static PyMethodDef EyeMethods[] = {
    {"start_inference", method_startinference, METH_VARARGS,
     "Start model inference"},
    {"stop_inference", method_stopinference, METH_VARARGS,
     "Stop model inference"},
    {"get_inference", (PyCFunction)method_getinference, METH_VARARGS,
     "Gets the current model inference output"},
    {"start_recording", method_startrecording, METH_VARARGS,
     "Start Azure Eye video recording"},
    {"get_frame", method_getframe, METH_VARARGS,
     "Returns an image taken by the AzureEye camera"},
    {"stop_recording", method_stoprecording, METH_VARARGS,
     "Stop Azure Eye video recording"},
    {"prepare_eye", method_prepareeye, METH_VARARGS,
     "Prepares the Azure Eye device"},
    {"close_eye", method_closedevice, METH_VARARGS, "Closes the Eye device"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef _azureeyemodule = {
    PyModuleDef_HEAD_INIT, "_azureeye", "Azure Eye interface", -1, EyeMethods};

PyMODINIT_FUNC PyInit__azureeye(void)
{
  import_array();
  return PyModule_Create(&_azureeyemodule);
}