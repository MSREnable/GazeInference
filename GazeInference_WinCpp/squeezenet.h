#pragma once
#include "framework.h"
#include "Utils.h"


// This is the structure to interface with the SqueezeNet model
// After instantiation, set the input data to be the 3x224x224 pixel image of the number to recognize
// Then call Run() to fill in the output data with the probabilities of each
// result holds the index with highest probability (aka the number the model thinks is in the image)
struct SqueezeNet {
    SqueezeNet() {

        // Modify session options here 
        //session_options.SetIntraOpNumThreads(1);
        ////OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, 1);
        //session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(
        OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
        inputTensors.push_back(Ort::Value::CreateTensor<float>(memoryInfo, 
            inputTensorValues.data(), inputTensorSize, 
            inputDims.data(), inputDims.size()));
        outputTensors.push_back(Ort::Value::CreateTensor<float>(memoryInfo, 
            outputTensorValues.data(), outputTensorSize,
            outputDims.data(), outputDims.size()));
    }

    ~SqueezeNet() {
        // Cleanup ORT memory variables here
        env.release();
        session_options.release();
        session.release();
        inputTypeInfo.release();
        inputTensorInfo.release();
        outputTypeInfo.release();
        outputTensorInfo.release();
        
    }

    std::tuple<std::string, float> run() {
        std::vector<const char*> inputNames{ inputName };
        std::vector<const char*> outputNames{ outputName };

        session.Run(Ort::RunOptions{ nullptr }, 
            inputNames.data(), inputTensors.data(), 1, 
            outputNames.data(), outputTensors.data(), 1);

        std::vector<float>::iterator predictionProbablity = std::max_element(outputTensorValues.begin(), outputTensorValues.end());
        int predictionIndex = std::distance(outputTensorValues.begin(), predictionProbablity);

        std::get<0>(result) = labels.at(predictionIndex);
        std::get<1>(result) = predictionProbablity[0];

        printOutput(result);
        return result;
    }


    //void FillInputTensor(std::string imageFilepath) {

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


    void initCamera() {
        // open camera for video stream
        capture = VideoCapture(0);

        // if not success, exit program
        if (!capture.isOpened())
        {
            //"Cannot open the video camera" 
            cin.get(); //wait for any key press
        }

        //get the frames rate of the video
        double fps = capture.get(CAP_PROP_FPS);
        double width = capture.get(CAP_PROP_FRAME_WIDTH);
        double height = capture.get(CAP_PROP_FRAME_HEIGHT);
        namedWindow(window_name, WINDOW_NORMAL); //create a window
    }

    bool getCameraFrame() {
        bool bSuccess = capture.read(frame); // read a new frame from video 
        // Breaking the while loop at the end of the video
        if (bSuccess == false) {
            //"Video camera disconnected"
        }
        // resize image to 224x224
        cv::resize(frame, frame, cv::Size(224, 224), cv::InterpolationFlags::INTER_CUBIC);

        // Return False on Esc key 
        int delay_ms = 1;
        if (waitKey(delay_ms) == 27) {
            return false;
        }  
        else {
            return true;
        }
    }

    void getFrameFromImagePath(string imageFilepath) {
        frame = cv::imread(imageFilepath, cv::ImreadModes::IMREAD_COLOR);
        // resize image to 224x224
        cv::resize(frame, frame, cv::Size(224, 224), cv::InterpolationFlags::INTER_CUBIC);
    }

    void fillInputTensor() {
        cv::Mat preprocessedImage;
        // HWC to CHW
        cv::dnn::blobFromImage(frame, preprocessedImage);
        // fill the inputTensorValues
        inputTensorValues.assign(preprocessedImage.begin<float>(),
            preprocessedImage.end<float>());
    }

    void printOutput(std::tuple<std::string, float> result) {
        char displayString[100];
        sprintf_s(displayString, "%s (%f)", std::get<0>(result).c_str(), std::get<1>(result));
        // specify the fontand draw the key using puttext
        putText(frame, displayString, Point(10, 10), FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2, LINE_AA);
        //show the frame in the created window
        imshow(window_name, frame);
    }

    int benchmark() {
        // Measure latency over large number of samples
        int numTests{ 100 };
        std::vector<const char*> inputNames{ inputName };
        std::vector<const char*> outputNames{ outputName };

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        for (int i = 0; i < numTests; i++)
        {
            session.Run(Ort::RunOptions{ nullptr }, 
                inputNames.data(), inputTensors.data(), 1, 
                outputNames.data(), outputTensors.data(), 1);
        }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        int minimumInferenceLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(numTests);
        return minimumInferenceLatency_ms;
    }

private:
    std::string labelFilepath{ "assets/Labels.txt" };
#ifdef _WIN32
    const wchar_t* modelFilepath = L"assets/SqueezeNet.onnx";//ORTCHAR_T*
#else
    const char* modelFilepath = "assets/SqueezeNet.onnx";
#endif

    Ort::Env env;
    //Ort::Env env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "InferencePipeline");
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::SessionOptions session_options = Ort::SessionOptions();
    Ort::Session session{ env, modelFilepath, session_options };
    std::vector<std::string> labels{ loadLabels(labelFilepath) };

    // Define Input/Output (name, tensors, dim) 
    size_t numInputNodes = session.GetInputCount();
    const char* inputName = session.GetInputName(0, allocator);
    Ort::TypeInfo inputTypeInfo = session.GetInputTypeInfo(0);
    Ort::TensorTypeAndShapeInfo inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
    ONNXTensorElementDataType inputType = inputTensorInfo.GetElementType();
    std::vector<int64_t> inputDims = inputTensorInfo.GetShape();
    size_t inputTensorSize = vectorProduct(inputDims);

    size_t numOutputNodes = session.GetOutputCount();
    const char* outputName = session.GetOutputName(0, allocator);
    Ort::TypeInfo outputTypeInfo = session.GetOutputTypeInfo(0);
    Ort::TensorTypeAndShapeInfo outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
    ONNXTensorElementDataType outputType = outputTensorInfo.GetElementType();
    std::vector<int64_t> outputDims = outputTensorInfo.GetShape();
    size_t outputTensorSize = vectorProduct(outputDims);

    std::vector<Ort::Value> inputTensors;
    std::vector<Ort::Value> outputTensors;

    Mat frame;
    VideoCapture capture;
    String window_name = "Camera Feed";

public:
    std::vector<float> inputTensorValues = std::vector<float>(inputTensorSize);
    std::vector<float> outputTensorValues = std::vector<float>(outputTensorSize);
    std::tuple<std::string, float> result = std::tuple<std::string, float>("Unknown", 0.0f);

};

std::unique_ptr<SqueezeNet> squeezeNet;




