#include <Python.h>

#include <numpy/arrayobject.h>

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <string>
#include <map>

#define __STDC_WANT_LIB_EXT1__ 1

#include "mxIf.h"
#include "mxIfCameraBlock.h"
#include "mxIfMemoryHandle.h"
#include <fcntl.h>
#include <XLink.h>
#include "XLinkMacros.h"
#include "XLinkPrivateFields.h"
#include "XLinkLog.h"
#include "XLinkStringUtils.h"

#include <VPUBlockTypes.h>

#include <CameraStub.hpp>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
}

#define mvLogDefaultLevelSet(MVLOG_ERROR)
#define mvLogLevelSet(MVLOG_ERROR)

char *mvcmdFile;
char *in_file;
char *out_file;
char *blob_file;
char *fname;
bool isRunning = false;
bool store = false;
bool inference = false;
bool isConverting = false;
uint8_t *inferenceOutput;
unsigned int inferenceSize;
uint32_t inferenceType;

float bytesToFloat16(uint8_t *bytes)
{
  float f;
  uint8_t *f_ptr = (uint8_t *)&f;
  f_ptr[1] = bytes[1];
  f_ptr[0] = bytes[0];
  return f;
}

int bytesToInt(uint8_t *bytes)
{
  int result = 0;
  for (int n = sizeof(int); n >= 0; n--)
  {
    result = (result << 8) + bytes[n];
  }
  return result;
}

float bytesToFloat162(uint8_t *bytes)
{
  uint16_t value = bytes[1] | (bytes[0] << 8);
  union FP32
  {
    uint32_t u;
    float f;
  };

  const union FP32 magic = {(254UL - 15UL) << 23};
  const union FP32 was_inf_nan = {(127UL + 16UL) << 23};
  union FP32 out;

  out.u = (value & 0x7FFFU) << 13;
  out.f *= magic.f;
  if (out.f >= was_inf_nan.f)
  {
    out.u |= 255UL << 23;
  }
  out.u |= (value & 0x8000UL) << 16;

  return out.f;
}

float bytesToFloat(uint8_t *bytes, bool big_endian)
{
  float f;
  uint8_t *f_ptr = (uint8_t *)&f;
  if (big_endian)
  {
    f_ptr[3] = bytes[0];
    f_ptr[2] = bytes[1];
    f_ptr[1] = bytes[2];
    f_ptr[0] = bytes[3];
  }
  else
  {
    f_ptr[3] = bytes[3];
    f_ptr[2] = bytes[2];
    f_ptr[1] = bytes[1];
    f_ptr[0] = bytes[0];
  }
  return f;
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
  auto cam_mode = mxIf::CameraBlock::CamMode::CamMode_720p;
  mxIf::CameraBlock camera_block = mxIf::CreateCameraBlock(cam_mode);
  camera_block.Start();
  isRunning = true;
  FILE *tmp;
  fname = tmpnam(NULL);
  in_file = (char *)malloc(strlen(fname) + 1);
  strcpy(in_file, fname);
  tmp = fopen(fname, "wb");

  while (isRunning)
  {
    mxIf::MemoryHandle h264_hndl = camera_block.GetNextOutput(mxIf::CameraBlock::Outputs::H264);
    uint8_t *pBuf = (uint8_t *)malloc(h264_hndl.bufSize);
    assert(nullptr != pBuf);
    h264_hndl.TransferTo(pBuf);

    fwrite(pBuf, sizeof(uint8_t), h264_hndl.bufSize, tmp);

    free(pBuf);
  }
  fclose(tmp);
  isConverting = true;
  int res = convertVideo(in_file, out_file);
  free(in_file);
  if (remove(fname) != 0)
  {
    printf("Deleting temporary file failed\n");
  }
  if (res != 0)
  {
    printf("Converting video failed\n");
    exit(1);
  }
  isConverting = false;
}

void *pullThread(void *par)
{
  auto cam_mode = mxIf::CameraBlock::CamMode::CamMode_720p;
  mxIf::CameraBlock camera_block = mxIf::CreateCameraBlock(cam_mode);
  mxIf::InferBlock infer_block = mxIf::CreateInferBlock(blob_file);
  camera_block.Start();
  // char *out = (char *)par;
  isRunning = true;
  // tmp = tmpfile();
  FILE *tmp;
  if (store)
  {
    fname = tmpnam(NULL);
    // printf("filename: %s\n", fname);
    in_file = (char *)malloc(strlen(fname) + 1);
    strcpy(in_file, fname);
    tmp = fopen(fname, "wb");
  }
  // pthread_t threadNn;
  while (isRunning)
  {
    mxIf::MemoryHandle h264_hndl = camera_block.GetNextOutput(mxIf::CameraBlock::Outputs::H264);
    uint8_t *pBuf = (uint8_t *)malloc(h264_hndl.bufSize);
    assert(nullptr != pBuf);
    h264_hndl.TransferTo(pBuf);

    // printf("H264: size=%d; seqNo=%ld; ts=%ld\n", h264_hndl.bufSize, h264_hndl.seqNo, h264_hndl.ts);

    //int ret_wr = write(out, pBuf, h264_hndl.bufSize);
    if (store)
    {
      fwrite(pBuf, sizeof(uint8_t), h264_hndl.bufSize, tmp);
    }
    //if (ret_wr != h264_hndl.bufSize)
    //    printf("Failed to write chunk!\n");

    if (inference)
    {
      mxIf::MemoryHandle bgr_hndl = camera_block.GetNextOutput(mxIf::CameraBlock::Outputs::BGR);
      std::map<std::string, mxIf::InferIn> inferIn;
      auto info = infer_block.get_info();
      mxIf::InferIn inferReq = {bgr_hndl, mxIf::PREPROCESS_ROI{0, 0, bgr_hndl.width, bgr_hndl.height}};
      for (const auto &input_name : info.first)
      {
        inferIn.insert(std::make_pair(input_name, inferReq));
      }

      infer_block.Enqueue(inferIn);

      std::map<std::string, mxIf::MemoryHandle> nn_out = infer_block.GetNextOutput();

      const auto &output_shapes = infer_block.get_output_shapes();
      for (const auto &output_shape : output_shapes)
      {
        inferenceType = static_cast<std::uint32_t>(output_shape.data_type);
      }
      // printf("Output datatype: %d\n", type);
      auto output_names = infer_block.get_info().second;
      for (const std::string &output_name : output_names)
      {
        auto nn_hndl = nn_out[output_name];
        inferenceOutput = (uint8_t *)malloc(nn_hndl.bufSize);
        assert(nullptr != inferenceOutput);
        nn_hndl.TransferTo(inferenceOutput);
        inferenceSize = nn_hndl.bufSize;

        // printf("NN: name=%s size=%d; seqNo=%ld; ts=%ld\n",
        //        output_name.data(),
        //        nn_hndl.bufSize,
        //        nn_hndl.seqNo,
        //        nn_hndl.ts);
      }

      // While NN is running, skip BGRs
      // while (isRunning == true)
      // {
      //   // Skip
      //   mxIf::MemoryHandle bgr_hndl_2 = camera_block.GetNextOutput(mxIf::CameraBlock::Outputs::BGR);
      //   camera_block.ReleaseOutput(mxIf::CameraBlock::Outputs::BGR, bgr_hndl_2);
      // }
      // pthread_join(threadNn, NULL);
      camera_block.ReleaseOutput(mxIf::CameraBlock::Outputs::BGR, bgr_hndl);
    }

    free(pBuf);
  }
  if (tmp)
  {
    fclose(tmp);
  }
  free(blob_file);
  if (store)
  {
    int res = convertVideo(in_file, out_file);
    if (res != 0)
    {
      printf("Converting video failed\n");
      exit(1);
    }
  }
  pthread_exit(0);
}

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
  store = true;
  inference = false;
  pthread_create(&tid, NULL, record, NULL);
  return Py_BuildValue("");
}

static PyObject *method_stoprecording(PyObject *self, PyObject *args)
{
  // if (tmp == NULL)
  // {
  //   printf("Temporary h264 packets file is not available");
  //   PyErr_SetString(PyExc_TypeError, "Temporary h264 packets file is not available");
  //   return NULL;
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
  return Py_BuildValue("");
}

static PyObject *method_closedevice(PyObject *self, PyObject *args)
{
  sleep(2);
  mxIf::Reset();
  free(out_file);
  return Py_BuildValue("");
}

static PyObject *method_stopinference(PyObject *self, PyObject *args)
{
  isRunning = false;
  if (inferenceOutput)
  {
    free(inferenceOutput);
  }
  return Py_BuildValue("");
}

static PyObject *method_getinference(PyObject *self, PyObject *args)
{
  return Py_BuildValue("(y#i)", inferenceOutput, inferenceSize, inferenceType);
}

static PyObject *method_getframe(PyObject *self, PyObject *args)
{
  auto cam_mode = mxIf::CameraBlock::CamMode::CamMode_720p;
  mxIf::CameraBlock camera_block = mxIf::CreateCameraBlock(cam_mode);
  camera_block.Start();
  mxIf::MemoryHandle bgr_hndl = camera_block.GetNextOutput(mxIf::CameraBlock::Outputs::BGR);
  int width = bgr_hndl.width;
  int height = bgr_hndl.height;

  uint8_t *pBuf = (uint8_t *)malloc(bgr_hndl.bufSize);
  assert(nullptr != pBuf);
  bgr_hndl.TransferTo(pBuf);

  // printf("BGR: size=%d; seqNo=%ld; ts=%ld\n", bgr_hndl.bufSize, bgr_hndl.seqNo, bgr_hndl.ts);
  uint32_t size = bgr_hndl.bufSize;
  // FILE *tmp;
  // tmp = fopen("/home/admin/testimage.rawbytes", "wb");
  // fwrite(pBuf, sizeof(uint8_t), bgr_hndl.bufSize, tmp);
  // fclose(tmp);

  camera_block.ReleaseOutput(mxIf::CameraBlock::Outputs::BGR, bgr_hndl);

  return Py_BuildValue("(y#ii)", pBuf, size, width, height);
}

static PyObject *method_startinference(PyObject *self, PyObject *args)
{
  char *out;
  if (!PyArg_ParseTuple(args, "s", &out))
  {
    return NULL;
  }

  blob_file = (char *)malloc(strlen(out) + 1);
  strcpy(blob_file, out);

  pthread_t tid;
  store = false;
  inference = true;
  pthread_create(&tid, NULL, pullThread, NULL);
  return Py_BuildValue("");
}

static PyMethodDef EyeMethods[] = {
    {"start_inference", method_startinference, METH_VARARGS, "Start model inference"},
    {"stop_inference", method_stopinference, METH_VARARGS, "Stop model inference"},
    {"get_inference", method_getinference, METH_VARARGS, "Gets the current model inference output"},
    {"start_recording", method_startrecording, METH_VARARGS, "Start Azure Eye video recording"},
    {"get_frame", method_getframe, METH_VARARGS, "Returns an image taken by the AzureEye camera"},
    {"stop_recording", method_stoprecording, METH_VARARGS, "Stop Azure Eye video recording"},
    {"prepare_eye", method_prepareeye, METH_VARARGS, "Prepares the Azure Eye device"},
    {"close_eye", method_closedevice, METH_VARARGS, "Closes the Eye device"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef _azureeyemodule = {
    PyModuleDef_HEAD_INIT,
    "_azureeye",
    "Azure Eye interface",
    -1,
    EyeMethods};

PyMODINIT_FUNC PyInit__azureeye(void)
{
  return PyModule_Create(&_azureeyemodule);
}
