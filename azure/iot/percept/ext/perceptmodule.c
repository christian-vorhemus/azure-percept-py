#include <Python.h>
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

FILE *out;
snd_pcm_format_t format;
char *buffer;
int buffer_frames;
int duration;
unsigned int rate;
unsigned int channels;
char *hardware;
snd_pcm_uframes_t frames;
snd_pcm_t *capture_handle;
snd_pcm_hw_params_t *hw_params;
struct timeval tval_before, tval_after, tval_result;
bool isRecording = false;

struct WavHeader
{
  char RIFF_marker[4];
  uint32_t file_size;
  char filetype_header[4];
  char format_marker[4];
  uint32_t data_header_length;
  uint16_t format_type;
  uint16_t number_of_channels;
  uint32_t sample_rate;
  uint32_t bytes_per_second;
  uint16_t bytes_per_frame;
  uint16_t bits_per_sample;
};

struct WavHeader *createWavHeader(uint32_t sample_rate, uint16_t bit_depth,
                                  uint16_t channels)
{
  struct WavHeader *hdr;
  hdr = malloc(sizeof(*hdr));
  if (!hdr)
    return NULL;

  memcpy(&hdr->RIFF_marker, "RIFF", 4);
  memcpy(&hdr->filetype_header, "WAVE", 4);
  memcpy(&hdr->format_marker, "fmt ", 4);
  hdr->data_header_length = 16;
  hdr->format_type = 1;
  hdr->number_of_channels = channels;
  hdr->sample_rate = sample_rate;
  hdr->bytes_per_second = sample_rate * channels * bit_depth / 8;
  hdr->bytes_per_frame = channels * bit_depth / 8;
  hdr->bits_per_sample = bit_depth;

  return hdr;
}

int writeWAVHeader(FILE *out, struct WavHeader *hdr)
{
  if (!hdr)
    return -1;

  fwrite(&hdr->RIFF_marker, 1, 4, out);
  fwrite(&hdr->file_size, 1, 4, out);
  fwrite(&hdr->filetype_header, 1, 4, out);
  fwrite(&hdr->format_marker, 1, 4, out);
  fwrite(&hdr->data_header_length, 1, 4, out);
  fwrite(&hdr->format_type, 1, 2, out);
  fwrite(&hdr->number_of_channels, 1, 2, out);
  fwrite(&hdr->sample_rate, 1, 4, out);
  fwrite(&hdr->bytes_per_second, 1, 4, out);
  fwrite(&hdr->bytes_per_frame, 1, 2, out);
  fwrite(&hdr->bits_per_sample, 1, 2, out);
  fwrite("data", 1, 4, out);
  uint32_t data_size = hdr->file_size + 8 - 44;
  fwrite(&data_size, 1, 4, out);
  return 0;
}

void *record(void *args)
{
  int err;
  // fprintf(stdout, "Recording...\n");
  isRecording = true;
  buffer = (char *)malloc(buffer_frames * snd_pcm_format_width(format) / 8 * channels);
  // fprintf(stdout, "buffer allocated\n");
  // write empty header
  for (int i = 0; i < 44; i++)
  {
    fputc(0x00, out);
  }
  gettimeofday(&tval_before, NULL);
  if (!capture_handle)
  {
    fprintf(stderr, "Error: capture_handle is NULL\n");
    return NULL;
  }
  while (isRecording)
  {
    // fputc(0x05, out);
    // sleep(1);
    if ((err = snd_pcm_readi(capture_handle, buffer, buffer_frames)) !=
        buffer_frames)
    {
      fprintf(stderr, "read from audio interface failed (%s)\n", err,
              snd_strerror(err));
      exit(1);
    }

    fwrite(buffer, 1, buffer_frames * snd_pcm_format_width(format) / 8 * channels, out);
  }
  return NULL;
}

static PyObject *method_getraw(PyObject *self, PyObject *args)
{
  int size;
  PyObject *res;
  int err;
  if (!PyArg_ParseTuple(args, "i", &size))
  {
    return NULL;
  }

  char *buf = (char *)malloc(size * snd_pcm_format_width(format) / 8 * channels);
  if ((err = snd_pcm_readi(capture_handle, buf, size)) != size)
  {
    fprintf(stderr, "read from audio interface failed (%s)\n", err,
            snd_strerror(err));
    exit(1);
  }
  res = PyBytes_FromString(buf);
  free(buf);
  return res;
}

static PyObject *method_startrecording(PyObject *self, PyObject *args)
{

  PyObject *f, *fileno_fn, *fileno_obj, *fileno_args;
  if (!PyArg_ParseTuple(args, "O", &f))
  {
    return NULL;
  }

  if (!(fileno_fn = PyObject_GetAttrString(f, "fileno")))
  {
    PyErr_SetString(PyExc_TypeError, "Object has no fileno function.");
    return NULL;
  }
  fileno_args = PyTuple_New(0);
  if (!(fileno_obj = PyObject_CallObject(fileno_fn, fileno_args)))
  {
    PyErr_SetString(PyExc_SystemError, "Error calling fileno function.");
    return NULL;
  }
  int fd = PyLong_AsSize_t(fileno_obj);

  out = fdopen(fd, "wb");

  pthread_t tid;
  pthread_create(&tid, NULL, record, NULL);
  return Py_BuildValue("");
}

static PyObject *method_preparedevice(PyObject *self, PyObject *args)
{
  int err;
  if (!PyArg_ParseTuple(args, "s", &hardware))
  {
    return NULL;
  }

  // fprintf(stderr, "Hardware: %s\n", hardware);

  buffer_frames = 160;
  rate = 16000;
  channels = 5;
  frames = 32;
  format = SND_PCM_FORMAT_S32_LE;

  if ((err = snd_pcm_open(&capture_handle, hardware, SND_PCM_STREAM_CAPTURE,
                          0)) < 0)
  {
    fprintf(stderr, "cannot open audio device %s (%s)\n", hardware,
            snd_strerror(err));
    exit(1);
  }

  // fprintf(stdout, "audio interface opened\n");

  if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
  {
    fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n",
            snd_strerror(err));
    exit(1);
  }

  // fprintf(stdout, "hw_params allocated\n");

  if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0)
  {
    fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n",
            snd_strerror(err));
    exit(1);
  }

  // fprintf(stdout, "hw_params initialized\n");

  if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params,
                                          SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
  {
    fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
    exit(1);
  }

  // fprintf(stdout, "hw_params access setted\n");

  if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format)) <
      0)
  {
    fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
    exit(1);
  }

  // fprintf(stdout, "hw_params format setted\n");

  if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate,
                                             0)) < 0)
  {
    fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
    exit(1);
  }

  // fprintf(stdout, "hw_params rate setted\n");

  if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, channels)) <
      0)
  {
    fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
    exit(1);
  }

  // fprintf(stdout, "hw_params channels setted\n");

  if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0)
  {
    fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
    exit(1);
  }

  // fprintf(stdout, "hw_params setted\n");

  snd_pcm_hw_params_free(hw_params);

  // fprintf(stdout, "hw_params freed\n");

  if ((err = snd_pcm_prepare(capture_handle)) < 0)
  {
    fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
            snd_strerror(err));
    exit(1);
  }

  // fprintf(stdout, "audio interface prepared\n");

  return Py_BuildValue("");
}

static PyObject *method_stoprecording(PyObject *self, PyObject *args)
{
  isRecording = false;
  gettimeofday(&tval_after, NULL);
  timersub(&tval_after, &tval_before, &tval_result);
  rewind(out);
  struct WavHeader *hdr;
  hdr = createWavHeader(rate, frames, channels);
  uint32_t pcm_data_size =
      hdr->sample_rate * hdr->bytes_per_frame * tval_result.tv_sec;
  hdr->file_size = pcm_data_size + 44 - 8;
  writeWAVHeader(out, hdr);
  fclose(out);
  free(buffer);
  // fprintf(stdout, "buffer freed\n");
  // snd_pcm_close(capture_handle);
  // fprintf(stdout, "audio interface closed\n");
  return Py_BuildValue("");
}

static PyObject *method_getazureearhardware(PyObject *self, PyObject *args)
{
  int deviceNo = 0;
  bool found = false;
  int cardNum = -1;
  int err;

  for (;;)
  {
    if ((err = snd_card_next(&cardNum)) < 0)
    {
      fprintf(stderr, "Can't get the next card: %s\n",
              snd_strerror(err));
      break;
    }

    if (cardNum < 0)
      break;

    char *cardname;
    snd_card_get_name(cardNum, &cardname);
    // printf("%s\n", cardname);

    if (strstr(cardname, "Azure Ear") != NULL)
    {
      found = true;
      break;
    }

    ++deviceNo;
  }

  snd_config_update_free_global();

  if (found)
  {
    return PyLong_FromLong(deviceNo);
  }
  else
  {
    return PyLong_FromLong(-1);
  }
}

static PyMethodDef HardwareMethods[] = {
    {"get_azure_ear_hardware", method_getazureearhardware, METH_VARARGS, "Find the hardware number for the Azure Ear device"},
    {"start_recording", method_startrecording, METH_VARARGS, "Start Azure Ear audio recording"},
    {"stop_recording", method_stoprecording, METH_VARARGS, "Stop Azure Ear audio recording"},
    {"get_raw_audio", method_getraw, METH_VARARGS, "Get audio frames as byte array"},
    {"prepare_device", method_preparedevice, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef _hardwaremodule = {
    PyModuleDef_HEAD_INIT,
    "_hardware",
    "Python interface for the fputs C library function",
    -1,
    HardwareMethods};

PyMODINIT_FUNC PyInit__hardware(void)
{
  return PyModule_Create(&_hardwaremodule);
}