#pragma once

#include "MainPage.g.h"
#include <GazeInference.h>


using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::Foundation;

using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Media::Capture;
using namespace Windows::ApplicationModel;
using namespace Windows::System::Display;
using namespace Windows::Graphics::Display;

namespace winrt::GazeInference_UWP_Cpp::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        GazeInference gazeInference;


        //void OnNavigatedTo(NavigationEventArgs const& e);
        //void OnNavigatingFrom(NavigatingCancelEventArgs const& e);

        void BtnClearConsole_Handler(IInspectable const&, RoutedEventArgs const&);
        void BtnRunInferencePipeline_Handler(IInspectable const& sender, RoutedEventArgs const& e);
        void BtnRunCameraInferencePipeline_Handler(IInspectable const& sender, RoutedEventArgs const& e);
        void BtnRunMediaCaptureInferencePipeline_Handler(IInspectable const& sender, RoutedEventArgs const& e);
    };
}

namespace winrt::GazeInference_UWP_Cpp::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
