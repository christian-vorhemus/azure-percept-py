
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
// #include <jpeglib.h>

// Adopted from https://www.twam.info/wp-content/uploads/2009/04/v4l2grab.c

#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer
{
  void *start;
  size_t length;
};

struct dimensions
{
  int width;
  int height;
  unsigned int n_buffers;
};

// struct metadata
// {
//   char *deviceName;
//   struct buffer *buffers;
//   unsigned int width;
//   unsigned int height;
//   unsigned int samples;
//   unsigned int n_buffers;
//   int fd;
// };

// static unsigned char jpegQuality = 70;
// static char *jpegFilename = NULL;
// static char *deviceName = "/dev/video0";

static void YUV422toBGR(int width, int height, unsigned char *src, unsigned char *dst)
{
  int line, column;
  unsigned char *py, *pu, *pv;
  unsigned char b[width * height * sizeof(char)];
  unsigned char g[width * height * sizeof(char)];
  unsigned char r[width * height * sizeof(char)];
  size_t bSize = 0;
  size_t gSize = 0;
  size_t rSize = 0;

  py = src;
  pu = src + 1;
  pv = src + 3;

#define CLIP(x) ((x) >= 0xFF ? 0xFF : ((x) <= 0x00 ? 0x00 : (x)))

  size_t index = 0;
  for (line = 0; line < height; ++line)
  {
    for (column = 0; column < width; ++column)
    {
      b[index] = CLIP((double)*py + 1.772 * ((double)*pu - 128.0));
      bSize++;
      g[index] = CLIP((double)*py - 0.344 * ((double)*pu - 128.0) - 0.714 * ((double)*pv - 128.0));
      gSize++;
      r[index] = CLIP((double)*py + 1.402 * ((double)*pv - 128.0));
      rSize++;
      index++;

      // increase py every time
      py += 2;
      // increase pu,pv every second time
      if ((column & 1) == 1)
      {
        pu += 4;
        pv += 4;
      }
    }
  }
  memcpy(dst, b, bSize);
  memcpy(dst + bSize, g, gSize);
  memcpy(dst + bSize + gSize, r, rSize);
}

/**
  Print error message and terminate programm with EXIT_FAILURE return code.
  \param s error message to print
*/
static void errno_exit(const char *s)
{
  fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
  exit(EXIT_FAILURE);
}

/**
  Do ioctl and retry if error was EINTR ("A signal was caught during the ioctl() operation."). Parameters are the same as on ioctl.

  \param fd file descriptor
  \param request request
  \param argp argument
  \returns result from ioctl
*/
static int xioctl(int fd, int request, void *argp)
{
  int r;

  do
    r = ioctl(fd, request, argp);
  while (-1 == r && EINTR == errno);

  return r;
}

/**
  Write image to jpeg file.

  \param img image to write
*/
// static void jpegWrite(unsigned char *img)
// {
//   struct jpeg_compress_struct cinfo;
//   struct jpeg_error_mgr jerr;

//   JSAMPROW row_pointer[1];
//   FILE *outfile = fopen(jpegFilename, "wb");

//   // try to open file for saving
//   if (!outfile)
//   {
//     errno_exit("jpeg");
//   }

//   // create jpeg data
//   cinfo.err = jpeg_std_error(&jerr);
//   jpeg_create_compress(&cinfo);
//   jpeg_stdio_dest(&cinfo, outfile);

//   // set image parameters
//   cinfo.image_width = width;
//   cinfo.image_height = height;
//   cinfo.input_components = 3;
//   cinfo.in_color_space = JCS_RGB;

//   // set jpeg compression parameters to default
//   jpeg_set_defaults(&cinfo);
//   // and then adjust quality setting
//   jpeg_set_quality(&cinfo, jpegQuality, TRUE);

//   // start compress
//   jpeg_start_compress(&cinfo, TRUE);

//   // feed data
//   while (cinfo.next_scanline < cinfo.image_height)
//   {
//     row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
//     jpeg_write_scanlines(&cinfo, row_pointer, 1);
//   }

//   // finish compression
//   jpeg_finish_compress(&cinfo);

//   // destroy jpeg data
//   jpeg_destroy_compress(&cinfo);

//   // close output file
//   fclose(outfile);
// }

/**
  process image read
*/
static void imageProcess(const void *p, unsigned char *dst, unsigned int width, unsigned int height)
{
  unsigned char *src = (unsigned char *)p;
  int size = width * height * 3 * sizeof(char);
  YUV422toBGR(width, height, src, dst);
}

/**
  read single frame
*/

static int frameRead(unsigned char *dst, unsigned int n_buffers, struct buffer ***buffer, unsigned int width, unsigned int height, int fd)
{
  struct buffer *buffers = **buffer;
  struct v4l2_buffer buf;

  CLEAR(buf);

  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
  {
    switch (errno)
    {
    case EAGAIN:
      return 0;

    case EIO:
      // Could ignore EIO, see spec

      // fall through
    default:
      errno_exit("VIDIOC_DQBUF");
    }
  }

  assert(buf.index < n_buffers);
  imageProcess(buffers[buf.index].start, dst, width, height);

  if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
    errno_exit("VIDIOC_QBUF");
  return 1;
}

/** 
  mainloop: read frames and process them
*/
void usbCapture(int fd, unsigned char *dst, unsigned int n_buffers, struct buffer **buffers, unsigned int width, unsigned int height)
{
  unsigned int count;
  count = 1;

  while (count-- > 0)
  {
    for (;;)
    {
      fd_set fds;
      struct timeval tv;
      int r;

      FD_ZERO(&fds);
      FD_SET(fd, &fds);

      /* Timeout. */
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      r = select(fd + 1, &fds, NULL, NULL, &tv);

      if (-1 == r)
      {
        if (EINTR == errno)
          continue;

        errno_exit("select");
      }

      if (0 == r)
      {
        fprintf(stderr, "select timeout\n");
        exit(EXIT_FAILURE);
      }

      int res = frameRead(dst, n_buffers, &buffers, width, height, fd);
      if (res)
        break;

      /* EAGAIN - continue select loop. */
    }
  }
}

/**
  stop capturing
*/
void captureStop(void)
{
  enum v4l2_buf_type type;
}

/**
  start capturing
*/
void captureStart(int fd, unsigned int n_buffers)
{
  unsigned int i;
  enum v4l2_buf_type type;

  for (i = 0; i < n_buffers; ++i)
  {
    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
      errno_exit("VIDIOC_QBUF");
  }

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
    errno_exit("VIDIOC_STREAMON");
}

void deviceUninit(unsigned int n_buffers, struct buffer *buffers)
{
  unsigned int i;

  for (i = 0; i < n_buffers; ++i)
    if (-1 == munmap(buffers[i].start, buffers[i].length))
      errno_exit("munmap");

  free(buffers);
}

unsigned int mmapInit(char *deviceName, unsigned int n_buffers, struct buffer **buffers, int fd)
{
  struct v4l2_requestbuffers req;

  CLEAR(req);

  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
  {
    if (EINVAL == errno)
    {
      fprintf(stderr, "%s does not support memory mapping\n", deviceName);
      exit(EXIT_FAILURE);
    }
    else
    {
      errno_exit("VIDIOC_REQBUFS");
    }
  }

  if (req.count < 2)
  {
    fprintf(stderr, "Insufficient buffer memory on %s\n", deviceName);
    exit(EXIT_FAILURE);
  }

  struct buffer *tm = (struct buffer *)calloc(req.count, sizeof(**buffers));

  if (!tm)
  {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
  for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
  {
    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n_buffers;

    if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
      errno_exit("VIDIOC_QUERYBUF");

    tm[n_buffers].length = buf.length;
    tm[n_buffers].start =
        mmap(NULL /* start anywhere */, buf.length, PROT_READ | PROT_WRITE /* required */, MAP_SHARED /* recommended */, fd, buf.m.offset);

    if (MAP_FAILED == tm[n_buffers].start)
      errno_exit("mmap");
  }
  *buffers = tm;
  return n_buffers;
}

/**
  initialize device
*/
struct dimensions deviceInit(char *deviceName, int fd, int width, int height)
{
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  struct v4l2_format fmt;
  unsigned int min;

  if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
  {
    if (EINVAL == errno)
    {
      fprintf(stderr, "%s is no V4L2 device\n", deviceName);
      exit(EXIT_FAILURE);
    }
    else
    {
      errno_exit("VIDIOC_QUERYCAP");
    }
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
  {
    fprintf(stderr, "%s is no video capture device\n", deviceName);
    exit(EXIT_FAILURE);
  }

  /* Select video input, video standard and tune here. */
  CLEAR(cropcap);

  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
  {
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
    {
      switch (errno)
      {
      case EINVAL:
        /* Cropping not supported. */
        break;
      default:
        /* Errors ignored. */
        break;
      }
    }
  }
  else
  {
    /* Errors ignored. */
  }

  CLEAR(fmt);

  // v4l2_format
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = width;
  fmt.fmt.pix.height = height;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

  if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
    errno_exit("VIDIOC_S_FMT");

  /* Note VIDIOC_S_FMT may change width and height. */
  if (width != fmt.fmt.pix.width)
  {
    width = fmt.fmt.pix.width;
    //fprintf(stderr, "Image width set to %i by device %s.\n", width, deviceName);
  }
  if (height != fmt.fmt.pix.height)
  {
    height = fmt.fmt.pix.height;
    //fprintf(stderr, "Image height set to %i by device %s.\n", height, deviceName);
  }

  /* Buggy driver paranoia. */
  min = fmt.fmt.pix.width * 2;
  if (fmt.fmt.pix.bytesperline < min)
    fmt.fmt.pix.bytesperline = min;
  min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
  if (fmt.fmt.pix.sizeimage < min)
    fmt.fmt.pix.sizeimage = min;

  struct dimensions newDim = {width, height, 0};
  return newDim;
}

/**
  close device
*/
void deviceClose(int fd)
{
  if (-1 == close(fd))
    errno_exit("close");

  fd = -1;
}

int deviceOpen(char *deviceName)
{
  struct stat st;

  // stat file
  if (-1 == stat(deviceName, &st))
  {
    fprintf(stderr, "Cannot identify '%s': %d, %s\n", deviceName, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  // check if its device
  if (!S_ISCHR(st.st_mode))
  {
    fprintf(stderr, "%s is no device\n", deviceName);
    exit(EXIT_FAILURE);
  }

  // open device
  int fd = open(deviceName, O_RDWR /* required */ | O_NONBLOCK, 0);

  // check if opening was successfull
  if (-1 == fd)
  {
    fprintf(stderr, "Cannot open '%s': %d, %s\n", deviceName, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
  return fd;
}

// int main()
// {
//   struct buffer *buffers = NULL;

//   // struct metadata *m1 = malloc(sizeof(struct metadata));
//   // m1->deviceName = "/dev/video0";
//   // m1->width = 1280;
//   // m1->height = 720;
//   // m1->samples = 1;
//   // m1->n_buffers = 0;
//   // m1->fd = -1;

//   // printf("%s\n", m1->deviceName);

//   // free(m1);

//   char deviceName[] = "/dev/video0";
//   unsigned int width = 1280;
//   unsigned int height = 720;
//   unsigned int samples = 1;
//   unsigned int n_buffers = 0;
//   int fd = -1;

//   fd = deviceOpen(deviceName);
//   struct dimensions newDim = deviceInit(deviceName, fd, width, height);
//   n_buffers = mmapInit(deviceName, n_buffers, &buffers, fd);
//   width = newDim.width;
//   height = newDim.height;
//   unsigned char *dst = malloc(width * height * 3 * sizeof(char));
//   int size = width * height * 3 * sizeof(char);
//   captureStart(fd, n_buffers);
//   for (int i = 0; i < samples; i++)
//   {
//     usbCapture(fd, dst, n_buffers, &buffers, width, height);
//   }

//   FILE *tmp;
//   tmp = fopen("./test3.raw", "wb");
//   fwrite(dst, sizeof(char), size, tmp);
//   fclose(tmp);
//   captureStop();
//   deviceUninit(n_buffers, buffers);
//   deviceClose(fd);
//   free(dst);

//   exit(EXIT_SUCCESS);

//   return 0;
// }