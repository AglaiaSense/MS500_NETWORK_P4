#ifndef INFERENCE_H
#define INFERENCE_H

// Cpp native
#include <fstream>
#include <vector>
#include <string>
#include <random>

// OpenCV / DNN / Inference
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"


// #include "../../as_common_deps/opencv/opencv2/imgproc.hpp"
// #include "../../as_common_deps/opencv/opencv2/opencv.hpp"

struct Detection
{
    int class_id{0};
    std::string className{};
    float confidence{0.0};
    cv::Scalar color{};
    cv::Rect box{};
};

class Inference
{
public:
    Inference(const cv::Size2f &modelInputShape, const std::string &classesTxtFile, const std::string &anchorTxtFile = "");
    std::vector<Detection> runInference(const void *input, std::vector<std::array<int, 3>> size);
    std::vector<std::string> classes{};
    
private:
    std::vector<cv::Mat> process_dnn_vector(std::vector<std::array<int, 3>> sizes, int data_type, const void * buffer);
    void loadClassesFromFile();
    void loadAnchorsFromFile();

    std::string modelPath{};
    std::string classesPath{};
    bool cudaEnabled{};
    std::string anchorPath{};

    cv::Size2f modelShape{};

    std::vector<std::array<float, 2>> anchors{};
    // std::vector<std::array<float, 2>> stride{{1295, 8}, {1619, 16}, {1700, 32}}; // for 288x288, these data were evaluated from DetectionValidatorReplacer @ replaces.py
    std::vector<std::array<float, 2>> stride{{2703, 8}, {3379, 16}, {3548, 32}}; // for 416x416, these data were evaluated from DetectionValidatorReplacer @ replaces.py

    float modelConfidenseThreshold {0.25};   
    float modelScoreThreshold      {0.45};   // 最低置信度
    float modelNMSThreshold        {0.50};   // NMS阈值

    bool letterBoxForSquare = true;
};

#endif // INFERENCE_H
