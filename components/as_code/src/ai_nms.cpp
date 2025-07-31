// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
//
// Copyright (C) 2017, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.


#include "ai_nms.hpp"
// #include <opencv2/imgproc.hpp>


template <typename T>
static inline float rectOverlap(const T& a, const T& b)
{
    return 1.f - static_cast<float>(jaccardDistance(a, b));
}

#ifdef __cplusplus
extern "C" {
#endif

void NMSBoxes(const std::vector<cv::Rect>& bboxes, const std::vector<float>& scores,
                          const float score_threshold, const float nms_threshold,
                          std::vector<int>& indices, const float eta, const int top_k)
{
    CV_Assert_N(bboxes.size() == scores.size(), score_threshold >= 0,
        nms_threshold >= 0, eta > 0);

    NMSFast_(bboxes, scores, score_threshold, nms_threshold, eta, top_k, indices, rectOverlap);
}

#ifdef __cplusplus
}
#endif

