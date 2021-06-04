#include "pch.h"
#include "GazeInference.h"

namespace winrt::GazeInference_UWP_Cpp::implementation
{
    // Constructor
    GazeInference::GazeInference()
    {
        config = InferenceConfig();
    }

    template<typename ... Args>
    std::string string_format(const std::string& format, Args ... args)
    {
        int size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
        if (size <= 0) { throw std::runtime_error("Error during formatting."); }
        std::unique_ptr<char[]> buf(new char[size]);
        snprintf(buf.get(), size, format.c_str(), args ...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

    /* Gets string tokens using the supplied delimiter*/
    std::vector<std::string> tokenize(std::string inputString, const char* delimiter)
    {
        std::vector<std::string> tokens;
        int first = 0;
        while (first < inputString.size()) {
            int second = inputString.find_first_of(delimiter, first);
            //first has index of start of token
            //second has index of end of token + 1;
            if (second == std::string::npos) {
                second = inputString.size();
            }
            std::string token = inputString.substr(first, second - first);
            tokens.push_back(token);
            first = second + 1;
        }
        return tokens;
    }

    /* Gets the top K results */
    IMap<hstring, float> GetTopK(IVectorView<float> results, IVectorView<hstring> labels, int k) {
        // Find the top k probabilities
        IVector<float> topProbabilities{ winrt::single_threaded_vector<float>(std::vector<float>(k)) };
        IVector<float> topProbabilityLabelIndexes{ winrt::single_threaded_vector<float>(std::vector<float>(k)) };
        // SqueezeNet returns a list of 1000 options, with probabilities for each, loop through all
        for (uint32_t i = 0; i < results.Size(); i++) {
            // is it one of the top k?
            for (int j = 0; j < k; j++) {
                if (results.GetAt(i) > topProbabilities.GetAt(j)) {
                    topProbabilityLabelIndexes.SetAt(j, i);
                    topProbabilities.SetAt(j, results.GetAt(i));
                    break;
                }
            }
        }

        // Collect the top K the result
        IMap<hstring, float> topK{ winrt::single_threaded_map<hstring, float>() };
        for (int i = 0; i < k; i++) {
            topK.Insert(labels.GetAt(topProbabilityLabelIndexes.GetAt(i)), topProbabilities.GetAt(i));
        }

        return topK;
    }

    /* Prints the output on the supplied textBlock */
    void PrintOutput(TextBlock textblock, IMap<hstring, float> output) {
        hstring debugString = L"";
        for (IKeyValuePair item : output)
            debugString = debugString + item.Key() + L" (" + to_hstring(item.Value()) + L"); ";

        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + debugString);
    }



    // Implementations of async tasks 
    IAsyncAction GazeInference::DoWorkAsync(TextBlock textblock)
    {
        // switch to backgroung thread-pool
        co_await winrt::resume_background();

        // Do compute-bound work here.

        // Switch to the foreground thread associated with textblock.
        //co_await textblock;
        //co_await winrt::resume_foreground(textblock.Dispatcher(), Windows::UI::Core::CoreDispatcherPriority::Normal);
        co_await winrt::resume_foreground(textblock.Dispatcher());

        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + L"Complete!");
    }

    IAsyncAction GazeInference::RunInferencePipelineAsync(TextBlock textblock)
    {
        // image path is going to be a dynamic input
        hstring imagePath = L"ms-appx:///Assets/kitten_224.png";

        config.model = co_await LoadModelOpAsync(textblock, config);
        config.imageFrame = co_await LoadImageFileOPAsync(textblock, imagePath);
        config.session = co_await CreateSessionOpAsync(textblock, config);
        config.binding = co_await CreateBindingOpAsync(textblock, config);
        IVectorView<float> results = co_await EvaluateModelOpAsync(textblock, config);
        IVectorView<hstring> labels = co_await LoadLabelsOpAsync(textblock, config);
        IMap<hstring, float> output = GetTopK(results, labels, config.k);
        PrintOutput(textblock, output);
    }

    IAsyncAction GazeInference::RunCameraInferencePipelineAsync(TextBlock textblock, Image imageControl)
    {
        config.model = co_await LoadModelOpAsync(textblock, config);
        // imageFrame is going to be a dynamic input from the CameraCapture
        StorageFile file = co_await GetPhotoAsync(textblock);
        config.imageFrame = co_await LoadImageFileOPAsync(textblock, file, imageControl);
        config.session = co_await CreateSessionOpAsync(textblock, config);
        config.binding = co_await CreateBindingOpAsync(textblock, config);
        IVectorView<float> results = co_await EvaluateModelOpAsync(textblock, config);
        IVectorView<hstring> labels = co_await LoadLabelsOpAsync(textblock, config);
        IMap<hstring, float> output = GetTopK(results, labels, config.k);
        PrintOutput(textblock, output);
    }

    IAsyncAction GazeInference::RunMediaCaptureInferencePipelineAsync(TextBlock textblock, CaptureElement previewControl)
    {
        config.model = co_await LoadModelOpAsync(textblock, config);
        // imageFrame is going to be a dynamic input from the CameraCapture
        config.mediaCapture = co_await StartPreviewAsync(textblock, previewControl, config);
        config.imageFrame = co_await GetFrameAsync(textblock, config);
        config.session = co_await CreateSessionOpAsync(textblock, config);
        config.binding = co_await CreateBindingOpAsync(textblock, config);
        IVectorView<float> results = co_await EvaluateModelOpAsync(textblock, config);
        IVectorView<hstring> labels = co_await LoadLabelsOpAsync(textblock, config);
        IMap<hstring, float> output = GetTopK(results, labels, config.k);
        PrintOutput(textblock, output);
        config.mediaCapture = co_await StopPreviewAsync(textblock, previewControl, config);


        //if (config.isPreviewing == true) {
        //    config.isPreviewing = false;
        //}
        //else {
        //    config.isPreviewing = true;
        //}

        //// One time operations
        //config.model = co_await LoadModelOpAsync(textblock, config);
        //IVectorView<hstring> labels = co_await LoadLabelsOpAsync(textblock, config);

        //// imageFrame is going to be a dynamic input from the CameraCapture
        //config.mediaCapture = co_await StartPreviewAsync(textblock, previewControl, config);
        //while (config.isPreviewing) {
        //    config.imageFrame = co_await GetFrameAsync(textblock, config);
        //    config.session = co_await CreateSessionOpAsync(textblock, config);
        //    config.binding = co_await CreateBindingOpAsync(textblock, config);
        //    IVectorView<float> results = co_await EvaluateModelOpAsync(textblock, config);
        //    IMap<hstring, float> output = GetTopK(results, labels, config.k);
        //    PrintOutput(textblock, output);
        //}
        //config.mediaCapture = co_await StopPreviewAsync(textblock, previewControl, config);
    }

    IAsyncOperation<LearningModel> GazeInference::LoadModelOpAsync(TextBlock textblock, InferenceConfig config)
    {
        // Create coroutine copy of input variables
        InferenceConfig m_config{ config };
        // Declare the output/return variables
        LearningModel m_model = nullptr;

        // switch to backgroung thread-pool
        co_await winrt::resume_background();

        // Do compute-bound work here.
        hstring debugString;
        DWORD ticks = GetTickCount();

        try {
            // Load and create the model
            StorageFile modelFile{ co_await StorageFile::GetFileFromApplicationUriAsync(Uri(m_config.modelPath)) };
            m_model = co_await LearningModel::LoadFromStorageFileAsync(modelFile);
            ticks = GetTickCount() - ticks;
            debugString = to_hstring(string_format("Model file loded in %d ticks", ticks));
        }
        catch (...) {
            debugString = L"Error - Could not load the onnx model.";
        }


        // Switch to the foreground thread associated with textblock.
        co_await winrt::resume_foreground(textblock.Dispatcher());
        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + debugString);
        config.model = m_model;

        return m_model;
    }

    IAsyncOperation<VideoFrame> GazeInference::LoadImageFileOPAsync(TextBlock textblock, hstring imagePath)
    {
        // Create coroutine copy of input variables
        hstring m_imagePath{ imagePath };

        // switch to backgroung thread-pool
        co_await winrt::resume_background();

        // load the file as StorageFile
        StorageFile file = co_await StorageFile::GetFileFromApplicationUriAsync(Uri(m_imagePath));
        // Use StorageFile to load the image using default constructor
        co_return co_await GazeInference::LoadImageFileOPAsync(textblock, file);
    }

    IAsyncOperation<VideoFrame> GazeInference::LoadImageFileOPAsync(TextBlock textblock, StorageFile file)
    {
        // Create coroutine copy of input variables
        StorageFile m_file{ file };

        // switch to backgroung thread-pool
        co_await winrt::resume_background();

        // Do compute-bound work here.
        DWORD ticks = GetTickCount();
        VideoFrame inputImage = nullptr;
        hstring debugString;

        try
        {

            // get a stream on it
            auto stream = co_await m_file.OpenAsync(FileAccessMode::Read);
            // Create the decoder from the stream
            BitmapDecoder decoder = co_await BitmapDecoder::CreateAsync(stream);

            // get the bitmap type
            SoftwareBitmap softwareBitmap = co_await decoder.GetSoftwareBitmapAsync();

            // load a videoframe from it
            inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);

            ticks = GetTickCount() - ticks;
            debugString = to_hstring(string_format("Image file loded in %d ticks", ticks));
        }
        catch (...)
        {
            debugString = L"Error: Failed to load the image file. Please check that the filepath exists.";
        }


        // Switch to the foreground thread associated with textblock.
        co_await winrt::resume_foreground(textblock.Dispatcher());
        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + debugString);

        co_return inputImage;
    }

    IAsyncOperation<VideoFrame> GazeInference::LoadImageFileOPAsync(TextBlock textblock, StorageFile file, Image imageControl)
    {
        hstring debugString;
        if (!file) {
            debugString = L"Error: image storage file is Null. Please check that Photo operation was successful.";
            textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + debugString);
            co_return;
        }

        // Create coroutine copy of input variables
        StorageFile m_file{ file };
        //Image m_imageControl{ imageControl };
        SoftwareBitmapSource bitmapSourceDisplay;
        SoftwareBitmap softwareBitmap = nullptr;

        // switch to backgroung thread-pool
        co_await winrt::resume_background();

        // Do compute-bound work here.
        DWORD ticks = GetTickCount();
        VideoFrame inputImage = nullptr;
        
        try
        {
            // get a stream on it
            auto stream = co_await m_file.OpenAsync(FileAccessMode::Read);
            // Create the decoder from the stream
            BitmapDecoder decoder = co_await BitmapDecoder::CreateAsync(stream);
            
            // get the bitmap type
            softwareBitmap = co_await decoder.GetSoftwareBitmapAsync();

            // create a videoframe from it
            inputImage = VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);

            ticks = GetTickCount() - ticks;
            debugString = to_hstring(string_format("Image file loded in %d ticks", ticks));
        }
        catch (...)
        {
            debugString = L"Error: Failed to load the image file. Please check that the filepath exists.";
        }

        // Switch to the foreground thread associated with textblock.
        co_await winrt::resume_foreground(textblock.Dispatcher());
        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + debugString);

        // Update the preview in foreground thread
        // The Image control requires that the image source be in BGRA8 format and premultiplied
        SoftwareBitmap softwareBitmapBGR8 = SoftwareBitmap::Convert(softwareBitmap,
                                            BitmapPixelFormat::Bgra8,
                                            BitmapAlphaMode::Premultiplied);
        co_await bitmapSourceDisplay.SetBitmapAsync(softwareBitmapBGR8);
        imageControl.Source(bitmapSourceDisplay);

        co_return inputImage;
    }

    IAsyncOperation<LearningModelSession> GazeInference::CreateSessionOpAsync(TextBlock textblock, InferenceConfig config)
    {
        // Create coroutine copy of input variables
        InferenceConfig m_config{ config };
        // Declare the output/return variables
        LearningModelSession session = nullptr;

        // switch to backgroung thread-pool
        co_await winrt::resume_background();

        // Do compute-bound work here.
        DWORD ticks = GetTickCount();
        hstring debugString;

        try {
            // now create a session and binding
            session = LearningModelSession{ m_config.model, LearningModelDevice(m_config.deviceKind) };
            config.session = session;

            ticks = GetTickCount() - ticks;
            debugString = to_hstring(string_format("Model bound in %d ticks", ticks));
        }
        catch (...) {
            debugString = L"Error: Could not bind the onnx model.";
        }

        // Switch to the foreground thread associated with textblock.
        co_await winrt::resume_foreground(textblock.Dispatcher());
        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + debugString);


        co_return session;
    }

    IAsyncOperation<LearningModelBinding> GazeInference::CreateBindingOpAsync(TextBlock textblock, InferenceConfig config)
    {
        // Create coroutine copy of input variables
        InferenceConfig m_config{ config };
        // Declare the output/return variables
        LearningModelBinding m_binding = nullptr;


        // switch to backgroung thread-pool
        co_await winrt::resume_background();

        // Do compute-bound work here.
        DWORD ticks = GetTickCount();
        hstring debugString;

        try {
            // now create a binding from the session
            m_binding = LearningModelBinding{ m_config.session };

            // bind the intput image
            m_binding.Bind(L"data_0", ImageFeatureValue::CreateFromVideoFrame(m_config.imageFrame));

            // bind the output
            vector<int64_t> shapeOutput({ 1, 1000, 1, 1 });
            m_binding.Bind(L"softmaxout_1", TensorFloat::Create(shapeOutput));

            ticks = GetTickCount() - ticks;
            debugString = to_hstring(string_format("Model bound in %d ticks", ticks));
        }
        catch (...) {
            debugString = L"Error: Could not bind the onnx model.";
        }

        // Switch to the foreground thread associated with textblock.
        co_await winrt::resume_foreground(textblock.Dispatcher());
        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + debugString);

        co_return m_binding;
    }

    IAsyncOperation<IVectorView<float>> GazeInference::EvaluateModelOpAsync(TextBlock textblock, InferenceConfig config)
    {
        // Create coroutine copy of input variables
        InferenceConfig m_config{ config };
        // Declare the output/return variables
        IVectorView<float> resultVector{};


        // switch to backgroung thread-pool
        co_await winrt::resume_background();

        // Do compute-bound work here.
        DWORD ticks = GetTickCount();
        hstring debugString;

        try {
            // now use the session to evaluate results
            LearningModelEvaluationResult results = m_config.session.Evaluate(m_config.binding, L"RunId");

            // get the output
            TensorFloat resultTensor = results.Outputs().Lookup(L"softmaxout_1").as<TensorFloat>();
            resultVector = resultTensor.GetAsVectorView();

            ticks = GetTickCount() - ticks;
            debugString = to_hstring(string_format("Evaluated model in %d ticks", ticks));
        }
        catch (...) {
            debugString = L"Error: Could not evaluate the model.";
        }

        // Switch to the foreground thread associated with textblock.
        co_await winrt::resume_foreground(textblock.Dispatcher());
        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + debugString);

        co_return resultVector;
    }

    IAsyncOperation<IVectorView<hstring>> GazeInference::LoadLabelsOpAsync(TextBlock textblock, InferenceConfig config)
    {

        // Create coroutine copy of input variables
        InferenceConfig m_config{ config };
        // Declare the output/return variables
        IVector<hstring> labels{ winrt::single_threaded_vector<hstring>() };

        // switch to backgroung thread-pool
        co_await winrt::resume_background();

        // Do compute-bound work here.
        DWORD ticks = GetTickCount();
        hstring debugString;

        try {
            // open the file
            StorageFile file = co_await StorageFile::GetFileFromApplicationUriAsync(Uri(m_config.labelsFilePath));

            // directly read the text
            //hstring text = co_await Windows::Storage::FileIO::ReadTextAsync(file);
            for (auto&& line : co_await Windows::Storage::FileIO::ReadLinesAsync(file)) {
                //TODO: Update tokenize method with winRT data types 
                std::vector<std::string> tokens = tokenize(winrt::to_string(line), ",");
                // We are only reading one key and one value pair here
                // Labels.txt has multiple labels per class
                //labels.Append(to_hstring(tokens[1]));
                labels.InsertAt(atoi(tokens[0].c_str()), to_hstring(tokens[1]));
            }

            ticks = GetTickCount() - ticks;
            debugString = to_hstring(string_format("Labels loaded in %d ticks.", ticks));
        }
        catch (...) {
            debugString = L"Error: Could not load the labels.";
        }

        //// Debug code for testing
        //for (hstring item : labels)
        //    debugString = debugString + item + L";";

        // Switch to the foreground thread associated with textblock.
        co_await winrt::resume_foreground(textblock.Dispatcher());
        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + debugString);

        // return a ReadOnly view of the vector
        co_return labels.GetView();
    }


    // Implementations of async tasks 
    IAsyncOperation<StorageFile> GazeInference::GetPhotoAsync(TextBlock textblock)
    {
        CameraCaptureUI captureUI; //needs to be on main UI thread
        captureUI.PhotoSettings().Format(CameraCaptureUIPhotoFormat::Jpeg);
        captureUI.PhotoSettings().CroppedSizeInPixels({ 224, 224 });
        
        // Camera Async operations happens on foreground thread
        StorageFile photo = co_await captureUI.CaptureFileAsync(CameraCaptureUIMode::Photo);

        // Do compute-bound work here.
        if (!photo)
        {
            // User cancelled photo capture
            co_return;
        }

        // switch to backgroung thread-pool
        co_await winrt::resume_background();

        // Optional: save photo to GazeInferencePhotoFolder in appdata/local
        if (false) {
            StorageFolder destinationFolder =
                co_await ApplicationData::Current().LocalFolder().CreateFolderAsync(L"GazeInferencePhotoFolder",
                    CreationCollisionOption::OpenIfExists);

            co_await photo.CopyAsync(destinationFolder, L"GazeInferencePhoto.jpg", NameCollisionOption::ReplaceExisting);
            co_await photo.DeleteAsync();
        }
        
        // Switch to the foreground thread associated with textblock.
        co_await winrt::resume_foreground(textblock.Dispatcher());
        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + L"Complete!");

        co_return photo;
    }

    IAsyncOperation<VideoFrame> GazeInference::GetFrameAsync(TextBlock textblock, InferenceConfig config)
    {
        // Create coroutine copy of input variables
        InferenceConfig m_config{ config };

        // Prepare and capture photo
        ImageEncodingProperties properties = ImageEncodingProperties::CreateUncompressed(MediaPixelFormat::Bgra8);
        LowLagPhotoCapture lowLagCapture = co_await m_config.mediaCapture.PrepareLowLagPhotoCaptureAsync(properties);

        CapturedPhoto capturedPhoto = co_await lowLagCapture.CaptureAsync();
        SoftwareBitmap softwareBitmap = capturedPhoto.Frame().SoftwareBitmap();
        co_await lowLagCapture.FinishAsync();

        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + L"Complete!");
        co_return VideoFrame::CreateWithSoftwareBitmap(softwareBitmap);
    }

    IAsyncOperation<MediaCapture> GazeInference::StartPreviewAsync(TextBlock textblock, CaptureElement previewControl, InferenceConfig config)
    {
        InferenceConfig m_config{ config };
        hstring debugString;

        try
        {
            m_config.mediaCapture = MediaCapture();
            co_await m_config.mediaCapture.InitializeAsync();
        }
        catch (...) 
        {
            debugString = L"The app was denied access to the camera.";
        }
        previewControl.Source(m_config.mediaCapture);
        co_await m_config.mediaCapture.StartPreviewAsync();

        debugString = L"Media preview started successfully.";
        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + debugString);
        co_return m_config.mediaCapture;
    }

    IAsyncOperation<MediaCapture> GazeInference::StopPreviewAsync(TextBlock textblock, CaptureElement previewControl, InferenceConfig config)
    {
        InferenceConfig m_config{config};

        if (m_config.mediaCapture != nullptr)
        {
            co_await m_config.mediaCapture.StopPreviewAsync();
            previewControl.Source(nullptr);
        }
        textblock.Text(textblock.Text() + L"\n" + to_hstring(__FUNCTION__) + L" : " + L"Complete!");
        co_return m_config.mediaCapture;
    }
}




