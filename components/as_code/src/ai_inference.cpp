#include "ai_inference.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "as_tools.h"


#ifdef __cplusplus
extern "C" {
#endif

std::string classPath = "/spiffs/dnn/classes.txt";
std::string anchorPath = "/spiffs/dnn/anchors.txt";

// 宏定义
#if 0
#define JUDGE(x) (x < 0 ? 0 : (x > 288 ? 288 : x))
#define RATIOX(x) (int(x * 4056 / 288))
#define RATIOY(y) (int(y * 3040 / 288))
#endif

#if 1
#define JUDGE(x) (x < 0 ? 0 : (x > 416 ? 416 : x))
#define RATIOX(x) (int(x * 4056 / 416))
#define RATIOY(y) (int(y * 3040 / 416))
#endif


#if 1
#define JX(x) (x < 0 ? 0 : (x > 1920 ? 1920 : x))
#define JY(y) (y < 0 ? 0 : (y > 1080 ? 1080 : y))
#define RX(x) (int(x * 2028 / 4056))
#define RY(y) (int(y * 1520 / 3040))
#define OFFSET_X (54)
#define OFFSET_Y (220)


#endif


extern int imx501_set_model(int model_id);
extern int imx501_set_roi(int x, int y, int w, int h);


extern void NMSBoxes(const std::vector<cv::Rect> &bboxes, const std::vector<float> &scores,
                     const float score_threshold, const float nms_threshold,
                     std::vector<int> &indices, const float eta, const int top_k);

Inference::Inference(const cv::Size2f &modelInputShape, const std::string &classesTxtFile, const std::string &anchorTxtFile) {
    modelShape = modelInputShape;
    classesPath = classesTxtFile;
    anchorPath = anchorTxtFile;

    loadClassesFromFile();
    loadAnchorsFromFile();

    // assert(anchors.size() == 1701); // for 288x288 size
    assert(anchors.size() == 3549); // for 416x416 size
}

std::vector<cv::Mat> Inference::process_dnn_vector(std::vector<std::array<int, 3>> sizes, int data_type, const void *buffer) {
    // Create an empty vector of cv::Mat
    std::vector<cv::Mat> vec;
    uint8_t *buf_in = (uint8_t *)buffer;

    for (int i = 0; i < (int)sizes.size(); i++) {
        // Create a single cv::Mat of size 84x2100, with data type CV_32FC1 (float)
        int size[] = {sizes[i][0], sizes[i][1], sizes[i][2]};
        cv::Mat mat(3, size, CV_32FC1);

        int data_size = 1;

        if (data_type == CV_32FC1)
            data_size = 4;
        else if (data_type == CV_8SC1 or data_type == CV_8UC1)
            data_size = 1;

        // Copy data from buffer to the 3D cv::Mat
        memcpy(mat.data, buf_in, sizes[i][0] * sizes[i][1] * sizes[i][2] * data_size);
        buf_in += sizes[i][0] * sizes[i][1] * sizes[i][2] * data_size;

        // Create a float32 Mat with the same dimensions as int8_mat
        cv::Mat float32_mat;

        // Add the cv::Mat to the vector
        if (data_type != CV_32FC1) {
            mat.convertTo(float32_mat, CV_32FC1);
            vec.push_back(float32_mat);
        } else
            vec.push_back(mat);
    }
    // 0 is score, 1 is coordinators
    std::vector<cv::Mat> vec_ret = {vec[1], vec[0]};
    return vec_ret;
}

std::vector<Detection> Inference::runInference(const void *input, std::vector<std::array<int, 3>> sizes) {
    std::vector<Detection> detections{};

    // CV_32FC1, CV_8SC1
    std::vector<cv::Mat> outputs = process_dnn_vector(sizes, CV_32FC1, (void *)input);

    int rows = outputs[0].size[1];
    int dimensions = outputs[0].size[2];
    int dimension_score = 0;

    bool yolov8_mct = false;
    bool yolov8 = false;
    // yolov5 has an output of shape (batchSize, 25200, 85) (Num classes + box[x,y,w,h] + confidence[c])
    // yolov8 has an output of shape (batchSize, 84,  8400) (Num classes + box[x,y,w,h])
    // yolov8_mct has two outputs of shape (batchSize, 4, 1701) and (batchSize, 3, 1701)
    if (outputs.size() == 2) {
        yolov8_mct = true;
        rows = outputs[1].size[2];
        dimensions = outputs[1].size[1];

        outputs[1] = outputs[1].reshape(1, dimensions);
        // cv::transpose(outputs[1], outputs[1]); // It seems sdsp always transposes the vectors .....

        rows = outputs[0].size[2];
        dimension_score = outputs[0].size[1];
        outputs[0] = outputs[0].reshape(1, dimension_score);
        // cv::transpose(outputs[0], outputs[0]); // It seems sdsp always transposes the vectors .....
    } else if (dimensions > rows) // Check if the shape[2] is more than shape[1] (yolov8)
    {
        yolov8 = true;
        rows = outputs[0].size[2];
        dimensions = outputs[0].size[1];

        outputs[0] = outputs[0].reshape(1, dimensions);
        cv::transpose(outputs[0], outputs[0]);
    }
    float *data = (float *)outputs[0].data;
    float *data_score = NULL;

    if (yolov8_mct) {
        data_score = (float *)outputs[0].data;
        data = (float *)outputs[1].data;
    }

    float x_factor = 1.0; // modelShape.width;
    float y_factor = 1.0; // modelShape.height;
    // printf("data: %f\n",*data);
    // printf("data_score: %f\n",*data_score);
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    for (int i = 0; i < rows; ++i) {
        if (yolov8 | yolov8_mct) {
            float *classes_scores = data + 4;
            if (yolov8_mct) {
                classes_scores = data_score;
            }

            cv::Mat scores(1, classes.size(), CV_32FC1, classes_scores);
            cv::Point class_id;
            // printf("Dim: %d, data[0]: %d\n", scores.dims, *scores.datalimit);
            // printf("class_id: %d\n",class_id);
            double maxClassScore;

            minMaxLoc(scores, 0, &maxClassScore, 0, &class_id);
            if (maxClassScore > modelScoreThreshold) {
                confidences.push_back(maxClassScore);
                class_ids.push_back(class_id.x);

                float x = data[0];
                float y = data[1];
                float w = data[2];
                float h = data[3];

                if (yolov8_mct) {
                    float mul = (i <= stride[0][0]) ? stride[0][1] : (i <= stride[1][0]) ? stride[1][1]
                                                                                         : stride[2][1];
                    float x1 = x, y1 = y, x2 = w, y2 = h;
                    x1 = anchors[i][0] - x1;
                    y1 = anchors[i][1] - y1;
                    x2 = anchors[i][0] + x2;
                    y2 = anchors[i][1] + y2;

                    x = (x1 + x2) / 2 * mul;
                    y = (y1 + y2) / 2 * mul;
                    w = (x2 - x1) * mul;
                    h = (y2 - y1) * mul;
                }

                int left = int((x - 0.5 * w) * x_factor);
                int top = int((y - 0.5 * h) * y_factor);

                int width = int(w * x_factor);
                int height = int(h * y_factor);
                // int width = int((x + 0.5 * w) * x_factor);
                // int height = int((y + 0.5 * h) * y_factor);

                boxes.push_back(cv::Rect(left, top, width, height));
            }
        } else // yolov5
        {
            float confidence = data[4];

            if (confidence >= modelConfidenseThreshold) {
                float *classes_scores = data + 5;

                cv::Mat scores(1, classes.size(), CV_32FC1, classes_scores);
                cv::Point class_id;
                double max_class_score;

                minMaxLoc(scores, 0, &max_class_score, 0, &class_id);

                if (max_class_score > modelScoreThreshold) {
                    confidences.push_back(confidence);
                    class_ids.push_back(class_id.x);

                    float x = data[0];
                    float y = data[1];
                    float w = data[2];
                    float h = data[3];

                    int left = int((x - 0.5 * w) * x_factor);
                    int top = int((y - 0.5 * h) * y_factor);

                    int width = int(w * x_factor);
                    int height = int(h * y_factor);

                    boxes.push_back(cv::Rect(left, top, width, height));
                }
            }
        }

        data += dimensions;
        if (yolov8_mct) {
            data_score += dimension_score;
        }
    }

    std::vector<int> nms_result;
    NMSBoxes(boxes, confidences, modelScoreThreshold, modelNMSThreshold, nms_result, 1.f, 0);

    // printf("####nms size %d\n", nms_result.size());

    int loop = nms_result.size();
    if (nms_result.size() > 1)
        loop = 1;

    for (size_t i = 0; i < loop /*nms_result.size()*/; ++i) {
        int idx = nms_result[i];

        Detection result;
        result.class_id = class_ids[idx];
        result.confidence = confidences[idx];

        // std::random_device rd;
        // std::mt19937 gen(rd());
        // std::uniform_int_distribution<int> dis(100, 255);
        // result.color = cv::Scalar(dis(gen),
        //                           dis(gen),
        //                           dis(gen));

        result.className = classes[result.class_id];
        result.box = boxes[idx];

        detections.push_back(result);
    }

    return detections;
}

void Inference::loadClassesFromFile() {
    std::ifstream inputFile(classesPath);
    if (inputFile.is_open()) {
        std::string classLine;
        while (std::getline(inputFile, classLine))
            classes.push_back(classLine);
        inputFile.close();
    }
}

void Inference::loadAnchorsFromFile() {
    std::array<float, 2> xy;
    std::ifstream inputFile(anchorPath);
    if (inputFile.is_open()) {
        std::string anchorLine;
        while (std::getline(inputFile, anchorLine)) {
            xy[0] = std::stof(anchorLine.substr(0, anchorLine.find(" ")));
            xy[1] = std::stof(anchorLine.substr(anchorLine.find(" ")));
            anchors.push_back(xy);
        }
        inputFile.close();
    }
}

int decode_yolo(const void *data) {

    // Inference yolo_inf(cv::Size(288, 288), classPath, anchorPath);
    Inference yolo_inf(cv::Size(416, 416), classPath, anchorPath);

    static int cnt = 0;
    if (cnt < 2) {
        cnt++;
        // 跳过第一次车牌检测
        return 0;
    }

    // int size[] = {1, 1701, 84}; // TODO: yolov8 should be {1, 84, 1701} (for 288x288), int8, not sure why it is transposed ....
    // std::vector<std::array<int, 3>> sizes = {{1, 4, 1701}, {1, 2, 1701}};     // 288x288
    std::vector<std::array<int, 3>> sizes = {{1, 4, 3549}, {1, 2, 3549}}; // 416x416

    std::vector<Detection> detection = yolo_inf.runInference(data, sizes);
    // printf("found %d objects\n", detection.size());

    for (Detection det : detection) {
#if 0
        BoundingBox bb;
        bb.x = det.box.x;
        bb.y = det.box.y;
        bb.width = det.box.width;
        bb.height = det.box.height;
        bb.name = det.className.c_str();
        bb.index = 0;

        bounding_boxes.push_back(bb);
#endif
        // printf("[INFERENCD] [x:%d  y:%d  w:%d  h:%d  name:%s]\n", det.box.x, det.box.y,
        //                             det.box.width, det.box.height, det.className.c_str());

#if 1
        // 如果连续检查车牌4次，切换模型并切换roi
        if (cnt == 3) {
            // 切换模型
            dnn_model_index = 1;
            imx501_set_model(dnn_model_index);
            // 切换ROI
            // int x = det.box.x - 8;
            // int y = det.box.y - 4;
            // int w = det.box.width + 12;
            // int h = det.box.height + 14;
            int x = det.box.x;
            int y = det.box.y;
            int w = det.box.width;
            int h = det.box.height;
            x = JUDGE(x);
            y = JUDGE(y);
            w = JUDGE(w);
            h = JUDGE(h * 4 / 3);
            x = RATIOX(x);
            y = RATIOY(y);
            w = RATIOX(w);
            h = RATIOY(h);

            // printf("roi: x %d, y %d, w %d, h %d\n", x, y, w, h);
            imx501_set_roi(x, y, w, h);

            cnt = 0;

            plate_result.x = JX(RX(x) - OFFSET_X);
            plate_result.y = JY(RY(y) - OFFSET_Y);
            plate_result.w = RX(w);
            plate_result.h = RY(h);
            plate_result.score = det.confidence;

        }
        cnt++;
#endif
    }

    if (detection.size() == 0)
        cnt = 0;

    return 0;
}

int decode_plate(const void *buf) {

    // char plate_chr[] = "#京沪津渝冀晋蒙辽吉黑苏浙皖闽赣鲁豫鄂湘粤桂琼川贵云藏陕甘青宁新学警港澳挂使领民航危0123456789ABCDEFGHJKLMNPQRSTUVWXYZ险品";
    char plate_chr[] = "#0123456789ABCDEFGHJKLMNPQRSTUVWXYZ"; // taiwan车牌字符集

 
    static int cnt = 0;
    if (cnt < 2) {
        cnt++;
        // 跳过第一次车牌识别
        return 0;
    }

    // 维度从78x21转为21x78 国内车牌
    // 维度从35x21转为21x35 台湾车牌
    char *buffer = (char *)buf;
    int file_size = 4 * 21 * 35;
    // 分配内存
    float *buffer1 = (float *)malloc(file_size);
    if (buffer1 == NULL) {
        printf("malloc error\n");
        return -1;
    }
    for (int i = 0; i < 21; i++) {
        float *ptr = (float *)buffer + i;
        for (int j = 0; j < 35; j++)
            buffer1[j + i * 35] = *(ptr + 21 * j);
    }

    free(buffer1);
    memcpy(buffer, buffer1, file_size);
 

#if 0
    for(int i = 0; i < sizeof(plate_chr); i++) {
        printf("%c", plate_chr[i]);
    }
    printf("\n");
#endif

    int array_index[21];
    float *src = (float *)buffer;

    for (int i = 0; i < 21; i++) {
        float *ptr = src + 35 * i;
        int index = 0;
        for (int j = 0; j < 35; j++) {
            // printf("%f ", *(ptr + j));
            if (*(ptr + j) > *(ptr + index))
                index = j;
        }

        array_index[i] = index;
    }

#if 0
    for (int i = 0; i < 21; i++)
        printf("%02d ", array_index[i]);
       
    printf("\n");
#endif

    int digits = 0;
    int plate_str[21] = {0};

    int pre_str = 0;
    int ct = 0;
    for (int i = 0; i < 21; i++) {
        if (array_index[i] != 0 ) {
            if (array_index[i] != pre_str) {
                plate_str[digits] = array_index[i];
                digits++;
                ct = 0;
            } else {
                ct++;
                if (ct == 2) {
                    plate_str[digits] = array_index[i];
                    digits++;
                    ct = 0;
                }
            }
        }
        pre_str = array_index[i];
    }
 

    if (cnt == 4) {
        memset(license_plate, '0', sizeof(license_plate));
        // 将整数转换为字符并赋值给字符数组的第一个元素
        license_plate[0] = '0' + digits;

        printf("License plate : ");
        for (int i = 0; i < digits; i++) {
            // 多次检查到车牌再输出
            if (plate_str[i] >= 1 && plate_str[i] <= 34) {
                int index = plate_str[i];
                license_plate[i + 1] = plate_chr[index];
                printf("%c", plate_chr[index]);
            }
        }
        memcpy(plate_result.plate, license_plate + 1, digits);
        plate_result.plate[digits] = '\0'; // 确保字符串以空字符结尾

        printf("\r\n");
        is_plate_detection = true;       // 设置车牌检测标志

        dnn_model_index = 0;
        imx501_set_model(dnn_model_index);
        // 切换ROI
        imx501_set_roi(0, 0, 4056, 3040);

        cnt = 0;

        // vTaskDelay(100 / portTICK_PERIOD_MS);
 
    }
    cnt++;

    return 0;
}



#ifdef __cplusplus
}
#endif
