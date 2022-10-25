#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <nuttx/video/video.h>
#include <arch/board/cxd56_imageproc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/boardctl.h>
#include <unistd.h>
#include <arch/board/board.h>
#include <arch/chip/pin.h>

static int video_fd = -1;
static unsigned char *frame_mem;
static unsigned char *pic_mem;
static const int FSIZE = VIDEO_HSIZE_QVGA * VIDEO_VSIZE_QVGA * sizeof(uint16_t);
static const int YWIDTH = VIDEO_HSIZE_QVGA;
static const int YHEIGHT = VIDEO_VSIZE_QVGA;
static const int UVWIDTH = VIDEO_HSIZE_QVGA / 2;
static const int UVHEIGHT = VIDEO_VSIZE_QVGA;
static const int UCENTER = 157;
static const int VCENTER = 110;
static const int URANGE = 20;
static const int VRANGE = 25;
static const int FENCE_ON_DURATION = 30;
static double hazardLT = 0.25;
static double hazardRT = 0.75;
static double hazardLB = 0.25;
static double hazardRB = 0.75;

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
    free(frame_mem);
    video_uninitialize();
    printf("Failed to VIDIOC_REQBUFS: errno = %d\n", errno);
    return ret;
  }

  /* VIDIOC_S_FMT set format */

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = VIDEO_HSIZE_QVGA;
  fmt.fmt.pix.height = VIDEO_VSIZE_QVGA;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;

  ret = ioctl(video_fd, VIDIOC_S_FMT, (unsigned long)&fmt);

  if (ret < 0)
  {
    close(video_fd);
    video_uninitialize();
    printf("Failed to VIDIOC_S_FMT: errno = %d\n", errno);
    return ret;
  }
  /* Prepare video memory to store images */

  frame_mem = (unsigned char *)memalign(32, FSIZE);
  if (!frame_mem)
  {
    close(video_fd);
    video_uninitialize();
    printf("Failed to alocate frame_mem\n");
    return -1;
  }

  pic_mem = (unsigned char *)memalign(32, FSIZE);
  if (!pic_mem)
  {
    close(video_fd);
    video_uninitialize();
    printf("Failed to alocate frame_mem\n");
    return -1;
  }

  /* VIDIOC_STREAMON start stream */

  ret = ioctl(video_fd, VIDIOC_STREAMON, (unsigned long)&req.type);
  if (ret < 0)
  {
    close(video_fd);
    free(frame_mem);
    video_uninitialize();
    printf("Failed to VIDIOC_STREAMON: errno = %d\n", errno);
    return ret;
  }
  return 0;
}

static int set_camimage(void)
{
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

static unsigned char *get_camimage(void)
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
  return (unsigned char *)v4l2_buf.m.userptr;
}

static unsigned char *load_pic(int *err)
{
  FILE *fp = fopen("/mnt/sd0/PIC/uyvy.yuv", "rb");
  if (fp == NULL)
  {
    printf("fail to open file.");
    *err = -1;
    return 0;
  }

  int sz = fread(pic_mem, sizeof(unsigned char), FSIZE, fp);
  printf("read %d bytes\n", sz);
  fclose(fp);
  *err = 0;
  return pic_mem;
}

void find_black_center(unsigned char *pic, unsigned char *cam, int *x, int *y)
{
  int diff = 0;
  int weight = 0;
  int x_sum = 0;
  int y_sum = 0;
  int i, j;
  for (i = 0; i < YHEIGHT; ++i)
  {
    for (j = 0; j < YWIDTH; ++j)
    {
      diff += pic[2 * (YWIDTH * i + j)] - cam[2 * (YWIDTH * i + j) + 1];
    }
  }
  printf("fin diff\n");
  diff /= YHEIGHT * YWIDTH;
  for (i = 0; i < YHEIGHT; ++i)
  {
    for (j = 0; j < YWIDTH; ++j)
    {
      int tmp = cam[2 * (YWIDTH * i + j) + 1] + diff - pic[2 * (YWIDTH * i + j) + 1];
      if (tmp < 25)
      {
        tmp = 0;
      }
      weight += tmp;
      x_sum += tmp * j;
      y_sum += tmp * i;
    }
  }

  if (weight <= 0)
  {
    *x = -1;
    *y = -1;
    printf("black not fount\n");
  }
  else
  {
    *x = x_sum / weight;
    *y = y_sum / weight;
    printf("black x : %d, y: %d\n", x_sum / weight, y_sum / weight);
  }
}

void find_skin_center(unsigned char *pic, unsigned char *cam, int *x, int *y)
{
  int weight = 0;
  int x_sum = 0;
  int y_sum = 0;
  int i, j;
  for (i = 0; i < UVHEIGHT; ++i)
  {
    for (j = 0; j < UVWIDTH; ++j)
    {
      unsigned char u = cam[4 * (YWIDTH * i + j)];
      unsigned char v = cam[4 * (YWIDTH * i + j) + 2];
      if (u < UCENTER - URANGE || u > UCENTER + URANGE)
      {
        continue;
      }
      if (v < VCENTER - VRANGE || v > VCENTER + VRANGE)
      {
        continue;
      }
      unsigned char up = pic[4 * (YWIDTH * i + j)];
      unsigned char vp = pic[4 * (YWIDTH * i + j) + 2];
      int diff = (u - up) * (u - up) + (v - vp) * (v - vp);
      weight += diff;
      x_sum += diff * j;
      y_sum += diff * i;
    }
    usleep(10000);
  }

  if (weight <= 0)
  {
    *x = -1;
    *y = -1;
    printf("skin not fount\n");
  }
  else
  {
    *x = x_sum / weight * 2;
    *y = y_sum / weight;
    printf("skin x : %d, y: %d\n", x_sum / weight * 2, y_sum / weight);
  }
}

static int GPIO_init(void)
{
  board_gpio_config(PIN_SPI4_SCK, 0, false, false, PIN_FLOAT);
  board_gpio_config(PIN_SPI4_MISO, 0, false, true, PIN_FLOAT);
  board_gpio_write(PIN_SPI4_SCK, 1);
  board_gpio_write(PIN_SPI4_MISO, 1);
  return 0;
}

static void Fence_on(void)
{
  board_gpio_write(PIN_SPI4_SCK, 0);

  // use direct
  board_gpio_write(PIN_SPI4_MISO, 0);
  // use transister
  // board_gpio_write(PIN_SPI4_MISO, -1);
  
  usleep(100 * 1000);
  board_gpio_write(PIN_SPI4_MISO, 1);
  usleep(FENCE_ON_DURATION * 1000 * 1000);

  // use direct
  board_gpio_write(PIN_SPI4_MISO, 0);
  // use transister
  // board_gpio_write(PIN_SPI4_MISO, -1);

  usleep(100 * 1000);
  board_gpio_write(PIN_SPI4_MISO, 1);
}

static void Fence_off(void)
{
  board_gpio_write(PIN_SPI4_SCK, -1);
}

static int Load_setting()
{
  FILE *fp = fopen("/mnt/sd0/CONFIG/hazard.ini", "r");
  if (fp == NULL)
  {
    return -1;
  }

  fscanf(fp, "%lf %lf %lf %lf", &hazardLT, &hazardRT, &hazardLB, &hazardRB);

  fclose(fp);
  return 0;
}

static bool IsHazard(float x, float y)
{
  double dL = hazardLT - hazardLB;
  double lX = 0.0;
  if (dL > -0.001 && dL < 0.001)
  {
    lX = hazardLB;
  }
  else
  {
    double gradL = 1.0 / dL;
    lX = hazardLB + gradL * y;
  }

  if (x < lX)
  {
    return true;
  }

  double dR = hazardRT - hazardRB;
  double rX = 0.0;

  if (dR > -0.001 && dR < 0.001)
  {
    rX = hazardRB;
  }
  else
  {
    double gradR = 1.0 / dR;
    rX = hazardRB + gradR * y;
  }

  if (rX < x)
  {
    return true;
  }

  return false;
}

int main(int argc, char *argv[])
{
  int ret;

  printf("program start\n");
  GPIO_init();
  Load_setting();

  ret = camera_prepare();

  if (ret < 0)
  {
    printf("Camera prepare faild.\n");
    return ret;
  }

  printf("camara prepared\n");

  unsigned char *pic;
  unsigned char *cam;
  bool isLoaded = false;
  bool isOn = false;
  int i = 0;
  int bx = -1;
  int by = -1;
  int sx = -1;
  int sy = -1;

  while (true)
  {
    printf("index : %d\n", i);
    FILE *fp = fopen("/mnt/sd0/CONFIG/switch.ini", "r");
    if (fp != NULL)
    {
      char cbuff[5];
      fread(cbuff, sizeof(char), 5, fp);
      if (!strncmp(cbuff, "on", 3))
      {
        Fence_on();
        isOn = true;
      }
      else if (!strncmp(cbuff, "off", 4))
      {
        Fence_off();
        isOn = false;
      }
      fclose(fp);
      remove("/mnt/sd0/CONFIG/switch.ini");
    }
    else
    {
      if (!isLoaded || i == 0)
      {
        int err = 0;
        pic = load_pic(&err);
        if (err < 0)
        {
          continue;
        }
        isLoaded = true;
        printf("pic loaded\n");
      }

      ret = set_camimage();
      if (ret < 0)
      {
        printf("Set camera image faild.\n");
        return ret;
      }

      cam = get_camimage();
      usleep(1000);
      printf("get cam image\n");
      find_black_center(pic, cam, &bx, &by);
      printf("find black center\n");
      find_skin_center(pic, cam, &sx, &sy);
      printf("find skin center\n");

      int x = 0;
      int y = 0;

      if (bx < 0 && sx < 0)
      {
        printf("cannot find face\n");
      }
      else if (bx < 0)
      {
        x = sx;
        y = sy;
        printf("ditect at x : %d, y : %d\n", sx, sy);
      }
      else if (sx < 0)
      {
        x = bx;
        y = by;
        printf("ditect at x : %d, y : %d\n", bx, by);
      }
      else
      {
        x = (bx + sx) / 2;
        y = (by + sy) / 2;
        printf("ditect at x : %d, y : %d\n", x, y);
      }

      if (bx >= 0 || sx >= 0)
      {
        float fx = (float)x / VIDEO_HSIZE_QVGA;
        float fy = (float)y / VIDEO_VSIZE_QVGA;
        if (IsHazard(x, y))
        {
          if (!isOn)
          {
            Fence_on();
          }
          isOn = true;
        }
        else
        {
          if (isOn)
          {
            Fence_off();
          }
          isOn = false;
        }
      }
    }

    i += 1;
    i %= 60;
    usleep(1000000);
  }

  return 0;
}
