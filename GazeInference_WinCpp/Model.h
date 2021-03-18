#pragma once
#include "framework.h"

struct Input {
    char* name;
    std::vector<int64_t> dims;
    std::vector<float> values;
};

struct Output {
    char* name;
    std::vector<int64_t> dims;
    std::vector<float> values;
};

template <typename T>
T vectorProduct(const std::vector<T>& v)
{
    return accumulate(v.begin(), v.end(), 1, std::multiplies<T>());
}


/* 
* This is the Base class to interface with any generic ONNX model
* and provide basic inference capability.
* TODO: Implement Training capability
*/
class Model {
private:   
    // Basic ONNX Runtime Setup
    Ort::Env env = Ort::Env(ORT_LOGGING_LEVEL_WARNING);
    Ort::Session session{ nullptr };

    std::vector<const char*> inputNames;
    std::vector<const char*> outputNames;
    std::vector<Ort::Value> inputTensors;
    std::vector<Ort::Value> outputTensors;
    const int numInterOpsThreads = 1;
    const int numIntraOpsThreads = 1;
    
public:
    std::vector<cv::Mat> preprocessedFrames;
    std::vector<Input> inputs;
    std::vector<Output> outputs;

private:
    void enable_hardware_acceleration(OrtSessionOptions* session_options) {
//#ifdef USE_CUDA
#if defined(USE_CUDA)
        ORT_ABORT_ON_ERROR(OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, 0));
//#elifdef USE_DML
#elif defined(USE_DML)
        ORT_ABORT_ON_ERROR(OrtSessionOptionsAppendExecutionProvider_DML(session_options, 0));
#else
        //do nothing
#endif
    }

    Ort::SessionOptions get_sessionOptions() {
        Ort::SessionOptions session_options = Ort::SessionOptions();
        // Modify session options here 
        session_options.SetInterOpNumThreads(numInterOpsThreads);//parallel execution of operators
        session_options.SetIntraOpNumThreads(numIntraOpsThreads);//parallelization of induvidual operator
        enable_hardware_acceleration(session_options);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        return session_options;
    }

    Ort::Session get_session(Ort::Env& env, const wchar_t* model_path, Ort::SessionOptions& session_options) {
        Ort::Session session{ env, model_path, session_options };
        return session;
    }

    void bindModelInputOutput(Ort::Session& session) {

        // Retrieve Input/Output Names
        Ort::AllocatorWithDefaultOptions allocator;
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(
            OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);


        // Assign the inputs/outputs Tensors to the inputTensors/outputTensors
        // *** these buffers will not change, only the inputs/outputs get updated ***
        size_t numInputNodes = session.GetInputCount();
        for (int i = 0; i < numInputNodes; i++) {

            // Retrieve Input Tensor Info
            Ort::TypeInfo inputTypeInfo = session.GetInputTypeInfo(i);
            Ort::TensorTypeAndShapeInfo inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
            ONNXTensorElementDataType inputType = inputTensorInfo.GetElementType();
            std::vector<int64_t> inputDims = inputTensorInfo.GetShape();

            LOG_DEBUG("##########\n");
            for (std::string provider : Ort::GetAvailableProviders()) {
                LOG_DEBUG("%s\n", provider.c_str());
            }
            

            Input input;
            input.dims = inputDims; // set shape
            input.values = std::vector<float>(vectorProduct(inputDims)); // set values
            input.name = session.GetInputName(i, allocator); // set name
            inputs.push_back(input);


            inputNames.push_back(inputs[i].name);
            inputTensors.push_back(Ort::Value::CreateTensor<float>(memoryInfo,
                inputs[i].values.data(), inputs[i].values.size(),
                inputs[i].dims.data(), inputs[i].dims.size()));

            //cv::Mat dummyFrame = cv::Mat(224, 224);
            //preprocessedFrames.push_back(dummyFrame);

            // Cleanup
            inputTypeInfo.release();
            inputTensorInfo.release();
        }

        size_t numOutputNodes = session.GetOutputCount();
        for (int i = 0; i < numOutputNodes; i++) {
            // Retrieve Output Tensor Info
            Ort::TypeInfo outputTypeInfo = session.GetOutputTypeInfo(i);
            Ort::TensorTypeAndShapeInfo outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
            ONNXTensorElementDataType outputType = outputTensorInfo.GetElementType();
            std::vector<int64_t> outputDims = outputTensorInfo.GetShape();

            Output output;
            output.dims = outputDims; // set shape
            output.values = std::vector<float>(vectorProduct(outputDims)); // set values
            output.name = session.GetOutputName(i, allocator); // set name
            outputs.push_back(output);

            outputNames.push_back(outputs[i].name);
            outputTensors.push_back(Ort::Value::CreateTensor<float>(memoryInfo,
                outputs[i].values.data(), outputs[i].values.size(),
                outputs[i].dims.data(), outputs[i].dims.size()));

            // Cleanup
            outputTypeInfo.release();
            outputTensorInfo.release();
        }
    }

public:
    Model(const wchar_t* modelFilepath) {
        
        // Load model from filepath and create session 
        Ort::SessionOptions session_options = get_sessionOptions();
        session = get_session(env, modelFilepath, session_options);

        // Define Input/Output (name, tensors, dim) 
        bindModelInputOutput(session);

        // Cleanup 
        session_options.release();
    }

    ~Model() {
        // Cleanup ORT memory variables here
        env.release();
        session.release();
    }

    void fillInputTensor() {
        // Assign each processedFrame(in CHW format) to each of the inputs
        for (int i = 0; i < preprocessedFrames.size(); i++) {
            // fill the inputs
            inputs[i].values.assign(preprocessedFrames[i].begin<float>(),
                preprocessedFrames[i].end<float>());
        }
    }

    void run() {
        try {
            session.Run(Ort::RunOptions{ nullptr },
                inputNames.data(), inputTensors.data(), inputTensors.size(),
                outputNames.data(), outputTensors.data(), outputTensors.size());
        }
        catch (Ort::Exception e) {
            LOG_ERROR("%d: %s", e.GetOrtErrorCode(), e.what());
        }
    }

};





