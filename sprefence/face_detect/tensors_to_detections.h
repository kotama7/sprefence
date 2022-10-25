#ifndef TENSORS_TO_DETECTIONS_
#define TENSORS_TO_DETECTIONS_

#include "ssd_anchors.h"

std::vector<Detection> Tensors2Detections(float* output0, float* output1);

#endif