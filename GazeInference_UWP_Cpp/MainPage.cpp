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
        GazeInference().RunInferencePipelineAsync(consoleTextBlock());
    }

    void MainPage::BtnRunCameraInferencePipeline_Handler(IInspectable const&, RoutedEventArgs const&)
    {
        GazeInference().RunCameraInferencePipelineAsync(consoleTextBlock(), imageControl());
    }

    void MainPage::BtnClearConsole_Handler(IInspectable const&, RoutedEventArgs const&)
    {
        // Clear the console
        consoleTextBlock().Text(L"");
    }
}
