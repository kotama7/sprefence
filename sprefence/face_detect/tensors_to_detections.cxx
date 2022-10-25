#include <unordered_map>
#include <vector>

#include "tensors_to_detections.h"

constexpr int kNumCoordsPerBox = 4;
constexpr int kNumBoxes = 896;
constexpr int kNumCoords = 16;
constexpr int kBoxCoordOffset = 0;
constexpr float kXScale = 128.0;
constexpr float kYScale = 128.0;
constexpr float kHScale = 128.0;
constexpr float kWScale = 128.0;
constexpr int kNumKeypoints = 6;
constexpr int kKeypointCoordOffset = 4;
constexpr int kNumValuesPerKeypoint = 2;
constexpr int kNumClasses = 1;
constexpr float kMinScoreThresh = 0.75;

void DecodeBoxes(
    const float *raw_boxes, const std::vector<Detection> &anchors,
    std::vector<float> *boxes)
{
    for (int i = 0; i < kNumBoxes; ++i)
    {
        const int box_offset = i * kNumCoords + kBoxCoordOffset;

        float y_center = raw_boxes[box_offset];
        float x_center = raw_boxes[box_offset + 1];
        float h = raw_boxes[box_offset + 2];
        float w = raw_boxes[box_offset + 3];
        x_center = raw_boxes[box_offset];
        y_center = raw_boxes[box_offset + 1];
        w = raw_boxes[box_offset + 2];
        h = raw_boxes[box_offset + 3];

        x_center =
            x_center / kXScale * anchors[i].w + anchors[i].x;
        y_center =
            y_center / kYScale * anchors[i].h + anchors[i].y;

        h = h / kHScale * anchors[i].h;
        w = w / kWScale * anchors[i].w;

        const float ymin = y_center - h / 2.f;
        const float xmin = x_center - w / 2.f;
        const float ymax = y_center + h / 2.f;
        const float xmax = x_center + w / 2.f;

        (*boxes)[i * kNumCoords + 0] = ymin;
        (*boxes)[i * kNumCoords + 1] = xmin;
        (*boxes)[i * kNumCoords + 2] = ymax;
        (*boxes)[i * kNumCoords + 3] = xmax;

        for (int k = 0; k < kNumKeypoints; ++k)
        {
            const int offset = i * kNumCoords + kKeypointCoordOffset +
                               k * kNumValuesPerKeypoint;

            float keypoint_y = raw_boxes[offset];
            float keypoint_x = raw_boxes[offset + 1];
            keypoint_x = raw_boxes[offset];
            keypoint_y = raw_boxes[offset + 1];

            (*boxes)[offset] = keypoint_x / kXScale * anchors[i].w +
                               anchors[i].x;
            (*boxes)[offset + 1] =
                keypoint_y / kYScale * anchors[i].h +
                anchors[i].y;
        }
    }
}

Detection ConvertToDetection(
    float box_ymin, float box_xmin, float box_ymax, float box_xmax, float score,
    int class_id)
{
    Detection detection;
    detection.score = score;
    detection.classes = class_id;
    detection.x = (box_xmin + box_xmax) / 2;
    detection.y = (box_ymin + box_ymax) / 2;
    detection.w = box_xmax - box_xmin;
    detection.h = box_ymax - box_ymin;
    return detection;
}

void ConvertToDetections(
    const float *detection_boxes, const float *detection_scores,
    const int *detection_classes, std::vector<Detection> &output_detections)
{
    for (int i = 0; i < kNumBoxes; ++i)
    {
        if (detection_scores[i] < kMinScoreThresh)
        {
            continue;
        }
        const int box_offset = i * kNumCoords;
        Detection detection = ConvertToDetection(
            detection_boxes[box_offset + 0], detection_boxes[box_offset + 1],
            detection_boxes[box_offset + 2], detection_boxes[box_offset + 3],
            detection_scores[i], detection_classes[i]);
        if (detection.w < 0 || detection.h < 0)
        {
            // Decoded detection boxes could have negative values for width/height due
            // to model prediction. Filter out those boxes since some downstream
            // calculators may assume non-negative values. (b/171391719)
            continue;
        }
        output_detections.emplace_back(detection);
    }
}

// output0 : raw_boxes, output1 : raw_scores
std::vector<Detection> Tensors2Detections(float *output0, float *output1)
{
    std::vector<float> boxes(kNumBoxes * kNumCoords);
    DecodeBoxes(output0, ssdAnchors, &boxes);
    std::vector<float> detection_scores(kNumBoxes);
    std::vector<int> detection_classes(kNumBoxes);

    // Filter classes by scores.
    for (int i = 0; i < kNumBoxes; ++i)
    {
        int class_id = -1;
        float max_score = -std::numeric_limits<float>::max();
        // Find the top score for box i.
        auto score = output1[i * kNumClasses];
        if (max_score < score)
        {
            max_score = score;
            class_id = 0;
        }
        detection_scores[i] = max_score;
        detection_classes[i] = class_id;
    }

    std::vector<Detection> output_detections;
    ConvertToDetections(boxes.data(), detection_scores.data(),
                        detection_classes.data(), output_detections);
    return output_detections;
}