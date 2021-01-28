#pragma once

using namespace winrt;
using namespace Windows::AI::MachineLearning;
using namespace Windows::Media;
using namespace Windows::Foundation;
using namespace Windows::Media::Capture;


namespace winrt::GazeInference_UWP_Cpp::implementation
{

    class InferenceConfig
    {
    public:
        InferenceConfig();

        LearningModel model = nullptr;
        // Default (system decides), Cpu, DirectX (takes initial setup time), DirectXHighPerformance (Pretty quick), DirectXMinPower
        LearningModelDeviceKind deviceKind = LearningModelDeviceKind::Default;
        LearningModelSession session = nullptr;
        LearningModelBinding binding = nullptr;
        VideoFrame imageFrame = nullptr;
        MediaCapture mediaCapture = nullptr;
        bool isPreviewing = false;
        int k = 3;

        hstring modelPath = L"ms-appx:///Assets/SqueezeNet.onnx";
        hstring imagePath = L"ms-appx:///Assets/kitten_224.png";
        hstring labelsFilePath = L"ms-appx:///Assets/Labels.txt";

    };

}