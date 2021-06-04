#pragma once
#include "framework.h"
#include "providers.h"


//#define USE_DML true

#ifdef USE_DML
#include "dml_provider_factory.h"
#endif


const OrtApi* g_ort = NULL;

#define ORT_ABORT_ON_ERROR(expr)                             \
  do {                                                       \
    OrtStatus* onnx_status = (expr);                         \
    if (onnx_status != NULL) {                               \
      const char* msg = g_ort->GetErrorMessage(onnx_status); \
      fprintf(stderr, "%s\n", msg);                          \
      g_ort->ReleaseStatus(onnx_status);                     \
      abort();                                               \
    }                                                        \
  } while (0);

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
    const int numInterOpsThreads = 16;
    const int numIntraOpsThreads = 16;
    
public:
    std::vector<cv::Mat> preprocessedFrames;
    std::vector<Input> inputs;
    std::vector<Output> outputs;

private:
    void enable_hardware_acceleration(Ort::SessionOptions &session_options) {
        int device_id = 0;
        ////int concurrent_session_runs = GetNumCpuCores();
        //bool enable_cpu_mem_arena = true;
        //ExecutionMode execution_mode = ExecutionMode::ORT_SEQUENTIAL;
        //int repeat_count = 1;
        ////int p_models = GetNumCpuCores();
        //GraphOptimizationLevel graph_optimization_level = ORT_ENABLE_ALL;
        //bool user_graph_optimization_level_set = false;
        //bool set_denormal_as_zero = false;
        //OrtLoggingLevel logging_level = ORT_LOGGING_LEVEL_ERROR;


#if defined(USE_CUDA)
        Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, 0));
#elif defined(USE_DML)
        // Disabling mem pattern and forcing single-threaded execution since DML is used
        session_options.DisableMemPattern();
        session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
        Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(session_options, device_id));

        //ID3D12Device* device;
        //ID3D12CommandQueue* cmd_queue;
        ////REFIID a = IDMLDevice; //IID_ID3D12Device
        //DML_CREATE_DEVICE_FLAGS flags;
        //Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProviderEx_DML(session_options, (IDMLDevice*)device, cmd_queue));
#elif defined(USE_OpenVINO)
        OrtOpenVINOProviderOptions options;
        options.device_type = "CPU_FP32";
        options.enable_vpu_fast_compile = 0;
        options.device_id = "";
        options.num_of_threads = 8;
        Ort::ThrowOnError(SessionOptionsAppendExecutionProvider_OpenVINO(session_options, &options));
        // Turn off high level optimizations performed by ONNX Runtime 
        // before handing the graph over to OpenVINO backend.
        session_options.SetGraphOptimizationLevel(ORT_DISABLE_ALL);
#else
        //do nothing
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
#endif
    }

    Ort::SessionOptions get_sessionOptions() {
        Ort::SessionOptions session_options = Ort::SessionOptions();
        // Modify session options here 
        session_options.SetInterOpNumThreads(numInterOpsThreads);//parallel execution of operators
        session_options.SetIntraOpNumThreads(numIntraOpsThreads);//parallelization of induvidual operator
        enable_hardware_acceleration(session_options);
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





