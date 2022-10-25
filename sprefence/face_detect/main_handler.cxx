/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include <cstdio>

#include "app/face_detect.h"
#include "tensors_to_detections.h"
#include "non_max_suppression.h"
#include "gpio_control.h"

static float o0[896 * 16];
static float o1[896];

// The name of this function is important for Arduino compatibility.
void Setup() {
  int gpioState = GPIOInit();
  if (gpioState != 0){
    printf("GPIO initialize failed. ");
    return;
  }
  printf("gpio init finished\n");
  FaceSetup();
  printf("tf init finished\n");
  return;
}

void DeOffset(int type, Detection& detection){
  int x_offset = 0;
  int y_offset = 0;
  int scale = 1;
  if (type < 3){
    scale = 2;
  }

  if (type == 0){
    x_offset = 32;
    y_offset = 0;
  }
  else if(type == 1){
    x_offset = 0;
    y_offset = 0;
  }else if (type == 2){
    x_offset = 64;
    y_offset = 0;
  }else{
    x_offset = 64 * ((type - 3) % 4);
    y_offset = 56 * ((type - 3) / 4);
  }

  detection.x *= scale;
  detection.y *= scale;
  detection.w *= scale;
  detection.h *= scale;

  detection.x += x_offset;
  detection.y += y_offset;
  detection.w += x_offset;
  detection.h += y_offset;

  detection.x /= 320.0f;
  detection.y /= 240.0f;
  detection.w /= 320.0f;
  detection.h /= 320.0f;
}

// The name of this function is important for Arduino compatibility.
void FaceLoop(int type, float* x, float* y, float* boxSize) {
  Run(type, o0, o1);
  printf("Get data from tf.\n");

  std::vector<Detection> detected = Tensors2Detections(o0, o1);
  printf("Finish extract data.\n");
  std::vector<Detection> nms = NonMaxSuppression(detected);

  if(nms.size() > 0){
    DeOffset(type, nms[0]);
    printf("person x: %f, y: %f, score %f",
                       nms[0].x, nms[0].y, nms[0].score);
    *x = nms[0].x;
    *y = nms[0].y;
    *boxSize = nms[0].w + nms[0].h;
  }
  else
  {
    printf("Face not find.\n");
    *x = -100.0f;
    *y = -100.0f;
    *boxSize = -1.0f;
  }
}
