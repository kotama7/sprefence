#include <nuttx/config.h>
#include <stdio.h>
#include <nuttx/video/video.h>
#include <arch/board/cxd56_imageproc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/boardctl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <poll.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <nuttx/usb/usbdev.h>
#include <nuttx/usb/cdcacm.h>
#include <nuttx/usb/usbdev_trace.h>

#define GNSS_POLL_FD_NUM 2
#define GNSS_POLL_TIMEOUT_FOREVER -1
#define GNSS_SIG_TERM 18
#define GNSS_SIG_SPECTRUM 19
#define GNSS_SIG_DCREPORT 20
#define GNSS_SIG_SARRLM 21
#define CMD_RBUF_SIZE 128

#define TRACE_INIT_BITS (0)
#define TRACE_ERROR_BITS (TRACE_DEVERROR_BIT | TRACE_CLSERROR_BIT)
#define TRACE_CLASS_BITS (0)
#define TRACE_TRANSFER_BITS (0)
#define TRACE_CONTROLLER_BITS (0)
#define TRACE_INTERRUPT_BITS (0)
#define TRACE_BITSET (TRACE_INIT_BITS | TRACE_ERROR_BITS | TRACE_CLASS_BITS | \
                      TRACE_TRANSFER_BITS | TRACE_CONTROLLER_BITS | TRACE_INTERRUPT_BITS)

#define SERIAL_8N1 0x06
#define USBSER_DEVNAME "/dev/ttyACM0"

static int video_fd = -1;
static unsigned char *frame_mem;
static const int FSIZE = VIDEO_HSIZE_QVGA * VIDEO_VSIZE_QVGA * sizeof(uint16_t) * 2;
static const int YUVSIZE = VIDEO_HSIZE_QVGA * VIDEO_VSIZE_QVGA * sizeof(uint16_t);

static int camera_prepare(void)
{
  int ret;

  if (video_fd >= 0)
  {
    return 0;
  }

  if (video_initialize("/dev/video") != 0)
  {
    return -1;
  }

  video_fd = open("/dev/video", 0);
  if (video_fd < 0)
  {
    video_uninitialize();
    return -1;
  }

  struct v4l2_format fmt = {0};
  struct v4l2_requestbuffers req = {0};

  /* VIDIOC_REQBUFS initiate user pointer I/O */

  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;
  req.count = 1;
  req.mode = V4L2_BUF_MODE_RING;

  ret = ioctl(video_fd, VIDIOC_REQBUFS, (unsigned long)&req);
  if (ret < 0)
  {
    close(video_fd);
    video_uninitialize();
    printf("Failed to VIDIOC_REQBUFS: errno = %d\n", errno);
    return ret;
  }

  /* VIDIOC_S_FMT set format */

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = VIDEO_HSIZE_VGA;
  fmt.fmt.pix.height = VIDEO_VSIZE_VGA;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG_WITH_SUBIMG;

  ret = ioctl(video_fd, VIDIOC_S_FMT, (unsigned long)&fmt);

  if (ret < 0)
  {
    close(video_fd);
    video_uninitialize();
    printf("Failed to VIDIOC_S_FMT0: errno = %d\n", errno);
    return ret;
  }

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = VIDEO_HSIZE_QVGA;
  fmt.fmt.pix.height = VIDEO_VSIZE_QVGA;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SUBIMG_UYVY;
  ret = ioctl(video_fd, VIDIOC_S_FMT, (unsigned long)&fmt);
  if (ret < 0)
  {
    close(video_fd);
    video_uninitialize();
    printf("Failed to VIDIOC_S_FMT1: errno = %d\n", errno);
    return ret;
  }
  /* Prepare video memory to store images */

  /* VIDIOC_STREAMON start stream */

  ret = ioctl(video_fd, VIDIOC_STREAMON, (unsigned long)&req.type);
  if (ret < 0)
  {
    close(video_fd);
    video_uninitialize();
    printf("Failed to VIDIOC_STREAMON: errno = %d\n", errno);
    return ret;
  }
  return 0;
}

static int set_camimage(void)
{

  frame_mem = (unsigned char *)memalign(32, FSIZE);
  if (!frame_mem)
  {
    close(video_fd);
    video_uninitialize();
    printf("Failed to alocate frame_mem\n");
    return -1;
  }

  struct v4l2_buffer buf = {0};

  /* VIDIOC_QBUF sets buffer pointer into video driver again. */

  memset(&buf, 0, sizeof(struct v4l2_buffer));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_USERPTR;
  buf.index = 0;
  buf.m.userptr = (unsigned long)frame_mem;
  buf.length = FSIZE;

  if (ioctl(video_fd, VIDIOC_QBUF, (unsigned long)&buf) != 0)
  {
    close(video_fd);
    free(frame_mem);
    video_uninitialize();
    printf("%d", video_fd);
    printf("Fail QBUF %d\n", errno);
    return ERROR;
  }
  return OK;
}

static unsigned char *get_camimage(int *len)
{
  int ret;
  struct v4l2_buffer v4l2_buf;

  /* VIDIOC_DQBUF acquires captured data. */

  memset(&v4l2_buf, 0, sizeof(v4l2_buf));
  v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  v4l2_buf.memory = V4L2_MEMORY_USERPTR;

  ret = ioctl(video_fd, VIDIOC_DQBUF, (unsigned long)&v4l2_buf);
  if (ret)
  {
    printf("Fail DQBUF %d\n", errno);
    return NULL;
  }
  *len = v4l2_buf.bytesused;
  return (unsigned char *)v4l2_buf.m.userptr;
}

int availableForWrite(int rwfd)
{
  int count = 0;
  ioctl(rwfd, FIONSPACE, (long unsigned int)&count);
  return count;
}

int main(int argc, char *argv[])
{
  struct boardioc_usbdev_ctrl_s ctrl;
  FAR void *handle;
  int rwfd;
  int ret;

  ret = camera_prepare();

  if (ret < 0)
  {
    return ret;
  }

  /* Initialize the USB serial driver */

  printf("%s: Registering USB serial driver\n", __func__);

  /* Then, in any event, configure trace data collection as configured */

  // usbtrace_enable(TRACE_BITSET);

  ctrl.usbdev = BOARDIOC_USBDEV_CDCACM;
  ctrl.action = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = 0;
  ctrl.handle = &handle;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
  {
    printf("%s: WARN: Maybe already registered the USB serial device: %d\n",
           __func__, -ret);
  }
  else
  {
    printf("%s: Successfully registered the serial driver\n", __func__);
  }

  sleep(1);

  /* Open the USB serial device for reading (non-blocking) */

  rwfd = open(USBSER_DEVNAME, O_RDWR | O_NOCTTY);
  if (rwfd < 0)
  {
    printf("%s: ERROR: Failed to open " USBSER_DEVNAME
           " for reading: %d\n",
           __func__, errno);
    ctrl.action = BOARDIOC_USBDEV_DISCONNECT;
    boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
    return errno;
  }

  printf("%s: Successfully opened the serial driver\n", __func__);

  struct termios tio;
  tcgetattr(rwfd, &tio);
  tio.c_cflag = CS8;
  tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tio.c_oflag &= ~(ONLCR | OCRNL);
  cfsetspeed(&tio, B115200);
  tcsetattr(rwfd, TCSANOW, &tio);
  tcflush(rwfd, TCIOFLUSH);

  unsigned char buff[256];
  unsigned char *mem;
  int usedLen = 0;
  memset(buff, 0, sizeof(buff));
  do
  {
    int sz;
    int n;

    sz = read(rwfd, buff, 256);
    if (sz < 0)
    {
      continue;
    }

    printf("buff : %s\n", buff);
    if (!strncmp(buff, "camera", 6))
    {
      printf("recived : camera\n");
      ret = set_camimage();
      if (ret < 0)
      {
        return ret;
      }
      mem = get_camimage(&usedLen);
      int jpgLen = usedLen - YUVSIZE;
      printf("len : %d\n", jpgLen);
      int total_size = 0;
      int i;
      for (i = 0; i < jpgLen / 7.0; ++i)
      {
        int wSize = jpgLen - i * 7;
        if (wSize > 7)
        {
          wSize = 7;
        }
        ++wSize;
        unsigned char wbuf[8];
        memset(wbuf, 0, sizeof(wbuf));
        unsigned char append = 0;
        int j;
        for (j = 0; j < wSize - 1; ++j)
        {
          unsigned char tmp = 0;
          tmp = 0x7f & mem[i * 7 + j + YUVSIZE];
          tmp += 32;
          if (tmp >= 127)
          {
            tmp += 33;
          }
          wbuf[j] = tmp;
          append |= (0x80 & mem[i * 7 + j + YUVSIZE]) >> (j + 1);
        }
        append += 32;
        if (append >= 127)
        {
          append += 33;
        }
        wbuf[wSize - 1] = append;
        total_size += write(rwfd, wbuf, wSize);
        
        if (i % 75 == 74)
        {
          printf("%d\n", i);
          tcdrain(rwfd);
          usleep(10000);
        }
      }

      write(rwfd, "EndOfFile", 10);
      tcdrain(rwfd);
      printf("writed : %d bytes\n", total_size);
      FILE* fp = fopen("/mnt/sd0/PIC/uyvy.yuv", "wb");
      if(fp == NULL){
        printf("fail to open file.");
        return -1;
      }
      fwrite(mem, sizeof(unsigned char), YUVSIZE, fp);
      fclose(fp);
      free(frame_mem);
    }
    else if (!strncmp(buff, "set", 3))
    {
      printf("recived : set\n");
      FILE* fp = fopen("/mnt/sd0/CONFIG/hazard.ini", "w");
      if(fp == NULL){
        printf("fail to open file.");
        return -1;
      }
      fwrite(buff + 4, sizeof(unsigned char), 16, fp);
      fclose(fp);
    }
    else if (!strncmp(buff, "on", 2))
    {
      printf("recived : on\n");
      FILE* fp = fopen("/mnt/sd0/CONFIG/switch.ini", "w");
      if(fp == NULL){
        printf("fail to open file.");
        return -1;
      }
      fwrite(buff, sizeof(unsigned char), 3, fp);
      fclose(fp);
    }
    memset(buff, 0, sizeof(buff));
  } while (ret != -ESHUTDOWN);

  ctrl.action = BOARDIOC_USBDEV_DISCONNECT;
  boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);

  return 0;
}
