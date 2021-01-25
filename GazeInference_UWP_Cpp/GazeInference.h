#pragma once
#include <InferenceConfig.h>
//#include <winrt/Windows.UI.Core.h> // for resume_foreground method
//#include <ppltasks.h> // for concurrency::create_task


using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::AI::MachineLearning;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Data::Text;


using namespace Windows::Media::Capture;
using namespace Windows::Media::Playback;

using namespace Windows::Graphics::Imaging;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Media::Imaging;


using namespace std;


namespace winrt::GazeInference_UWP_Cpp::implementation
{
    struct GazeInference
    {
        GazeInference();

        IAsyncAction GazeInference::RunInferencePipelineAsync(TextBlock textblock);
        IAsyncAction GazeInference::RunCameraInferencePipelineAsync(TextBlock textblock, Image imageControl);
        IAsyncAction GazeInference::DoWorkAsync(TextBlock textblock);

        IAsyncOperation<StorageFile> GazeInference::GetPhotoAsync(TextBlock textblock);
        IAsyncOperation<LearningModel> GazeInference::LoadModelOpAsync(TextBlock textblock, InferenceConfig config);
        IAsyncOperation<VideoFrame> GazeInference::LoadImageFileOPAsync(TextBlock textblock, hstring imagePath);
        IAsyncOperation<VideoFrame> GazeInference::LoadImageFileOPAsync(TextBlock textblock, StorageFile file);
        IAsyncOperation<VideoFrame> GazeInference::LoadImageFileOPAsync(TextBlock textblock, StorageFile file, Image imageControl);
        IAsyncOperation<LearningModelSession> GazeInference::CreateSessionOpAsync(TextBlock textblock, InferenceConfig config);
        IAsyncOperation<LearningModelBinding> GazeInference::CreateBindingOpAsync(TextBlock textblock, InferenceConfig config);
        IAsyncOperation<IVectorView<float>> GazeInference::EvaluateModelOpAsync(TextBlock textblock, InferenceConfig config);
        IAsyncOperation<IVectorView<hstring>> GazeInference::LoadLabelsOpAsync(TextBlock textblock, InferenceConfig config);

        // Forward declarations
        /*template<typename ... Args>
        std::string string_format(const std::string& format, Args ... args);
        std::vector<std::string> tokenize(std::string inputString, const char* delimiter);
        IMap<hstring, float> GetTopK(IVectorView<float> results, IVectorView<hstring> labels, int k);
        void PrintOutput(TextBlock textblock, IMap<hstring, float> output);*/
    };

}

namespace winrt::GazeInference_UWP_Cpp::factory_implementation
{
    struct GazeInference
    {
    };
}