#ifndef SSD_ANCHORS_
#define SSD_ANCHORS_

#include <vector>

struct Detection
{
    float x; // x center
    float y; // y center
    float w;
    float h;
    float score;
    int classes;
};

std::vector<Detection> GenerateSSDAnchors(void);

extern std::vector<Detection> ssdAnchors;

#endif