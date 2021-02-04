#include "framework.h"


template <typename T>
static void softmax(T& input) {
    float rowmax = *std::max_element(input.begin(), input.end());
    std::vector<float> y(input.size());
    float sum = 0.0f;
    for (size_t i = 0; i != input.size(); ++i) {
        sum += y[i] = std::exp(input[i] - rowmax);
    }
    for (size_t i = 0; i != input.size(); ++i) {
        input[i] = y[i] / sum;
    }
}


template <typename T>
T vectorProduct(const std::vector<T>& v)
{
    return accumulate(v.begin(), v.end(), 1, std::multiplies<T>());
}


//byte[][] getMultiChannelArray(Mat m) {
//    //first index is pixel, second index is channel
//    int numChannels = m.channels();//is 3 for 8UC3 (e.g. RGB)
//    int frameSize = m.rows() * m.cols();
//    byte[] byteBuffer = new byte[frameSize * numChannels];
//    m.get(0, 0, byteBuffer);
//
//    //write to separate R,G,B arrays
//    byte[][] out = new byte[frameSize][numChannels];
//    for (int p = 0, i = 0; p < frameSize; p++) {
//        for (int n = 0; n < numChannels; n++, i++) {
//            out[p][n] = byteBuffer[i];
//        }
//    }
//    return out;
//}

/* Gets the top K results */
std::vector<std::tuple<std::string, float>> getTopK(std::vector<float> results, std::vector<std::string> labels, int k) {
    // Find the top k probabilities
    std::vector<float> topProbabilities(k);
    std::vector<float> topProbabilityLabelIndexes(k);
    // SqueezeNet returns a list of 1000 options, with probabilities for each, loop through all
    for (uint32_t i = 0; i < results.size(); i++) {
        // is it one of the top k?
        for (int j = 0; j < k; j++) {
            if (results[i] > topProbabilities[j]) {
                topProbabilityLabelIndexes[j] = i;
                topProbabilities[j] = results[i];
                break;
            }
        }
    }

    // Collect the top K the result
    std::vector<std::tuple<std::string, float>> topK(k);
    for (int i = 0; i < k; i++) {
        topK[i] = std::make_tuple(labels[topProbabilityLabelIndexes[i]], topProbabilities[i]);
    }

    return topK;
}


std::vector<std::string> readLabels(std::string& labelFilepath)
{
    std::vector<std::string> labels;
    std::string line;
    std::ifstream fp(labelFilepath);
    while (std::getline(fp, line))
    {
        labels.push_back(line);
    }
    return labels;
}

std::vector<std::string> loadLabels(std::string labelFilepath)
{
    std::vector<std::string> labels;
    // Parse labels from labels file.  We know the file's entries are already sorted in order.
    std::ifstream labelFile{ labelFilepath, std::ifstream::in };
    if (labelFile.fail())
    {
        printf("failed to load the %s file.  Make sure it exists in the same folder as the app\r\n", labelFilepath.c_str());
        exit(EXIT_FAILURE);
    }

    std::string line;
    while (std::getline(labelFile, line, ','))
    {
        int labelValue = atoi(line.c_str());
        if (labelValue >= labels.size())
        {
            labels.resize(labelValue + 1);
        }
        std::getline(labelFile, line);
        labels[labelValue] = line;
    }
    return labels;
}

//void Run_with_WebCAM() {
//    std::string WindowName = "WebCam_Example";
//    cv::VideoCapture webcam;
//    webcam.open(0);
//    cv::Mat m_frame;
//    if (webcam.isOpened()) {
//        CvCapture* capture = NULL;
//        //capture = cvCaptureFromCAM(camera_index);
//        // create a window
//        //cvNamedWindow(WindowName.c_str(), CV_WINDOW_AUTOSIZE);
//        //IplImage* frame;
//        //while (1) {
//        //    // update frame and display it:
//        //    webcam >> m_frame;
//
//        //    // convert captured frame to a IplImage
//        //    frame = new IplImage(m_frame);
//        //    if (!frame) break;
//        //    cvShowImage(WindowName.c_str(), frame);
//
//        //    // Do some processing...
//
//        //    delete frame;
//
//        //    // some abort condition...
//        //}
//        //// release memory and destroy all windows
//        //cvDestroyWindow(WindowName.c_str());
//    }
//}


void liveCameraFeed() {
    Mat frame;

    // open camera for video stream
    VideoCapture capture = VideoCapture(0);
    //// open a video file for capture
    //VideoCapture capture("D:/My OpenCV Website/A Herd of Deer Running.mp4");

    // if not success, exit program
    if (!capture.isOpened())
    {
        //"Cannot open the video camera" 
        cin.get(); //wait for any key press
    }

    //get the frames rate of the video
    double fps = capture.get(CAP_PROP_FPS);
    double width = capture.get(CAP_PROP_FRAME_WIDTH);
    double height = capture.get(CAP_PROP_FRAME_HEIGHT);

    String window_name = "Camera Feed";
    namedWindow(window_name, WINDOW_NORMAL); //create a window

    while (true)
    {
        bool bSuccess = capture.read(frame); // read a new frame from video 

        // Breaking the while loop at the end of the video
        if (bSuccess == false)
        {
            //"Video camera disconnected"
            break;
        }

        //show the frame in the created window
        imshow(window_name, frame);

        //wait for for 10 ms until any key is pressed.  
        //If the 'Esc' key is pressed, break the while loop.
        //If the any other key is pressed, continue the loop 
        //If any key is not pressed withing 10 ms, continue the loop
        if (waitKey(10) == 27)
        {
            cout << "Esc key is pressed by user. Stoppig the video" << endl;
            break;
        }
    }
}


void readImage(string imageFilePath) {
    Mat image = imread(imageFilePath);
}