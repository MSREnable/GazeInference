#pragma once
#include "framework.h"
#include "Model.h"
#include "LiveCapture.h"

enum NMS_TYPE { HARD, BLENDING };

typedef struct FaceInfo {
    float x1;
    float y1;
    float x2;
    float y2;
    float score;

    float* landmarks;//TODO Remove unsed member
} FaceInfo;

#define clip(x, y) (x < 0 ? 0 : (x > y ? y : x))

// SqueezeNet Model
class UltraFaceNet : public Model
{
private:
    cv::Mat frame;
    std::tuple<std::string, float> result = std::tuple<std::string, float>("Unknown", 0.0f);
    std::unique_ptr<LiveCapture> live_capture;

    int num_featuremap = 4;
    int image_w;
    int image_h;
    int in_w = 320;
    int in_h = 240;
    int num_anchors;
    int topk = -1;
    float score_threshold = 0.7;
    float iou_threshold = 0.3;
    int nms_type = NMS_TYPE::BLENDING;//NMS_TYPE::HARD

    const float center_variance = 0.1;
    const float size_variance = 0.2;
    const std::vector<std::vector<float>> min_boxes = {
            {10.0f,  16.0f,  24.0f},
            {32.0f,  48.0f},
            {64.0f,  96.0f},
            {128.0f, 192.0f, 256.0f} };
    const std::vector<float> strides = { 8.0, 16.0, 32.0, 64.0 };
    std::vector<std::vector<float>> featuremap_size;
    std::vector<std::vector<float>> shrinkage_size;
    std::vector<int> w_h_list;
    std::vector<std::vector<float>> priors = {};

public:
    UltraFaceNet(const wchar_t* modelFilePath)
        : Model{ modelFilePath }
    {
        w_h_list = { in_w, in_h };
        for (auto size : w_h_list) {
            std::vector<float> fm_item;
            for (float stride : strides) {
                fm_item.push_back(ceil(size / stride));
            }
            featuremap_size.push_back(fm_item);
        }

        for (auto size : w_h_list) {
            shrinkage_size.push_back(strides);
        }

        /* generate prior anchors */
        for (int index = 0; index < num_featuremap; index++) {
            float scale_w = in_w / shrinkage_size[0][index];
            float scale_h = in_h / shrinkage_size[1][index];
            for (int j = 0; j < featuremap_size[1][index]; j++) {
                for (int i = 0; i < featuremap_size[0][index]; i++) {
                    float x_center = (i + 0.5) / scale_w;
                    float y_center = (j + 0.5) / scale_h;

                    for (float k : min_boxes[index]) {
                        float w = k / in_w;
                        float h = k / in_h;
                        priors.push_back({ clip(x_center, 1), clip(y_center, 1), clip(w, 1), clip(h, 1) });
                    }
                }
            }
        }
        num_anchors = priors.size();
        /* generate prior anchors finished */
    }

    ~UltraFaceNet() {
        // Cleanup 
    }

    bool initCamera() {
        live_capture = std::make_unique<LiveCapture>(0, 30, cv::Size(640, 480));
        live_capture->open();
        return live_capture->is_open();
    }

    bool getFrameFromImagePath(std::string imageFilepath) {
        return live_capture->getFrameFromImagePath(imageFilepath, frame);
    }

    bool getFrame() {
        return live_capture->getFrame(frame);
    }

    void setFrame(cv::Mat& input_frame) {
        frame = input_frame;
    }

    void showRawInput() {
        live_capture->showRawInput(frame);
    }

    bool applyTransformations() {
        //Normalize, Resize, HWC to CHW and BGR to RGB
        cv::Mat preprocessedImage = cv::dnn::blobFromImage(frame, 1.0 / 128, cv::Size(in_w, in_h), cv::Scalar(127, 127, 127), true);

        // TODO: Make it faster by initing the preprocessedFrames and reusing
        preprocessedFrames.clear();
        preprocessedFrames.push_back(preprocessedImage);

        return true;
    }

    cv::Mat draw_rectangles(cv::Mat webcamImage, std::vector<FaceInfo> rectangles) {
        cv::Mat image = webcamImage.clone();
        cv::Scalar color = cv::Scalar(0, 0, 255);
        for (auto& rect : rectangles) {
            cv::Point pt1(rect.x1, rect.y1);
            cv::Point pt2(rect.x2, rect.y2);
            cv::rectangle(image, pt1, pt2, color, 2);
        }
        return image;
    }

    void show_image(cv::String windowTitle, cv::Mat image) {
        cv::imshow(windowTitle, image);
        cv::waitKey(1);
    }

    std::vector<FaceInfo> processOutput(BOOL displayResults) {
        std::vector<float> confidence_scores = outputs[0].values;
        std::vector<float> location_boxes = outputs[1].values;
        std::vector<FaceInfo> bbox_collection;
        std::vector<FaceInfo> face_list;

        image_w = frame.size().width;
        image_h = frame.size().height;
        generateBBox(bbox_collection, confidence_scores, location_boxes, score_threshold, num_anchors);
        nms(bbox_collection, face_list, nms_type);
        if (displayResults)
            printOutput(face_list);
        return face_list;
    }

    void generateBBox(std::vector<FaceInfo>& bbox_collection, std::vector<float> scores, std::vector<float> boxes, float score_threshold, int num_anchors) {
        for (int i = 0; i < num_anchors; i++) {
            if (scores[i * 2 + 1] > score_threshold) {
                FaceInfo rect;
                float x_center = boxes[i * 4 + 0] * center_variance * priors[i][2] + priors[i][0];
                float y_center = boxes[i * 4 + 1] * center_variance * priors[i][3] + priors[i][1];
                float w =    exp(boxes[i * 4 + 2] * size_variance) * priors[i][2];
                float h =    exp(boxes[i * 4 + 3] * size_variance) * priors[i][3];

                rect.x1 = clip(x_center - w / 2.0, 1) * image_w;
                rect.y1 = clip(y_center - h / 2.0, 1) * image_h;
                rect.x2 = clip(x_center + w / 2.0, 1) * image_w;
                rect.y2 = clip(y_center + h / 2.0, 1) * image_h;
                rect.score = clip(scores[i * 2 + 1], 1);
                bbox_collection.push_back(rect);
            }
        }
    }

    void nms(std::vector<FaceInfo>& input, std::vector<FaceInfo>& output, int type) {
        // sort the bounding boxes by their confidence scores
        std::sort(input.begin(), input.end(), [](const FaceInfo& a, const FaceInfo& b) { return a.score > b.score; });

        int box_num = input.size();

        std::vector<int> merged(box_num, 0);

        for (int i = 0; i < box_num; i++) {
            if (merged[i])
                continue;
            std::vector<FaceInfo> buf;

            buf.push_back(input[i]);
            merged[i] = 1;

            float h0 = input[i].y2 - input[i].y1 + 1;
            float w0 = input[i].x2 - input[i].x1 + 1;

            float area0 = h0 * w0;

            for (int j = i + 1; j < box_num; j++) {
                if (merged[j])
                    continue;

                float inner_x0 = input[i].x1 > input[j].x1 ? input[i].x1 : input[j].x1;
                float inner_y0 = input[i].y1 > input[j].y1 ? input[i].y1 : input[j].y1;

                float inner_x1 = input[i].x2 < input[j].x2 ? input[i].x2 : input[j].x2;
                float inner_y1 = input[i].y2 < input[j].y2 ? input[i].y2 : input[j].y2;

                float inner_h = inner_y1 - inner_y0 + 1;
                float inner_w = inner_x1 - inner_x0 + 1;

                if (inner_h <= 0 || inner_w <= 0)
                    continue;

                float inner_area = inner_h * inner_w;

                float h1 = input[j].y2 - input[j].y1 + 1;
                float w1 = input[j].x2 - input[j].x1 + 1;

                float area1 = h1 * w1;

                float score;

                score = inner_area / (area0 + area1 - inner_area);

                if (score > iou_threshold) {
                    merged[j] = 1;
                    buf.push_back(input[j]);
                }
            }
            switch (type) {
            case NMS_TYPE::HARD: {
                output.push_back(buf[0]);
                break;
            }
            case NMS_TYPE::BLENDING: {
                float total = 0;
                for (int i = 0; i < buf.size(); i++) {
                    total += exp(buf[i].score);
                }
                FaceInfo rects;
                memset(&rects, 0, sizeof(rects));
                for (int i = 0; i < buf.size(); i++) {
                    float rate = exp(buf[i].score) / total;
                    rects.x1 += buf[i].x1 * rate;
                    rects.y1 += buf[i].y1 * rate;
                    rects.x2 += buf[i].x2 * rate;
                    rects.y2 += buf[i].y2 * rate;
                    rects.score += buf[i].score * rate;
                }
                output.push_back(rects);
                break;
            }
            default: {
                printf("wrong type of nms.");
                exit(-1);
            }
            }
        }
    }
    
    void printOutput(std::vector<FaceInfo> face_list) {
        LOG_DEBUG("NumFacesDetected:%d\n", face_list.size());
        show_image("Rectangles", draw_rectangles(frame, face_list));
    }

    std::vector<dlib::rectangle> detect_faces(cv::Mat& input_frame) {
        setFrame(input_frame);
        applyTransformations();
        fillInputTensor();
        run();
        std::vector<FaceInfo> faces = processOutput(false);
        std::vector<dlib::rectangle> face_rectangles;
        for (auto& face : faces) {
            dlib::point pt1(face.x1, face.y1);
            dlib::point pt2(face.x2, face.y2);
            face_rectangles.push_back(dlib::rectangle(pt1, pt2));
        }
        return face_rectangles;
    }

};

