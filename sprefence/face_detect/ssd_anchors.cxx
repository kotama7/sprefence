#include "ssd_anchors.h"
#include <cmath>

constexpr int kNumLayers = 4;
constexpr int kStridesSize = 4;
constexpr int kStrides[] = {8, 16, 16, 16};
constexpr float kMinScale = 0.1484375;
constexpr float kMaxScale = 0.75;
constexpr bool kRreduceBoxesInLowestLayer = false;
constexpr float kAspectRatio = 1.0;
constexpr float kInterpolatedScaleAspectRatio = 1.0;
constexpr int kInputSizeWidth = 128;
constexpr int kInputSizeHeight = 128;
constexpr float kAnchorOffsetX = 0.5;
constexpr float kAnchorOffsetY = 0.5;

float CalculateScale(float min_scale, float max_scale, int stride_index,
                     int num_strides)
{
    if (num_strides == 1)
    {
        return (min_scale + max_scale) * 0.5f;
    }
    else
    {
        return min_scale +
               (max_scale - min_scale) * 1.0 * stride_index / (num_strides - 1.0f);
    }
}

std::vector<Detection> GenerateSSDAnchors()
{
    std::vector<Detection> ret;
    int layer_id = 0;
    while (layer_id < kNumLayers)
    {
        std::vector<float> anchor_height;
        std::vector<float> anchor_width;
        std::vector<float> aspect_ratios;
        std::vector<float> scales;

        // For same strides, we merge the anchors in the same order.
        int last_same_stride_layer = layer_id;
        while (last_same_stride_layer < kStridesSize &&
               kStrides[last_same_stride_layer] ==
                   kStrides[layer_id])
        {
            const float scale =
                CalculateScale(kMinScale, kMaxScale,
                               last_same_stride_layer, kStridesSize);

            aspect_ratios.push_back(kAspectRatio);
            scales.push_back(scale);
            const float scale_next =
                last_same_stride_layer == kStridesSize - 1
                    ? 1.0f
                    : CalculateScale(kMinScale, kMaxScale,
                                     last_same_stride_layer + 1,
                                     kStridesSize);
            scales.push_back(std::sqrt(scale * scale_next));
            aspect_ratios.push_back(kInterpolatedScaleAspectRatio);

            last_same_stride_layer++;
        }

        for (int i = 0; i < aspect_ratios.size(); ++i)
        {
            const float ratio_sqrts = std::sqrt(aspect_ratios[i]);
            anchor_height.push_back(scales[i] / ratio_sqrts);
            anchor_width.push_back(scales[i] * ratio_sqrts);
        }

        int feature_map_height = 0;
        int feature_map_width = 0;

        const int stride = kStrides[layer_id];
        feature_map_height =
            std::ceil(1.0f * kInputSizeHeight / stride);
        feature_map_width = std::ceil(1.0f * kInputSizeWidth / stride);

        for (int y = 0; y < feature_map_height; ++y)
        {
            for (int x = 0; x < feature_map_width; ++x)
            {
                for (int anchor_id = 0; anchor_id < anchor_height.size(); ++anchor_id)
                {
                    // TODO: Support specifying anchor_offset_x, anchor_offset_y.
                    const float x_center =
                        (x + kAnchorOffsetX) * 1.0f / feature_map_width;
                    const float y_center =
                        (y + kAnchorOffsetY) * 1.0f / feature_map_height;

                    Detection new_anchor;
                    new_anchor.x = x_center;
                    new_anchor.y = y_center;

                    new_anchor.w = 1.0f;
                    new_anchor.h = 1.0f;

                    ret.push_back(new_anchor);
                }
            }
        }
        layer_id = last_same_stride_layer;
    }
    return ret;
}

std::vector<Detection> ssdAnchors = GenerateSSDAnchors();