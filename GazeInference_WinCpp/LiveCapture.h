#pragma once
#include "framework.h"
#include "cv_constants.h"
#include <queue>


enum STATE { DORMANT, RUNNING, INACTIVE};

class LiveCapture
{
private:
    /* Config Values */
    int CAPTURE_DEVICE_ID = 0; // Front camera
    int FRAME_RATE = 30;
    cv::Size RESOLUTION = cv::Size(1280, 720);
    const int buffer_length = 30;
    std::queue<cv::Mat> frame_queue = std::queue<cv::Mat>();
    std::thread frame_grabber_thread;
    int state = STATE::INACTIVE;
    //int state = STATE::DORMANT;
    
    const std::vector<cv::Size> CommonResolutions = {
        cv::Size(320, 240),
        cv::Size(640, 480),
        cv::Size(1280, 720),
        cv::Size(1920, 1080),
        cv::Size(3840, 2160),
        cv::Size(4096, 2160)
    };

public:
    /* member variables */
    cv::VideoCapture capture;
    cv::String window_name = "Camera Feed";
    cv::Size downscaling = cv::Size(1.0, 1.0);
    cv::Size active_resolution;
    int active_frame_rate;

public:
    LiveCapture()
    {
    
    }

    LiveCapture(int capture_device_id, int frame_rate, cv::Size resolution)
        : CAPTURE_DEVICE_ID{ capture_device_id }, 
        FRAME_RATE{ frame_rate },
        RESOLUTION{resolution}
    {

    }

    ~LiveCapture() 
    {
        // Cleanup 
    }

    int getFrontCameraId()
    {
        std::vector<double> frameDims;
        int maxID = 2; 
        int idx = 0;
        while (idx < maxID) {
            cv::VideoCapture cap = cv::VideoCapture(idx); // open the camera
            if (cap.isOpened()) {
                cap.set(cv::CAP_PROP_FRAME_WIDTH, 10000);
                frameDims[idx] = cap.get(cv::CAP_PROP_FRAME_WIDTH);
            }
            cap.release();
            idx++;
        }

        if (frameDims[0] <= frameDims[1]) {
            return 0;
        }
        else {
            return 1;
        }
    }

    
    std::vector<int> enumerateCaptureDevices()
    {
        std::vector<int> camIdx;

        struct CapDriver {
            int enumValue; std::string enumName; std::string comment;
        };

        // list of all CAP drivers (see highgui_c.h)
        std::vector<CapDriver> drivers;
        drivers.push_back({ CV_CAP_MIL, "CV_CAP_MIL", "MIL proprietary drivers" });
        drivers.push_back({ CV_CAP_VFW, "CV_CAP_VFW", "platform native" });
        drivers.push_back({ CV_CAP_FIREWARE, "CV_CAP_FIREWARE", "IEEE 1394 drivers" });
        drivers.push_back({ CV_CAP_STEREO, "CV_CAP_STEREO", "TYZX proprietary drivers" });
        drivers.push_back({ CV_CAP_QT, "CV_CAP_QT", "QuickTime" });
        drivers.push_back({ CV_CAP_UNICAP, "CV_CAP_UNICAP", "Unicap drivers" });
        drivers.push_back({ CV_CAP_DSHOW, "CV_CAP_DSHOW", "DirectShow (via videoInput)" });
        drivers.push_back({ CV_CAP_MSMF, "CV_CAP_MSMF", "Microsoft Media Foundation (via videoInput)" });
        drivers.push_back({ CV_CAP_PVAPI, "CV_CAP_PVAPI", "PvAPI, Prosilica GigE SDK" });
        drivers.push_back({ CV_CAP_OPENNI, "CV_CAP_OPENNI", "OpenNI (for Kinect)" });
        drivers.push_back({ CV_CAP_OPENNI_ASUS, "CV_CAP_OPENNI_ASUS", "OpenNI (for Asus Xtion)" });
        drivers.push_back({ CV_CAP_ANDROID, "CV_CAP_ANDROID", "Android" });
        drivers.push_back({ CV_CAP_ANDROID_BACK, "CV_CAP_ANDROID_BACK", "Android back camera" });
        drivers.push_back({ CV_CAP_ANDROID_FRONT, "CV_CAP_ANDROID_FRONT","Android front camera" });
        drivers.push_back({ CV_CAP_XIAPI, "CV_CAP_XIAPI", "XIMEA Camera API" });
        drivers.push_back({ CV_CAP_AVFOUNDATION, "CV_CAP_AVFOUNDATION", "AVFoundation framework for iOS" });
        drivers.push_back({ CV_CAP_GIGANETIX, "CV_CAP_GIGANETIX", "Smartek Giganetix GigEVisionSDK" });
        drivers.push_back({ CV_CAP_INTELPERC, "CV_CAP_INTELPERC", "Intel Perceptual Computing SDK" });

        std::string winName, driverName, driverComment;
        int driverEnum;
        cv::Mat frame;
        bool found;
        std::map<std::string, std::string> statusMap;

        for (int drv = 0; drv < drivers.size(); drv++)
        {
            driverName = drivers[drv].enumName;
            driverEnum = drivers[drv].enumValue;
            driverComment = drivers[drv].comment;
            found = false;

            int maxID = 100; //100 IDs between drivers
            if (driverEnum == CV_CAP_VFW)
                maxID = 10; //VWF opens same camera after 10 ?!?
            else if (driverEnum == CV_CAP_ANDROID)
                maxID = 98; //98 and 99 are front and back cam
            else if ((driverEnum == CV_CAP_ANDROID_FRONT) || (driverEnum == CV_CAP_ANDROID_BACK))
                maxID = 1;

            for (int idx = 0; idx < maxID; idx++)
            {
                cv::VideoCapture cap(driverEnum + idx);  // open the camera
                if (cap.isOpened())                  // check if we succeeded
                {

                    LOG_VERBOSE("%s %d %d\n", driverName.c_str(), driverEnum, idx);
                    found = true;
                    camIdx.push_back(driverEnum + idx);  // vector of all available cameras
                    cap >> frame;
                    if (frame.empty()) {
                        statusMap.insert(std::pair<std::string, std::string>(driverName.c_str(), "[PASS] [FAIL]"));
                    }
                    else {
                        statusMap.insert(std::pair<std::string, std::string>(driverName.c_str(), "[PASS] [PASS]"));
                        // display the frame
                         //imshow(driverName + "+" + std::to_string(idx), frame); cv::waitKey(1);

                    }
                }
                cap.release();
            }
            if (!found) {
                statusMap.insert(std::pair<std::string, std::string>(driverName.c_str(), "[FAIL]"));
            }
        }

        std::map<std::string, std::string>::iterator it;
        LOG_VERBOSE("%20s\t%s\n", "====== DRIVER NAME", "[OPEN] [FRAME] ======");
        for (it = statusMap.begin(); it != statusMap.end(); ++it) {
            LOG_VERBOSE("%20s\t%s\n", it->first.c_str(), it->second.c_str());
        }

        // Convert to comma-separated string
        std::ostringstream activeIndices;
        std::copy(camIdx.begin(), camIdx.end(), std::ostream_iterator<int>(activeIndices, ","));
        LOG_VERBOSE("%d camera IDs has been found: %s\n", camIdx.size(), activeIndices.str().c_str());

        return camIdx;
    }

    std::vector<cv::Size> enumerateSupportedResolutions(cv::VideoCapture capture)
    {
        std::vector<cv::Size> supportedVideoResolutions;
        int nbTests = sizeof(CommonResolutions) / sizeof(CommonResolutions[0]);
        for (int i = 0; i < nbTests; i++) {
            cv::Size test = CommonResolutions[i];

            // try to set resolution
            capture.set(cv::CAP_PROP_FRAME_WIDTH, test.width);
            capture.set(cv::CAP_PROP_FRAME_HEIGHT, test.height);
            double width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
            double height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);

            if (test.width == width && test.height == height) {
                supportedVideoResolutions.push_back(test);
            }
        }
        return supportedVideoResolutions;
    }

    void open() {
        // TODO: Infuture use the enumerate method to get the captureDeviceId

        //std::vector<int> captureDevices = enumerateCaptureDevices();
        //capture = cv::VideoCapture(captureDevices[0]);
        //std::vector<cv::Size> supportedResolutions = enumerateSupportedResolutions(capture);

        // open camera for video stream
        capture = cv::VideoCapture(CAPTURE_DEVICE_ID);

        if (!capture.isOpened())
        {
            LOG_ERROR("Cannot open the video camera.");
        }
        else
        {
            // Set desired capture settings
            set_resolution(RESOLUTION);
            set_frame_rate(FRAME_RATE);
            //cv::namedWindow(window_name, cv::WINDOW_NORMAL); //create a window

            if (state != STATE::INACTIVE) {
                // initialize the frame_grabber_thread
                frame_grabber_thread = std::thread(&LiveCapture::grabFrame, this);
            }
        }
    }

    void close() {
        if (state == STATE::RUNNING) {
            state = STATE::DORMANT;
            frame_grabber_thread.join();
        }
        capture.release();
    }

    bool is_open() {
        return capture.isOpened();
        //return capture.isOpened() && state == STATE::RUNNING;
    }

    double set_frame_rate(int fps) {
        capture.set(cv::CAP_PROP_FPS, fps);
        active_frame_rate = capture.get(cv::CAP_PROP_FPS);
        return active_frame_rate;
    }
    
    cv::Size set_resolution(cv::Size size) {
        return set_resolution(size.width, size.height);
    }

    cv::Size set_resolution(int width, int height) {
        capture.set(cv::CAP_PROP_FRAME_WIDTH, width);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, height);

        // Update downscaling factors for dlib detector
        active_resolution = cv::Size(capture.get(cv::CAP_PROP_FRAME_WIDTH), capture.get(cv::CAP_PROP_FRAME_HEIGHT));
        downscaling.width = active_resolution.width / CommonResolutions[0].width;
        downscaling.height = active_resolution.height / CommonResolutions[0].height;
        return active_resolution;
    }

    
    bool getFrameFromImagePath(std::string imageFilepath, cv::Mat& frame) {
        frame = cv::imread(imageFilepath, cv::ImreadModes::IMREAD_COLOR);
        if (!frame.empty()) {
            return true;
        }
        else
            return false;
    }

    bool getFrame(cv::Mat& frame) {
        if (state == STATE::INACTIVE) {
            return capture.read(frame); // read a new frame from video 
        }
        else if (state == STATE::RUNNING) {
            if (frame_queue.size() > 0) {
                frame = frame_queue.front();
                frame_queue.pop();
                return (!frame.empty());
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }

    void grabFrame() {
        cv::Mat frame;
        state = STATE::RUNNING;
        // to stop the thread state could be set DORMANT outside
        while (state == STATE::RUNNING) {
            capture.read(frame);
            // If queue is full remove a frame from the front
            if (frame_queue.size() == buffer_length) {
                frame_queue.pop();
            }

            // push at the back of the queue
            frame_queue.push(frame);
            //LOG_DEBUG("Queue: %d\n", frame_queue.size());
        }
    }

    double getPropertyValue(cv::VideoCaptureProperties property) {
        return capture.get(property);
    }

    void showRawInput(cv::Mat frame) {
        //show the frame in the created window
        cv::imshow(window_name, frame);
        cv::waitKey(1);
    }

};


