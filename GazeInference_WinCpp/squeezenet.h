#pragma once
#include "framework.h"
#include "Model.h"
#include "LiveCapture.h"


// SqueezeNet Model
class SqueezeNet : public Model
{
private:
    cv::Mat frame;
    std::tuple<std::string, float> result = std::tuple<std::string, float>("Unknown", 0.0f);
    std::unique_ptr<LiveCapture> live_capture;
    std::vector<std::string> labels;
public:
    SqueezeNet(const wchar_t* modelFilePath, const wchar_t* labelFilepath)
        : Model{ modelFilePath }
    {
        // Load labels from filepath into a vector
        // TODO: Pass customLoadLabel func
        if (labelFilepath) {
            USES_CONVERSION;
            labels = loadLabels(std::string(W2A(labelFilepath)));
        }
    }

    ~SqueezeNet() {
        // Cleanup 
    }

    bool init() {
        live_capture = std::make_unique<LiveCapture>(0, 30, cv::Size(640, 480));
        live_capture->open();
        return live_capture->is_open();
    }

    std::vector<std::string> loadLabels(std::string labelFilepath)
    {
        std::vector<std::string> labels;
        // Parse labels from labels file.  We know the file's entries are already sorted in order.
        std::ifstream labelFile{ labelFilepath, std::ifstream::in };
        if (labelFile.fail())
        {
            LOG_ERROR("failed to load the %s file.  Make sure it exists in the same folder as the app\r\n", labelFilepath.c_str());
            exit(EXIT_FAILURE);
        }

        std::string line;
        while (std::getline(labelFile, line, ','))
        {
            int labelValue = atoi(line.c_str());
            if (labelValue >= labels.size())
            {
                labels.resize(labelValue + 1);
            }
            std::getline(labelFile, line);
            labels[labelValue] = line;
        }
        return labels;
    }

    bool getFrameFromImagePath(std::string imageFilepath) {
        return live_capture->getFrameFromImagePath(imageFilepath, frame);
    }

    bool getCameraFrame() {
        return live_capture->getFrame(frame);
    }

    void showRawInput() {
        live_capture->showRawInput(frame);
    }

    bool applyTransformations() {
        cv::Mat resizedImage, preprocessedImage;
        cv::resize(frame, resizedImage,
            cv::Size(224, 224),
            cv::InterpolationFlags::INTER_CUBIC);

        // HWC to CHW
        cv::dnn::blobFromImage(resizedImage, preprocessedImage);
         
        // TODO: Make it faster by initing the preprocessedFrames and reusing
        preprocessedFrames.clear();
        preprocessedFrames.push_back(preprocessedImage);

        return true;
    }

    //    void applyTransformations(std::string imageFilepath) {

    //    cv::Mat imageBGR = cv::imread(imageFilepath, cv::ImreadModes::IMREAD_COLOR);
    //    cv::Mat resizedImageBGR, resizedImageRGB, resizedImage, preprocessedImage;
    //    cv::resize(imageBGR, resizedImageBGR,
    //        cv::Size(224, 224),
    //        cv::InterpolationFlags::INTER_CUBIC);
    //    cv::cvtColor(resizedImageBGR, resizedImageRGB,
    //        cv::ColorConversionCodes::COLOR_BGR2RGB);
    //    resizedImageRGB.convertTo(resizedImage, CV_32F, 1.0 / 255);

    //    cv::Mat channels[3];
    //    cv::split(resizedImage, channels);
    //    // Normalization per channel
    //    // Normalization parameters obtained from
    //    // https://github.com/onnx/models/tree/master/vision/classification/squeezenet
    //    channels[0] = (channels[0] - 0.485) / 0.229;
    //    channels[1] = (channels[1] - 0.456) / 0.224;
    //    channels[2] = (channels[2] - 0.406) / 0.225;
    //    cv::merge(channels, 3, resizedImage);
    //    // HWC to CHW
    //    cv::dnn::blobFromImage(resizedImage, preprocessedImage);

    //    inputTensorValues.assign(preprocessedImage.begin<float>(),
    //        preprocessedImage.end<float>());
    //}


    std::tuple<std::string, float> processOutput(BOOL displayLabels) {
        std::vector<float> output = outputs[0].values;// select first output
        std::vector<float>::iterator predictionProbablity = std::max_element(output.begin(), output.end());
        int predictionIndex = std::distance(output.begin(), predictionProbablity);

        std::get<0>(result) = labels.at(predictionIndex);
        std::get<1>(result) = predictionProbablity[0];

        if (displayLabels)
            printOutput(result);
        return result;
    }

    void printOutput(std::tuple<std::string, float> result) {
        char displayString[100];
        sprintf_s(displayString, "%s (%f)", std::get<0>(result).c_str(), std::get<1>(result));
        // specify the fontand draw the key using puttext
        cv::putText(frame, displayString, cv::Point(10, 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2, cv::LINE_AA);
        //show the frame in the created window
        cv::imshow("Output", frame);
        cv::waitKey(1);
    }

};

