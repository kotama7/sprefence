#ifndef NON_MAX_SUPPRESSION
#define NON_MAX_SUPPRESSION

#include "ssd_anchors.h"

std::vector<Detection> NonMaxSuppression(const std::vector<Detection>& bb);

#endif