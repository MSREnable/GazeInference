#pragma once
#include "Model.h"
#include "DlibFaceDetector.h"
#include "LiveCapture.h"
#include "DelaunayCalibrator.h"
#include "EyeGazeIoctlLibrary.h"
#pragma comment(lib, "EyeGazeIoctlLibrary.lib")
#include "cam2screen.h"

#include <chrono>
#include <ctime>


// ITracker Model
class ITrackerModel : public Model
{
private:
    cv::Mat frame;
    std::tuple<std::string, float> result = std::tuple<std::string, float>("Unknown", 0.0f);
    std::unique_ptr<LiveCapture> live_capture;
    std::unique_ptr<DlibFaceDetector> detector;
    std::unique_ptr<DelaunayCalibrator> calibrator;

    std::chrono::steady_clock::time_point epoch;
    double timestamp_ms = -1;
    int frame_count = 0;
    std::thread frame_process_thread;
    std::thread inference_thread;

    FLOAT xMonitorRatio;
    FLOAT yMonitorRatio;
    POINT mousePoint;
    int screenWidth = GetPrimaryMonitorWidthUm();
    int screenHeight = GetPrimaryMonitorHeightUm();
    bool updateMesh = false;

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

        InitializeEyeGaze();
        
        // Initialize live capture
        live_capture = std::make_unique<LiveCapture>();
        live_capture->open();

        initCalibrator();

        RECT desktopRect;
        HWND desktopHwnd = GetDesktopWindow();
        GetWindowRect(desktopHwnd, &desktopRect);
        // screen size to pixel ratio
        xMonitorRatio = (FLOAT)GetPrimaryMonitorWidthUm() / (FLOAT)desktopRect.right;
        yMonitorRatio = (FLOAT)GetPrimaryMonitorHeightUm() / (FLOAT)desktopRect.bottom;

        return ( live_capture && detector);
    }

    void initCalibrator() {

        //int W = screenWidth;
        //int H = screenHeight;

        //std::vector<cv::Point2f> actual_coordinates{ cv::Point2f(0, 0), cv::Point2f(W, 0), cv::Point2f(W, H), cv::Point2f(0, H), cv::Point2f(W/2, H/2) };
        //std::vector<cv::Point2f> predicted_coordinates{ cv::Point2f(0, 0), cv::Point2f(W, 0), cv::Point2f(W, H), cv::Point2f(0, H), cv::Point2f(W/2, H/2) };

        //////std::vector<cv::Point2f> actual_coordinates{ cv::Point2f(10, 10), cv::Point2f(W - 10, 10), cv::Point2f(W - 10, H - 10), cv::Point2f(10, H - 10), cv::Point2f((W - 10) / 2, (H - 10) / 2), cv::Point2f((W - 10) / 3, (H - 10) / 4), cv::Point2f(2 * (W - 10) / 3, 3 * (H - 10) / 4) };
        //////std::vector<cv::Point2f> predicted_coordinates{ cv::Point2f(10 + 30, 10 + 90), cv::Point2f(W - 10 + 100, 10 + 20), cv::Point2f(W - 10 - 60, H - 10 - 20), cv::Point2f(10 + 30, H - 10 - 130), cv::Point2f((W - 10) / 2 - 40, (H - 10) / 2 + 160), cv::Point2f((W - 10) / 3 + 30, (H - 10) / 4 - 20), cv::Point2f(2 * (W - 10) / 3 + 110, 3 * (H - 10) / 4 - 30) };


        //// Screen size (can use desktopRect as well)
        //cv::Rect rect = cv::Rect(0, 0, screenWidth, screenHeight);
        //calibrator = std::make_unique<DelaunayCalibrator>(rect, actual_coordinates, predicted_coordinates);

        // Screen size (can use desktopRect as well)
        cv::Rect rect = cv::Rect(0, 0, screenWidth, screenHeight);
        calibrator = std::make_unique<DelaunayCalibrator>(rect, std::vector<cv::Point2f>(), std::vector<cv::Point2f>());
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
        // frame in BGR and roi_frames YCbCr
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

    //void addCalibrationPoint() {
    //    this->updateMesh = true;
    //}

    std::vector<float> processOutput() {
        std::vector<float> coordinates = outputs[0].values;
        float x = coordinates[0];
        float y = coordinates[1];
        LOG_DEBUG("x=%.2f, y=%.2f\n", x, y);

        int xMouse;
        int yMouse;


        if (GetCursorPos(&mousePoint))
        {
            // mouse is returned in pixels, need to convert to um
            xMouse = (INT32)(mousePoint.x * xMonitorRatio);
            yMouse = (INT32)(mousePoint.y * yMonitorRatio);
        }

        // Convert to screen coordinates
        cv::Point point = cam2screen(x, y, screenWidth, screenHeight);

        cv::Point calibratedPoint;

        //#define VK_LBUTTON        0x01
        //#define VK_RBUTTON        0x02
        //#define VK_CANCEL         0x03
        //#define VK_MBUTTON        0x04 
        // Left button Down
        if (GetAsyncKeyState(VK_RBUTTON) != 0){
            // collect more data points for calibration
            calibrator->add(cv::Point(xMouse, yMouse), point);
            //this->updateMesh = false;
        }

        // Calibrate for distortion
        calibratedPoint = calibrator->calibrate(point);
        LOG_DEBUG("Mouse (%d, %d) | Predicted (%d, %d) | Calibrated (%d, %d)\n", xMouse, yMouse, point.x, point.y, calibratedPoint.x, calibratedPoint.y);
        calibrator->drawDelaunayMap();

        // Send to GazeHID
        SendGazeReportUm(calibratedPoint.x, calibratedPoint.y, 0);

        return coordinates;
    }

    void processFrame() {
        bool is_valid;
        int i = 0;
        while (true) { 
            i++;
            is_valid = getFrame(); //reads a new frame
            if (!is_valid)
                continue;
            is_valid = applyTransformations();
            if (!is_valid)
                continue;
            fillInputTensor();
            run();
            processOutput();
        }
    }

    void runInference() {
        frame_process_thread = std::thread(&ITrackerModel::processFrame, this);
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


