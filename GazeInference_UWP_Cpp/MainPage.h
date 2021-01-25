#pragma once

#include "MainPage.g.h"


using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::Foundation;

namespace winrt::GazeInference_UWP_Cpp::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void BtnClearConsole_Handler(IInspectable const&, RoutedEventArgs const&);
        void BtnRunInferencePipeline_Handler(IInspectable const& sender, RoutedEventArgs const& e);
        void BtnRunCameraInferencePipeline_Handler(IInspectable const& sender, RoutedEventArgs const& e);
    };
}

namespace winrt::GazeInference_UWP_Cpp::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
