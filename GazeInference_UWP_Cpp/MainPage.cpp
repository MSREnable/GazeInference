#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include "GazeInference.h"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::GazeInference_UWP_Cpp::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
        GazeInference gazeInference = GazeInference();
    }

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void MainPage::BtnRunInferencePipeline_Handler(IInspectable const&, RoutedEventArgs const&)
    {
        gazeInference.RunInferencePipelineAsync(consoleTextBlock());
    }

    void MainPage::BtnRunCameraInferencePipeline_Handler(IInspectable const&, RoutedEventArgs const&)
    {
        gazeInference.RunCameraInferencePipelineAsync(consoleTextBlock(), imageControl());
    }

    void MainPage::BtnRunMediaCaptureInferencePipeline_Handler(IInspectable const&, RoutedEventArgs const&)
    {
        gazeInference.RunMediaCaptureInferencePipelineAsync(consoleTextBlock(), previewControl());
    }

    void MainPage::BtnClearConsole_Handler(IInspectable const&, RoutedEventArgs const&)
    {
        // Clear the console
        consoleTextBlock().Text(L"");
    }


    //void MainPage::OnNavigatedTo(NavigationEventArgs const& e)
    //{
    //    GazeInference().StartPreviewAsync(consoleTextBlock(), previewControl());
    //}

    //void MainPage::OnNavigatingFrom(NavigatingCancelEventArgs const& e)
    //{
    //    GazeInference().StopPreviewAsync(consoleTextBlock(), previewControl());
    //}
}
