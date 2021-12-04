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
extern "C" void usbCapture(int fd, unsigned char *dst, unsigned int n_buffers, struct buffer **buffers, unsigned int width, unsigned int height);
extern "C" void captureStop(void);
extern "C" void captureStart(int fd, unsigned int n_buffers);
extern "C" void deviceUninit(unsigned int n_buffers, struct buffer *buffers);
extern "C" struct dimensions deviceInit(char *deviceName, int fd, int width, int height);
extern "C" void deviceClose(int fd);
extern "C" int deviceOpen(char *deviceName);
extern "C" unsigned int mmapInit(char *deviceName, unsigned int n_buffers, struct buffer **buffers, int fd);