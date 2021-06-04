#pragma once
#include "Model.h"
#include "DlibFaceDetector.h"
#include "LiveCapture.h"
#include "cam2screen.h"
#include <chrono>
#include <ctime>
#include "LinearRBFCalibrator.h"
#include "DelaunayCalibrator.h"


#define USE_EYECONTROL false
#define USE_CALIBRATION false
#define USE_VERBOSE false

#ifdef USE_EYECONTROL
#include "EyeGazeIoctlLibrary.h"
#pragma comment(lib, "EyeGazeIoctlLibrary.lib")
#endif


// ITracker Model
class ITrackerModel : public Model
{
private:
    cv::Mat frame;
    std::tuple<std::string, float> result = std::tuple<std::string, float>("Unknown", 0.0f);
    std::unique_ptr<LiveCapture> live_capture;
    std::unique_ptr<DlibFaceDetector> detector;
    std::unique_ptr<Calibrator> calibrator;
    int calibration_type = CALIBRATION_TYPE::DELAUNAY;

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
        // Screen size (can use desktopRect as well)
        cv::Rect rect = cv::Rect(0, 0, screenWidth, screenHeight);
        if (this->calibration_type == CALIBRATION_TYPE::LINEAR_RBF) {
            this->calibrator = std::make_unique<LinearRBFCalibrator>(rect);
        }
        else if(this->calibration_type == CALIBRATION_TYPE::DELAUNAY) {
            this->calibrator = std::make_unique<DelaunayCalibrator>(rect);
        }
        //this->calibrator->load();
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

    cv::Point processOutput() {
        cv::Point predictedPoint = cv::Point(outputs[0].values[0], outputs[0].values[1]);
        LOG_DEBUG("x=%.2f, y=%.2f\n", predictedPoint.x, predictedPoint.y);

        // Convert to screen coordinates
        cv::Point point = cam2screen(predictedPoint, screenWidth, screenHeight);

        
#ifdef USE_CALIBRATION
        /*
        * Use the mouse cursor as gaze target to calibrate
        */

        /* Retrieve Mouse Cursor Position */
        int xMouse, yMouse;
        if (GetCursorPos(&mousePoint)) {
            // mouse is returned in pixels, need to convert to um
            xMouse = (INT32)(mousePoint.x * xMonitorRatio);
            yMouse = (INT32)(mousePoint.y * yMonitorRatio);
        }


        /* Use Navigation keys for calibration 
        *
        * Left Key : Deactivate Calibration
        * Right Key : Activate Calibration
        * Down Key : Save Current Calibration Setting
        * Up Key : Load/Reset calibration with existing Settings
        * Mouse Right Button: Add new calibration point
        */
        if (this->calibrator->isActive()) {
            // Disable
            if (GetAsyncKeyState(VK_LEFT) != 0) {
                this->calibrator->setActive(false);
            }

            // Add new point (Right button Down)
            if (GetAsyncKeyState(VK_RBUTTON) != 0) {
                // collect more data points for calibration
                this->calibrator->add(cv::Point(xMouse, yMouse), point);
            }

            // Save Calibration model (down button)
            if (GetAsyncKeyState(VK_DOWN) != 0) {
                this->calibrator->save();
            }

            // Load Calibration model (up button Down)
            if (GetAsyncKeyState(VK_UP) != 0) {
                this->calibrator->load();
            }

            // Reset Calibration model (Delete button)
            if (GetAsyncKeyState(VK_DELETE) != 0) {
                this->calibrator->reset();
            }
        }
        else { // Optionas: Enable Calibration
            if (GetAsyncKeyState(VK_RIGHT) != 0) {
                this->calibrator->setActive(true);
            }
        }
#endif

        cv::Point calibratedPoint;
        if (this->calibrator->isActive()) {
            // Calibrate for distortion
            calibratedPoint = this->calibrator->evaluate(point);
#ifdef USE_VERBOSE
            this->calibrator->drawDistortionMap();
#endif
        }
        else {
            calibratedPoint = point;
        }
        LOG_DEBUG("Predicted (%d, %d) | Calibrated (%d, %d)\n", point.x, point.y, calibratedPoint.x, calibratedPoint.y);

#ifdef USE_EYECONTROL
        // Send to GazeHID
        SendGazeReportUm(calibratedPoint.x, calibratedPoint.y, 0);
#endif

        return calibratedPoint;
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
        cv::Point coordinates_XY;
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
        cv::Point coordinates_XY;
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