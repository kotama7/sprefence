/****************************************************************************
 * externals/tensorflow/tf_image_provider.c
 *
 *   Copyright 2021 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <nuttx/video/video.h>
#include <arch/board/cxd56_imageproc.h>

#include "spresense_face_image_provider.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FACE_IMAGE_WIDTH       (320)
#define FACE_IMAGE_HEIGHT      (240)
#define FACE_IMAGE_DATASIZE    (FACE_IMAGE_WIDTH*FACE_IMAGE_HEIGHT*2) /* QVGA YUV422 */

#define FACE_IMG_WIDTH_SAMPLE_REQUIRED     (128)
#define FACE_IMG_HEIGHT_SAMPLE_REQUIRED    (128)
#define FACE_IMG_DATASIZE_SAMPLE_REQUIRED  \
  (FACE_IMG_WIDTH_SAMPLE_REQUIRED*FACE_IMG_HEIGHT_SAMPLE_REQUIRED*3)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int video_fd = -1;
static unsigned char *frame_mem;
static unsigned char *clip_mem;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: camera_prepare()
 *
 * Description:
 *   Allocate frame buffer for camera and Queue the allocated buffer
 *   into video driver.
 ****************************************************************************/

static int camera_prepare(enum v4l2_buf_type type,
                          uint32_t buf_mode, uint32_t pixformat,
                          uint16_t hsize, uint16_t vsize)
{
  int ret;
  struct v4l2_format         fmt = {0};
  struct v4l2_requestbuffers req = {0};

  /* VIDIOC_REQBUFS initiate user pointer I/O */

  req.type   = type;
  req.memory = V4L2_MEMORY_USERPTR;
  req.count  = 1;
  req.mode   = buf_mode;
  
  ret = ioctl(video_fd, VIDIOC_REQBUFS, (unsigned long)&req);
  if (ret < 0)
    {
      printf("Failed to VIDIOC_REQBUFS: errno = %d\n", errno);
      return ret;
    }

  /* VIDIOC_S_FMT set format */

  fmt.type                = type;
  fmt.fmt.pix.width       = hsize;
  fmt.fmt.pix.height      = vsize;
  fmt.fmt.pix.field       = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = pixformat;

  ret = ioctl(video_fd, VIDIOC_S_FMT, (unsigned long)&fmt);
  if (ret < 0)
    {
      printf("Failed to VIDIOC_S_FMT: errno = %d\n", errno);
      return ret;
    }

  /* Prepare video memory to store images */

  frame_mem = (unsigned char *)memalign(32, FACE_IMAGE_DATASIZE);
  if (!frame_mem)
    {
      return -1;
    }

  clip_mem = (unsigned char *)memalign(32, FACE_IMG_DATASIZE_SAMPLE_REQUIRED);
  if (!clip_mem)
    {
      free(frame_mem);
      return -1;
    }

  /* VIDIOC_STREAMON start stream */

  ret = ioctl(video_fd, VIDIOC_STREAMON, (unsigned long)&type);
  if (ret < 0)
    {
      printf("Failed to VIDIOC_STREAMON: errno = %d\n", errno);
      free(frame_mem);
      return ret;
    }

  return 0;
}

/****************************************************************************
 * Name: get_camimage()
 *
 * Description:
 *   DQBUF camera frame buffer from video driver with taken picture data.
 ****************************************************************************/

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

/****************************************************************************
 * Name: release_camimage()
 *
 * Description:
 *   Re-QBUF to set used frame buffer into video driver.
 ****************************************************************************/

static int set_camimage(void)
{
  struct v4l2_buffer         buf = {0};

  /* VIDIOC_QBUF sets buffer pointer into video driver again. */

  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_USERPTR;
  buf.index = 0;
  buf.m.userptr = (unsigned long)frame_mem;
  buf.length = FACE_IMAGE_DATASIZE;

  if (ioctl(video_fd, VIDIOC_QBUF, (unsigned long)&buf) != 0)
    {
      printf("Fail QBUF %d\n", errno);
      return ERROR;
    }
  return OK;
}

/****************************************************************************
 * Name: init_video()
 *
 * Description:
 *   Initialize video sub-system.
 ****************************************************************************/

static int init_video(void)
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

  ret = camera_prepare(V4L2_BUF_TYPE_VIDEO_CAPTURE,
                       V4L2_BUF_MODE_RING, V4L2_PIX_FMT_UYVY,
                       VIDEO_HSIZE_QVGA, VIDEO_VSIZE_QVGA);
  if (ret != OK)
    {
      close(video_fd);
      free(frame_mem);
      video_uninitialize();
      video_fd = -1;
      return -1;
    }

  imageproc_initialize();

  return 0;
}

/****************************************************************************
 * Name: convert_image0()
 *
 * Description:
 *   Return the buffer with captured image. resolution 1 / 2. center.
 ****************************************************************************/

int convert_image0(unsigned char *mem, float* out_data){
  printf("convert step 0.\n");
  const imageproc_rect_t clip_rect = {
    .x1 = (FACE_IMAGE_WIDTH  / 2) - FACE_IMG_WIDTH_SAMPLE_REQUIRED,
    .y1 = 0,
    .x2 = (FACE_IMAGE_WIDTH  / 2) + FACE_IMG_WIDTH_SAMPLE_REQUIRED  -1,
    .y2 = FACE_IMAGE_HEIGHT - 1
  };

  printf("x1: %d y1: %d x2: %d y2: %d\n", clip_rect.x1, clip_rect.y1, clip_rect.x2, clip_rect.y2);
  printf("%d \n", FACE_IMAGE_HEIGHT / 2);
  sleep(1);

  if (imageproc_clip_and_resize(mem, FACE_IMAGE_WIDTH, FACE_IMAGE_HEIGHT,
      clip_mem, FACE_IMG_WIDTH_SAMPLE_REQUIRED, FACE_IMAGE_HEIGHT / 2,
      16 /*YUV422*/, (imageproc_rect_t *)&clip_rect) == 0)
      {
        printf("convert step 1.\n");
        imageproc_convert_yuv2rgb(clip_mem,
            FACE_IMG_WIDTH_SAMPLE_REQUIRED, FACE_IMAGE_HEIGHT / 2);
        int pad = FACE_IMG_WIDTH_SAMPLE_REQUIRED * (FACE_IMG_HEIGHT_SAMPLE_REQUIRED - FACE_IMAGE_HEIGHT / 2) / 2 * 3;
        printf("pad: %d\n", pad);
        printf("odata size: %d\n", sizeof(out_data));
        sleep(1);
        printf("convert step 2.\n");
        for(int i = 0; i < pad; ++i){
          *(out_data + i) = -1.0f;
        }
        printf("convert step 3.\n");
        for(int i = pad; i < FACE_IMG_DATASIZE_SAMPLE_REQUIRED - pad; ++i){
          *(out_data + i) = *(clip_mem + i - pad) / 128.0f - 1.0f;
        }
        printf("convert step 4.\n");
        for(int i = FACE_IMG_DATASIZE_SAMPLE_REQUIRED - pad; i < FACE_IMG_DATASIZE_SAMPLE_REQUIRED; ++i){
          *(out_data + i) = -1.0f;
        }
        printf("convert step 5.\n");
        return 0;
      }
    else
      {
        printf("imageproc_clip_and_resize() is faled.\n");
      }
  return -1;
}

/****************************************************************************
 * Name: convert_image1()
 *
 * Description:
 *   Return the buffer with captured image. resolution 1 / 2. left.
 ****************************************************************************/

int convert_image1(unsigned char *mem, float* out_data){
  printf("convert step 0.\n");
  const imageproc_rect_t clip_rect = {
    .x1 = 0,
    .y1 = 0,
    .x2 = FACE_IMG_WIDTH_SAMPLE_REQUIRED * 2 - 1,
    .y2 = FACE_IMAGE_HEIGHT - 1
  };

  if (imageproc_clip_and_resize(mem, FACE_IMAGE_WIDTH, FACE_IMAGE_HEIGHT,
      clip_mem, FACE_IMG_WIDTH_SAMPLE_REQUIRED, FACE_IMAGE_HEIGHT / 2,
      16 /*YUV422*/, (imageproc_rect_t *)&clip_rect) == 0)
      {
        printf("convert step 1.\n");
        imageproc_convert_yuv2rgb(clip_mem,
            FACE_IMG_WIDTH_SAMPLE_REQUIRED, FACE_IMAGE_HEIGHT / 2);
        int pad = FACE_IMG_WIDTH_SAMPLE_REQUIRED * (FACE_IMG_HEIGHT_SAMPLE_REQUIRED - FACE_IMAGE_HEIGHT / 2) / 2 * 3;
        printf("convert step 2.\n");
        for(int i = 0; i < pad; ++i){
          *(out_data + i) = -1.0f;
        }
        printf("convert step 3.\n");
        for(int i = pad; i < FACE_IMG_DATASIZE_SAMPLE_REQUIRED - pad; ++i){
          *(out_data + i) = *(clip_mem +i - pad) / 128.0f - 1.0f;
        }
        printf("convert step 4.\n");
        for(int i = FACE_IMG_DATASIZE_SAMPLE_REQUIRED - pad; i < FACE_IMG_DATASIZE_SAMPLE_REQUIRED; ++i){
          *(out_data + i) = -1.0f;
        }
        printf("convert step 5.\n");
        return 0;
      }
    else
      {
        printf("imageproc_clip_and_resize() is faled.\n");
      }
  return -1;
}

/****************************************************************************
 * Name: convert_image2()
 *
 * Description:
 *   Return the buffer with captured image. resolution 1 / 2. right.
 ****************************************************************************/

int convert_image2(unsigned char *mem, float* out_data){
  printf("convert step 0.\n");
  const imageproc_rect_t clip_rect = {
    .x1 = FACE_IMAGE_WIDTH - FACE_IMG_WIDTH_SAMPLE_REQUIRED * 2,
    .y1 = 0,
    .x2 = FACE_IMAGE_WIDTH - 1,
    .y2 = FACE_IMAGE_HEIGHT - 1
  };

  if (imageproc_clip_and_resize(mem, FACE_IMAGE_WIDTH, FACE_IMAGE_HEIGHT,
      clip_mem, FACE_IMG_WIDTH_SAMPLE_REQUIRED, FACE_IMAGE_HEIGHT / 2,
      16 /*YUV422*/, (imageproc_rect_t *)&clip_rect) == 0)
      {
        printf("convert step 1.\n");
        imageproc_convert_yuv2rgb(clip_mem,
            FACE_IMG_WIDTH_SAMPLE_REQUIRED, FACE_IMAGE_HEIGHT / 2);
        int pad = FACE_IMG_WIDTH_SAMPLE_REQUIRED * (FACE_IMG_HEIGHT_SAMPLE_REQUIRED - FACE_IMAGE_HEIGHT / 2) / 2 * 3;
        printf("convert step 2.\n");
        for(int i = 0; i < pad; ++i){
          *(out_data + i) = -1.0f;
        }
        printf("convert step 3.\n");
        for(int i = pad; i < FACE_IMG_DATASIZE_SAMPLE_REQUIRED - pad; ++i){
          *(out_data + i) = *(clip_mem + i - pad) / 128.0f - 1.0f;
        }
        printf("convert step 4.\n");
        for(int i = FACE_IMG_DATASIZE_SAMPLE_REQUIRED - pad; i < FACE_IMG_DATASIZE_SAMPLE_REQUIRED; ++i){
          *(out_data + i) = -1.0f;
        }
        printf("convert step 5.\n");
        return 0;
      }
    else
      {
        printf("imageproc_clip_and_resize() is faled.\n");
      }
  return -1;
}

/****************************************************************************
 * Name: convert_image3_11()
 *
 * Description:
 *   Return the buffer with captured image. resolution 1. type: 3-11.
 ****************************************************************************/

int convert_image3_15(unsigned char *mem, float* out_data, int type){
  printf("convert step -1.\n");
  if (type < 3 || type > 15){
    return -1;
  }
  printf("convert step 0.\n");
  int x_offset = 64 * ((type - 3) % 4);
  int y_offset = 56 * ((type - 3) / 4);
  const imageproc_rect_t clip_rect = {
    .x1 = x_offset,
    .y1 = y_offset,
    .x2 = x_offset + FACE_IMG_WIDTH_SAMPLE_REQUIRED - 1,
    .y2 = y_offset + FACE_IMG_HEIGHT_SAMPLE_REQUIRED - 1
  };

  if (imageproc_clip_and_resize(mem, FACE_IMAGE_WIDTH, FACE_IMAGE_HEIGHT,
      clip_mem, FACE_IMG_WIDTH_SAMPLE_REQUIRED, FACE_IMG_HEIGHT_SAMPLE_REQUIRED,
      16 /*YUV422*/, (imageproc_rect_t *)&clip_rect) == 0)
      {
        printf("convert step 1.\n");
        imageproc_convert_yuv2rgb(clip_mem,
            FACE_IMG_WIDTH_SAMPLE_REQUIRED, FACE_IMAGE_HEIGHT / 2);
        printf("convert step 2.\n");
        for(int i = 0; i < FACE_IMG_DATASIZE_SAMPLE_REQUIRED; ++i){
          *(out_data + i) = *(clip_mem + i) / 128.0f - 1.0f;
        }
        printf("convert step 5.\n");
        return 0;
      }
    else
      {
        printf("imageproc_clip_and_resize() is faled.\n");
      }
  return -1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spresense_getimage()
 *
 * Description:
 *   Return the buffer with captured image.
 ****************************************************************************/

int spresense_getfaceimage(float *out_data, int type)
{
  unsigned char *mem;

  if (init_video()<0)
    {
      printf("init_video() is faled..\n");
      return -1;
    }

  if (set_camimage()!=OK)
    {
      printf("set_camimage() is faled..\n");
      return -1;
    }

  mem = get_camimage();
  int ret = -1;
  if (mem!=NULL)
    {
      printf("Captured image now!\n");
      printf("type : %d\n", type);
      switch (type)
      {
      case 0:
        ret = convert_image0(mem, out_data);
        break;
      case 1:
        ret = convert_image1(mem, out_data);
        break;
      case 2:
        ret = convert_image2(mem, out_data);
        break;
      default:
        ret = convert_image3_15(mem, out_data, type);
        break;
      }
      printf("Image captured.\n");
      return ret;
    }
  else
    {
      printf("get_camimage() is faled.\n");
    }

  return -1;
}

