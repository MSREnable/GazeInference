#pragma once
#include "Model.h"
#include "DlibFaceDetector.h"
#include "LiveCapture.h"


// ITracker Model
class ITrackerModel : public Model
{
private:
    cv::Mat frame;
    std::tuple<std::string, float> result = std::tuple<std::string, float>("Unknown", 0.0f);
    std::unique_ptr<LiveCapture> live_capture;
    std::unique_ptr<DlibFaceDetector> detector;

    std::chrono::steady_clock::time_point epoch;
    double timestamp_ms = -1;
    int frame_count = 0;

public:
    ITrackerModel(const wchar_t* modelFilePath) 
        : Model{ modelFilePath }
    {

    }

    ~ITrackerModel() {
        // Cleanup 
    }

    bool initCamera() {

        // Initialize face ROI/landmark detector
        detector = std::make_unique<DlibFaceDetector>();
        
        // Initialize live capture
        live_capture = std::make_unique<LiveCapture>();
        live_capture->open();

        
        return ( live_capture && detector);
    }

    bool getFrameFromImagePath(std::string imageFilepath) {
        return live_capture->getFrameFromImagePath(imageFilepath, frame);
    }

    bool getFrame() {
        if (frame_count == 0)
            epoch = std::chrono::steady_clock::now();

        bool status = live_capture->getFrame(frame);

        // calculate timing properties
        frame_count++;
        timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - epoch).count();

        LOG_DEBUG("%d | %f | %f \n", frame_count, timestamp_ms, (1000 * frame_count)/ timestamp_ms);
        
        return status;
    }

    void showRawInput() {
        live_capture->showRawInput(frame);
    }

    bool applyTransformations() {
        // Apply ROI Extraction through dlib
        std::vector<cv::Mat> roi_frames = detector->ROIExtraction(frame, live_capture->downscaling);

        if (roi_frames.size() != 4) {
            return false;
        }

        // TODO: Make it faster by initing the preprocessedFrames and reusing
        preprocessedFrames.clear();
        for (int i = 0; i < roi_frames.size(); i++) {
            // HWC to CHW
            cv::dnn::blobFromImage(roi_frames[i], roi_frames[i]);
            preprocessedFrames.push_back(roi_frames[i]);
        }
        return true;
    }

    std::vector<float> processOutput() {
        std::vector<float> coordinates = outputs[0].values;
        float x = coordinates[0];
        float y = coordinates[1];
        LOG_DEBUG("x=%.2f, y=%.2f\n", x, y);
        return coordinates;
    }

    int benchmark() {
        cv::Mat frame;
        std::vector<cv::Mat> roi_images;
        std::vector<float> coordinates_XY;
        bool is_valid;

        // Measure latency over large number of samples
        int numTests{ 20 };
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        for (int i = 0; i < numTests; i++)
        {
            is_valid = getFrame(); //reads a new frame
            if (!is_valid)
                continue;
            is_valid = applyTransformations();
            if (!is_valid)
                continue;
            fillInputTensor();
            run();
            coordinates_XY = processOutput();
        }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        int avgLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(numTests);
        return avgLatency_ms;
    }

    int benchmark2() {
        cv::Mat frame;
        std::vector<cv::Mat> roi_images;
        std::vector<float> coordinates_XY;
        bool is_valid;

        // Measure latency over large number of samples
        int numTests{ 100 };
        std::chrono::steady_clock::time_point begin;
        std::chrono::steady_clock::time_point end;

        // Camera Frame Latency
        begin = std::chrono::steady_clock::now();
        for (int i = 0; i < numTests; i++)
        {
            is_valid = getFrame(); //reads a new frame
        }
        end = std::chrono::steady_clock::now();
        int avgFrameLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(numTests);

        // 
        begin = std::chrono::steady_clock::now();
        for (int i = 0; i < numTests; i++)
        {
            is_valid = applyTransformations();
        }
        end = std::chrono::steady_clock::now();
        int avgApplyTransformationsLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(numTests);


        begin = std::chrono::steady_clock::now();
        for (int i = 0; i < numTests; i++)
        {
            fillInputTensor();
        }
        end = std::chrono::steady_clock::now();
        int avgFillInputTensorLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(numTests);

        begin = std::chrono::steady_clock::now();
        for (int i = 0; i < numTests; i++)
        {
            run();
        }
        end = std::chrono::steady_clock::now();
        int avgModelLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(numTests);

        begin = std::chrono::steady_clock::now();
        for (int i = 0; i < numTests; i++)
        {
            coordinates_XY = processOutput();
        }
        end = std::chrono::steady_clock::now();
        int avgProcessOutputLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(numTests);


        int avgLatency_ms = avgFrameLatency_ms
            + avgApplyTransformationsLatency_ms
            + avgFillInputTensorLatency_ms
            + avgModelLatency_ms
            + avgProcessOutputLatency_ms;

        return avgLatency_ms;
    }

    int benchmark_dlib() {
        std::vector<cv::Mat> roi_images;
        bool is_valid;
        std::vector<cv::Point2f> face_shape_vector;
        std::vector<cv::RotatedRect> rectangles;

        // Measure latency over large number of samples
        int numTests{ 100 };
        std::chrono::steady_clock::time_point begin;
        std::chrono::steady_clock::time_point end;


        // Load frame
        getFrame(); //reads a new frame model->frame
        

        // Init dlib
        std::unique_ptr<DlibFaceDetector> detector;
        begin = std::chrono::steady_clock::now();
        for (int i = 0; i < 5; i++)
        {
            if (detector)
                detector.release();
            detector = std::make_unique<DlibFaceDetector>();

        }
        end = std::chrono::steady_clock::now();
        int avgDetectorInitLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(5);


        // Face Detection
        begin = std::chrono::steady_clock::now();
        for (int i = 0; i < numTests; i++)
        {
            is_valid = detector->find_primary_face_ultraFace(frame, face_shape_vector, live_capture->downscaling);
        }
        end = std::chrono::steady_clock::now();
        int avgFaceDetectionLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(numTests);



        // Landmark detection
        begin = std::chrono::steady_clock::now();
        for (int i = 0; i < numTests; i++)
        {
            is_valid = detector->landmarksToRects(face_shape_vector, rectangles);
        }
        end = std::chrono::steady_clock::now();
        int avgLandmarkDetectionLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(numTests);


        // Generate face eye images
        begin = std::chrono::steady_clock::now();
        for (int i = 0; i < numTests; i++)
        {
            detector->generate_face_eye_images(frame, rectangles, roi_images);
        }
        end = std::chrono::steady_clock::now();
        int avgROICroppingLatency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / static_cast<float>(numTests);


        int avgLatency_ms = avgDetectorInitLatency_ms
            + avgFaceDetectionLatency_ms
            + avgLandmarkDetectionLatency_ms
            + avgROICroppingLatency_ms;

        return avgLatency_ms;
    }

};


