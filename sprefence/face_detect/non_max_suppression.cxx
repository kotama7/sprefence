#include <algorithm>
#include <utility>

#include "non_max_suppression.h"

constexpr float kMinSuppressionThreshold = 0.3;

bool SortBySecond(const std::pair<int, float> &indexed_score_0,
                  const std::pair<int, float> &indexed_score_1)
{
    return (indexed_score_0.second > indexed_score_1.second);
}

Detection IntersectRect(const Detection &rect1, const Detection &rect2)
{
    Detection ret;
    float hw0 = rect1.w / 2;
    float hw1 = rect2.w / 2;
    float x0 = rect1.x - hw0;
    float x1 = rect2.x - hw1;
    float w0 = rect1.x + hw0;
    float w1 = rect2.x + hw1;
    float hh0 = rect1.h / 2;
    float hh1 = rect2.h / 2;
    float y0 = rect1.y - hh0;
    float y1 = rect2.y - hh1;
    float h0 = rect1.y + hh0;
    float h1 = rect2.y + hh1;
    ret.x = x0 > x1 ? x0 : x1;
    ret.w = w0 < w1 ? w0 : w1;
    ret.y = y0 > y1 ? y0 : y1;
    ret.h = h0 < h1 ? h0 : h1;
    return ret;
}

float OverlapSimilarity(
    const Detection &rect1, const Detection &rect2)
{
    Detection intersect = IntersectRect(rect1, rect2);
    if (intersect.x >= intersect.w || intersect.y >= intersect.h)
        return 0.0f;
    const float intersection_area = (intersect.w - intersect.x) * (intersect.h - intersect.y);
    float normalization = rect1.w * rect1.h + rect2.w * rect2.h - intersection_area;
    return normalization > 0.0f ? intersection_area / normalization : 0.0f;
}

std::vector<Detection> NonMaxSuppression(const std::vector<Detection> &bb)
{
    if (bb.empty())
    {
        return bb;
    }

    std::vector<std::pair<int, float>> indexed_scores;
    indexed_scores.reserve(bb.size());
    for (int index = 0; index < bb.size(); ++index)
    {
        indexed_scores.push_back(
            std::make_pair(index, bb[index].score));
    }
    std::sort(indexed_scores.begin(), indexed_scores.end(), SortBySecond);

    std::vector<Detection> ret;

    std::vector<std::pair<int, float>> remained_indexed_scores;
    remained_indexed_scores.assign(indexed_scores.begin(),
                                   indexed_scores.end());

    std::vector<std::pair<int, float>> remained;
    std::vector<std::pair<int, float>> candidates;

    while (!remained_indexed_scores.empty())
    {
        const int original_indexed_scores_size = remained_indexed_scores.size();
        const auto &detection = bb[remained_indexed_scores[0].first];
        remained.clear();
        candidates.clear();

        // This includes the first box.
        for (const auto &indexed_score : remained_indexed_scores)
        {
            float similarity =
                OverlapSimilarity(bb[indexed_score.first], detection);
            if (similarity > kMinSuppressionThreshold)
            {
                candidates.push_back(indexed_score);
            }
            else
            {
                remained.push_back(indexed_score);
            }
        }
        auto weighted_detection = detection;
        if (!candidates.empty())
        {
            float w_xmin = 0.0f;
            float w_ymin = 0.0f;
            float w_xmax = 0.0f;
            float w_ymax = 0.0f;
            float total_score = 0.0f;
            for (const auto &candidate : candidates)
            {
                total_score += candidate.second;
                const auto &bbox = bb[candidate.first];
                float hw = bbox.w / 2;
                float hh = bbox.h / 2;
                w_xmin += (bbox.x - hw) * candidate.second;
                w_ymin += (bbox.y - hh) * candidate.second;
                w_xmax += (bbox.x + hw) * candidate.second;
                w_ymax += (bbox.x + hw) * candidate.second;
            }
            float x_min = w_xmin / total_score;
            float y_min = w_ymin / total_score;
            float x_max = w_xmax / total_score;
            float y_max = w_ymax / total_score;
            weighted_detection.x = (x_min + x_max) / 2;
            weighted_detection.y = (y_min + y_max) / 2;
            weighted_detection.w = x_max - x_min;
            weighted_detection.h = y_max - y_min;
        }

        ret.push_back(weighted_detection);
        // Breaks the loop if the size of indexed scores doesn't change after an
        // iteration.
        if (original_indexed_scores_size == remained.size())
        {
            break;
        }
        else
        {
            remained_indexed_scores = std::move(remained);
        }
    }
    return ret;
}
