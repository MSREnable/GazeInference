#pragma once
#include <dlib/opencv.h> 
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#pragma comment(lib, "dlib19.21.0_debug_64bit_msvc1928.lib") 
#include "UltraFaceNet.h"

template <typename T>
std::vector<T> slice(std::vector<T> v, std::tuple<int, int> regionBounds)
{
    return std::vector<T>(v.begin() + std::get<0>(regionBounds), v.begin() + std::get<1>(regionBounds) + 1);
}

template <typename T>
std::vector<T> slice(std::vector<T> v, int start, int end)
{
    return std::vector<T>(v.begin() + start, v.begin() + end + 1);
}

template <typename T>
std::vector<T> select(std::vector<T> vec, std::initializer_list<std::size_t> indices)
{
    std::vector<T> result_vec = std::vector<T>(indices.size());
    std::transform(indices.begin(), indices.end(), result_vec.begin(), [vec](size_t pos) {return vec[pos]; });
    return result_vec;
}


enum DETECTOR_TYPE {DLIB, ULTRA_FACE, ULTRA_FACE_SLIM};
enum BLURRING { LOW = 11, MEDIUM = 23, HIGH = 37 };
enum NOISE { STATIC, SALT_PEPPER };

cv::Scalar BLUE = cv::Scalar(255, 0, 0);
cv::Scalar GREEN = cv::Scalar(0, 255, 0);
cv::Scalar RED = cv::Scalar(0, 0, 255);
cv::Scalar BLACK = cv::Scalar(0, 0, 0);
cv::Scalar WHITE = cv::Scalar(255, 255, 255);


class DlibFaceDetector {
private:
    dlib::frontal_face_detector detector;
    dlib::shape_predictor predictor;
    std::string predictor_model_path = "assets/shape_predictor_68_face_landmarks.dat";
    std::map<std::string, std::tuple<int, int>> FACIAL_LANDMARKS_IDXS = { {"mouth", {48, 67}},
                                                                            {"inner_mouth", {60, 67}},
                                                                            {"right_eyebrow", {17, 21}},
                                                                            {"left_eyebrow", {22, 26}},
                                                                            {"right_eye", {36, 41}},
                                                                            {"left_eye", {42, 47}},
                                                                            {"nose", {27, 35}},
                                                                            {"jaw", {0, 16}}
    };

    const int IMAGE_WIDTH = 224;
    const int IMAGE_HEIGHT = 224;
    const int SKIP_FRAMES = 1;
    const int blurring = BLURRING::HIGH;
    
    int detector_type = DETECTOR_TYPE::ULTRA_FACE_SLIM;//ULTRA_FACE_SLIM, ULTRA_FACE
    int frame_count = 0;
    std::vector<dlib::rectangle> face_rectangles;
    std::unique_ptr<UltraFaceNet> ultraFaceNet;

public:
    DlibFaceDetector() {
        
        // Initialize face detector
        if (detector_type == DETECTOR_TYPE::DLIB)
            detector = dlib::get_frontal_face_detector();
        else if (detector_type == DETECTOR_TYPE::ULTRA_FACE)
            ultraFaceNet = std::make_unique<UltraFaceNet>(L"assets/version-RFB-320_without_postprocessing.onnx");
        else if (detector_type == DETECTOR_TYPE::ULTRA_FACE_SLIM)
            ultraFaceNet = std::make_unique<UltraFaceNet>(L"assets/version-slim-320_without_postprocessing.onnx");
            
        // initialize landmark detector
        std::async(&DlibFaceDetector::init_predictor, this);
    }

    void init_predictor() {
        dlib::deserialize(predictor_model_path) >> predictor;
    }

    std::vector<cv::Point2f> shape_to_landmarks(dlib::full_object_detection shape)
    {
        std::vector<cv::Point2f> parts;
        for (unsigned long i = 0; i < shape.num_parts(); i++) {
            dlib::point part = shape.part(i);
            parts.push_back(cv::Point2f(part.x(), part.y()));
        }
        return parts;
    }

    // TDODO: Instead of frame count use time difference to run face detector
    // 30fps => 1000ms/30fps => 33ms
    bool find_primary_face_dlib(cv::Mat inputImage, std::vector<cv::Point2f>& landmarks, cv::Size downscaling) {
        bool is_valid = false;

        // Check for invalid input
        if (!inputImage.data) {
            LOG_ERROR("Image is empty.");
        }

        // Convert mat to dlib's image format
        dlib::cv_image<dlib::bgr_pixel> inputImage_dlib(inputImage);

        // Detect faces in every Kth (SKIP_FRAMES) frames
        if (frame_count % SKIP_FRAMES == 0)
        {
            // Resize image for face detection
            cv::Mat downsampledImage;
            cv::cvtColor(inputImage, downsampledImage, cv::COLOR_BGR2GRAY);
            cv::resize(downsampledImage, downsampledImage, cv::Size(), 1.0 / downscaling.width, 1.0 / downscaling.height);
            dlib::cv_image<unsigned char> downsampledImage_dlib(downsampledImage);
            /*dlib::cv_image<dlib::bgr_pixel> downsampledImage_dlib(downsampledImage);*/

            face_rectangles = detector(downsampledImage_dlib);
            //frame_count = 0; //reset frame count 
        }
        frame_count++;

        dlib::full_object_detection shape;
        if (face_rectangles.size() == 1) {
            is_valid = true;
            dlib::rectangle rect(
                (long)(face_rectangles[0].left() * downscaling.width),
                (long)(face_rectangles[0].top() * downscaling.height),
                (long)(face_rectangles[0].right() * downscaling.width),
                (long)(face_rectangles[0].bottom() * downscaling.height)
            );
            shape = predictor(inputImage_dlib, rect);
            landmarks = shape_to_landmarks(shape);
        }
        return is_valid;
    }

    // https://github.com/Linzaer/Ultra-Light-Fast-Generic-Face-Detector-1MB
    // inputImage is BGR
    bool find_primary_face_ultraFace(cv::Mat inputImage, std::vector<cv::Point2f>& landmarks, cv::Size downscaling) {
        bool is_valid = false;

        // Check for invalid input
        if (!inputImage.data) {
            LOG_ERROR("Image is empty.");
        }

        cv::Mat inputImageRGB;
        cv::cvtColor(inputImage, inputImageRGB, cv::ColorConversionCodes::COLOR_BGR2RGB);
        // Detect faces in every Kth (SKIP_FRAMES) frames
        if (frame_count % SKIP_FRAMES == 0)
        {
            // image Resize is handled by ultraface internally 
            face_rectangles = ultraFaceNet->detect_faces(inputImageRGB);
            //frame_count = 0; //reset frame count 
        }
        frame_count++;

        // Convert mat to dlib's image format
        dlib::cv_image<dlib::bgr_pixel> inputImage_dlib(inputImage);
        dlib::full_object_detection shape;
        if (face_rectangles.size() == 1) {
            is_valid = true;
            dlib::rectangle rect = face_rectangles[0];
            shape = predictor(inputImage_dlib, rect);
            landmarks = shape_to_landmarks(shape);
        }
        return is_valid;
    }

    std::vector<dlib::full_object_detection> find_all_faces(cv::Mat inputImage) {

        // Check for invalid input
        if (!inputImage.data) {
            LOG_ERROR("Image is empty.");
        }

        // Convert mat to dlib's bgr pixel
        dlib::cv_image<dlib::bgr_pixel> inputImage_dlib(inputImage);

        // Start detecting
        std::vector<dlib::rectangle> face_rectangles = detector(inputImage_dlib, 0);

        // TODO: Try rect-only face detection and compare speed
        // To get all shapes
        std::vector<dlib::full_object_detection> shapes;
        for (unsigned long j = 0; j < face_rectangles.size(); ++j) {
            dlib::full_object_detection shape = predictor(inputImage_dlib, face_rectangles[j]);
            shapes.push_back(shape);
        }
        return shapes;
    }

    //TODO: Implement
    bool check_negative_coordinates(cv::RotatedRect) {
        return true;
    }


    cv::Size makeSquare(cv::Size size) {
        float length = std::max(size.height, size.width);
        return cv::Size(length, length);
    }

    cv::RotatedRect getSquareBoundingRect(std::vector<cv::Point2f> shape_vector) {
        cv::RotatedRect boundingRect = cv::minAreaRect(shape_vector);
        boundingRect.size = makeSquare(boundingRect.size);
        return boundingRect;
    }

    void calibrate(cv::RotatedRect& srcRect, const cv::RotatedRect& refRect) {
        cv::Point2f src_vertices[4];
        srcRect.points(src_vertices);

        cv::Point2f ref_vertices[4];
        refRect.points(ref_vertices);

        // width is the smaller side. For eyes thats the vertical side.
        cv::Point2f tgt_vertices[4] = { cv::Point2f(0,refRect.size.width), cv::Point2f(0,0), cv::Point2f(refRect.size.height,0), cv::Point2f(refRect.size.height,refRect.size.width) };
        cv::Mat M = cv::getAffineTransform(ref_vertices, tgt_vertices);

        std::vector<cv::Point2f> rect_corners_transformed(4);
        cv::transform(std::vector<cv::Point2f>(std::begin(src_vertices), std::end(src_vertices)), rect_corners_transformed, M);

        // Using the 3-pt constructor fails occasionaaly when transformed rectangle 
        // is not exactly right-angled. We use minAreaRect instead.
        srcRect = cv::minAreaRect(rect_corners_transformed);
        calibrate(srcRect);
    }

    void calibrate(cv::RotatedRect& srcRect) {
        // The function minAreaRect seems to give angles ranging in(-90, 0].
        // This is based on the long edge of the rectangle
        if (srcRect.angle > 45) {
            srcRect.angle = srcRect.angle - 90;
            std::swap(srcRect.size.height, srcRect.size.width);//Swap values of (W,H)
        }
    }

    bool landmarksToRects(std::vector<cv::Point2f> face_shape_vector, std::vector<cv::RotatedRect>& rectangles) {
        cv::RotatedRect face_rect = { {0, 0}, {0, 0}, 0 };
        cv::RotatedRect left_eye_rect = { {0, 0}, {0, 0}, 0 };
        cv::RotatedRect right_eye_rect = { {0, 0}, {0, 0}, 0 };


        auto left_eye_shape_vector = slice(face_shape_vector, FACIAL_LANDMARKS_IDXS["left_eye"]);
        auto right_eye_shape_vector = slice(face_shape_vector, FACIAL_LANDMARKS_IDXS["right_eye"]);

        face_rect = getSquareBoundingRect(face_shape_vector);
        left_eye_rect = cv::minAreaRect(left_eye_shape_vector);
        right_eye_rect = cv::minAreaRect(right_eye_shape_vector);

        // face_rect must be calibrated before eye rects 
        // for relative coordinates
        calibrate(face_rect);
        calibrate(left_eye_rect);
        calibrate(right_eye_rect);

        bool is_valid = check_negative_coordinates(face_rect) &
            check_negative_coordinates(left_eye_rect) &
            check_negative_coordinates(right_eye_rect);

        rectangles.clear();
        rectangles.push_back(face_rect);
        rectangles.push_back(left_eye_rect);
        rectangles.push_back(right_eye_rect);

        return is_valid;
    }

    bool landmarksToRectsEDGE(std::vector<cv::Point2f> face_shape_vector, std::vector<cv::RotatedRect>& rectangles) {
        cv::RotatedRect face_rect = { {0, 0}, {0, 0}, 0 };
        cv::RotatedRect left_eye_rect = { {0, 0}, {0, 0}, 0 };//relative to face_rect
        cv::RotatedRect right_eye_rect = { {0, 0}, {0, 0}, 0 };//relative to face_rect

        auto left_eye_shape_vector = slice(face_shape_vector, FACIAL_LANDMARKS_IDXS["left_eye"]);
        auto right_eye_shape_vector = slice(face_shape_vector, FACIAL_LANDMARKS_IDXS["right_eye"]);

        // For face get a square boundingBox so that relative 
        // calibration of eye rects is valid for square cropped image 
        face_rect = getSquareBoundingRect(face_shape_vector);
        left_eye_rect = cv::minAreaRect(left_eye_shape_vector);
        right_eye_rect = cv::minAreaRect(right_eye_shape_vector);

        // face_rect must be calibrated before eye rects 
        // for relative coordinates
        calibrate(face_rect);
        calibrate(left_eye_rect, face_rect);
        calibrate(right_eye_rect, face_rect);

        bool is_valid = check_negative_coordinates(face_rect) &
            check_negative_coordinates(left_eye_rect) &
            check_negative_coordinates(right_eye_rect);

        rectangles.clear();
        rectangles.push_back(face_rect);
        rectangles.push_back(left_eye_rect);
        rectangles.push_back(right_eye_rect);

        return is_valid;
    }

    cv::Mat crop_rect(cv::Mat img, cv::RotatedRect rotatedRect) {

        // get the parameters of the rotated rectangle
        auto angle = rotatedRect.angle;
        auto size = makeSquare(rotatedRect.size); // get a square crop of the detected region 
        auto center = rotatedRect.center;
        // TODO: Remove 10px padding from the training pipeline 

        // get size of image
        int height = img.size().height;
        int width = img.size().width;

        // calculate the rotation matrix
        cv::Mat M = cv::getRotationMatrix2D(center, angle, 1);
        // rotate the original image
        cv::Mat img_rot;
        cv::warpAffine(img, img_rot, M, cv::Size2f(width, height));

        //now rotated rectangle becomes vertical and we crop it
        cv::Mat img_crop;
        cv::getRectSubPix(img_rot, size, center, img_crop);

        return img_crop;
    }

    cv::Mat generate_grid(cv::Size webcamImageSize, cv::RotatedRect face_rect) {
        cv::Mat image = cv::Mat(webcamImageSize, CV_8UC3, WHITE);// fill with white 

        // Extract 4 corner vertices 
        cv::Point2f vertices2f[4];
        face_rect.points(vertices2f);
        // Convert them so we can use them in a fillConvexPoly
        cv::Point vertices[4];
        for (int i = 0; i < 4; ++i) {
            vertices[i] = vertices2f[i];
        }
        // Now we can fill the rotated rectangle with our specified color
        cv::fillConvexPoly(image, vertices, 4, BLACK);

        return image;
    }

    void fillNoise(cv::Mat& image, const int type) {
        if (type == NOISE::STATIC) {
            for (int i = 0; i < image.rows; i++)
                for (int j = 0; j < image.cols; j++) {
                    image.at<cv::Vec3b>(i, j) = cv::Vec3b(rand() % 255, rand() % 255, rand() % 255);
                }
        }
        else if (type == NOISE::SALT_PEPPER) {
            float threshold = 0.2;// extent of noise [0,1]
            float p;
            for (int i = 0; i < image.rows; i++)
                for (int j = 0; j < image.cols; j++) {
                    p = (float)rand() / RAND_MAX;
                    if (p < threshold / 2) {
                        image.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 0);
                    }
                    else if (p > (1.0 - threshold / 2)) {
                        image.at<cv::Vec3b>(i, j) = cv::Vec3b(255, 255, 255);
                    }
                }
        }
    }

    void privacy_mask(cv::Mat& webcamImage, cv::RotatedRect face_rect) {
        cv::Mat mask = webcamImage.clone();
        //cv::Mat mat = cv::Mat(webcamImage, 100, CV_8UC3, BLACK);

        // Make square
        auto max_dim = cv::max(face_rect.size.height, face_rect.size.width);
        face_rect.size = cv::Size2f(max_dim, max_dim);

        // Extract 4 corner vertices 
        cv::Point2f vertices2f[4];
        face_rect.points(vertices2f);
        // Convert them so we can use them in a fillConvexPoly
        cv::Point vertices[4];
        for (int i = 0; i < 4; ++i) {
            vertices[i] = vertices2f[i];
        }
        // Create a blur mask
        cv::Mat blurMask;
        cv::blur(mask, blurMask, cv::Size(blurring, blurring));
        fillNoise(blurMask, NOISE::SALT_PEPPER);
        cv::fillConvexPoly(blurMask, vertices, 4, BLACK);

        // Create an extraction mask
        mask.setTo(BLACK);// fill with black 
        cv::fillConvexPoly(mask, vertices, 4, WHITE);

        // Apply extraction and blur masks
        cv::bitwise_and(webcamImage, mask, webcamImage);
        cv::bitwise_or(webcamImage, blurMask, webcamImage);
    }

    cv::Mat cvtColor_BRG2YCbCr(cv::Mat inputImageBGR) {
        cv::Mat inputImageYCrCb, inputImageYCbCr;
        cv::cvtColor(inputImageBGR, inputImageYCrCb, cv::ColorConversionCodes::COLOR_BGR2YCrCb);
        cv::Mat channels[3];
        cv::split(inputImageYCrCb, channels);
        std::swap(channels[1], channels[2]);
        cv::merge(channels, 3, inputImageYCbCr);
        return inputImageYCbCr;
    }

    void generate_face_eye_images(cv::Mat webcam_image, std::vector<cv::RotatedRect> rectangles, std::vector<cv::Mat>& roi_images) {
        cv::RotatedRect face_rect = rectangles[0];
        cv::RotatedRect left_eye_rect = rectangles[1];
        cv::RotatedRect right_eye_rect = rectangles[2];

        // Convert to YCbCr
        cv::Mat inputImageYCbCr = cvtColor_BRG2YCbCr(webcam_image);

        cv::Mat face_image = crop_rect(inputImageYCbCr, face_rect);
        cv::Mat left_eye_image = crop_rect(inputImageYCbCr, left_eye_rect);
        cv::Mat right_eye_image = crop_rect(inputImageYCbCr, right_eye_rect);
        cv::Mat face_grid_image = generate_grid(inputImageYCbCr.size(), face_rect);

        roi_images.clear();
        roi_images.push_back(face_image);
        roi_images.push_back(left_eye_image);
        roi_images.push_back(right_eye_image);
        roi_images.push_back(face_grid_image);

        return;
    }

    cv::Mat generate_face_image_EDGE(cv::Mat webcam_image, std::vector<cv::RotatedRect> rectangles) {
        cv::RotatedRect face_rect = rectangles[0];
        return crop_rect(webcam_image, face_rect);
    }

    void generate_face_eye_images_COMPUTE(cv::Size webcamImageSize, cv::Mat face_image, std::vector<cv::RotatedRect> rectangles, std::vector<cv::Mat>& roi_images) {
        cv::RotatedRect face_rect = rectangles[0];
        cv::RotatedRect left_eye_rect = rectangles[1];//relative to face_rect
        cv::RotatedRect right_eye_rect = rectangles[2];//relative to face_rect

        // Convert to YCbCr
        face_image = cvtColor_BRG2YCbCr(face_image);

        cv::Mat left_eye_image = crop_rect(face_image, left_eye_rect);//relative to face_rect
        cv::Mat right_eye_image = crop_rect(face_image, right_eye_rect);//relative to face_rect
        cv::Mat face_grid_image = generate_grid(webcamImageSize, face_rect);
       
        roi_images.clear();
        roi_images.push_back(face_image);
        roi_images.push_back(left_eye_image);
        roi_images.push_back(right_eye_image);
        roi_images.push_back(face_grid_image);

        //cv::Mat testImage = draw_rectangles(face_image, std::vector<cv::RotatedRect>{rectangles[1], rectangles[2]});
        //show_image("test", testImage);

        return;
    }

    void resize_ROI_images(std::vector<cv::Mat>& roi_images) {
        for (int i = 0; i < roi_images.size(); i++) {
            cv::resize(roi_images[i], roi_images[i], cv::Size(224, 224), cv::InterpolationFlags::INTER_AREA);
        }
    }

    cv::Mat draw_landmarks(cv::Mat webcamImage, std::vector<cv::Point2f> face_shape_vector) {
        cv::Mat image = webcamImage.clone();
        for (auto& point : face_shape_vector) {
            cv::circle(image, point, 1, cv::Scalar(0, 0, 255), 1, -1);
        }
        return image;
    }

    cv::Mat draw_rectangles(cv::Mat webcamImage, std::vector<cv::RotatedRect> rectangles) {
        cv::Mat image = webcamImage.clone();
        for (auto& rotatedRect : rectangles) {

            // Extract 4 corner vertices and draw lines to join them
            cv::Point2f vertices2f[4];
            rotatedRect.points(vertices2f);
            cv::circle(image, vertices2f[1]/*topLeft*/, 3, RED, 3);
            for (int i = 0; i < 4; i++) {
                cv::line(image, vertices2f[i], vertices2f[(i + 1) % 4], BLUE, 2);
            }

            //// Convert them so we can use them in a fillConvexPoly
            //cv::Point vertices[4];
            //for (int i = 0; i < 4; ++i) {
            //    vertices[i] = vertices2f[i];
            //}
            //// Now we can fill the rotated rectangle with our specified color
            //cv::fillConvexPoly(webcamImage, vertices2f, 4, BLUE);
        }
        return image;
    }

    void show_image(cv::String windowTitle, cv::Mat image) {
        cv::imshow(windowTitle, image);
        cv::waitKey(1);
    }

    cv::Mat create_grid(std::vector<cv::Mat> images, int n_cols)
    {
        // Define the size of your images
        cv::Size sz = images[0].size();

        // Define correct number of rows, according to columns
        int n_rows = (images.size() / n_cols) + ((images.size() % n_cols) ? 1 : 0);

        // Create a black image with correct size
        cv::Mat3b grid(n_rows * sz.height, n_cols * sz.width, cv::Vec3b(0, 0, 0));

        // For each image
        for (int i = 0; i < images.size(); ++i)
        {
            // Get x,y position in the grid
            int x = (i % n_cols) * sz.width;
            int y = (i / n_cols) * sz.height;

            // Select the roi in the grid
            cv::Rect roi(x, y, sz.width, sz.height);

            // Copy the image into the roi
            images[i].copyTo(grid(roi));
        }
        return grid;
    }

    void show_ROI_extraction(cv::Mat webcamImage, std::vector<cv::Point2f> face_shape_vector, std::vector<cv::RotatedRect> rectangles, std::vector<cv::Mat> roi_images) {

        cv::Mat anchor_image, roi_grid_image, anchored_roi_grid_image;
        roi_grid_image = create_grid(select(roi_images, { 0, 3, 1, 2 }), 2);
        anchor_image = draw_rectangles(draw_landmarks(webcamImage, face_shape_vector), rectangles);

        // Resize anchor image to roi_grid height maintaining the aspect ratio
        float aspect_ratio = roi_grid_image.size().height / float(anchor_image.size().height);
        cv::resize(anchor_image, anchor_image, cv::Size(anchor_image.size().width * aspect_ratio, roi_grid_image.size().height));

        // Horizaontally concatenate the anchor and roi-grid images
        cv::hconcat(anchor_image, roi_grid_image, anchored_roi_grid_image);
        show_image("ROI", anchored_roi_grid_image);
    }

    void show_ROI_extraction_COMPUTE(cv::Mat webcamImage, std::vector<cv::Point2f> face_shape_vector, std::vector<cv::RotatedRect> rectangles, std::vector<cv::Mat> roi_images) {

        cv::Mat anchor_image, roi_grid_image, anchored_roi_grid_image;
        // Draw eye rects on face image
        resize_rectangles(rectangles);
        roi_images[0] = draw_rectangles(roi_images[0], select(rectangles, { 1, 2 }));
        roi_grid_image = create_grid(select(roi_images, { 0, 3, 1, 2 }), 2);
        anchor_image = draw_rectangles(draw_landmarks(webcamImage, face_shape_vector), select(rectangles, { 0 }));

        // Resize anchor image to roi_grid height maintaining the aspect ratio
        float aspect_ratio = roi_grid_image.size().height / float(anchor_image.size().height);
        cv::resize(anchor_image, anchor_image, cv::Size(anchor_image.size().width * aspect_ratio, roi_grid_image.size().height));

        // Horizaontally concatenate the anchor and roi-grid images
        cv::hconcat(anchor_image, roi_grid_image, anchored_roi_grid_image);
        show_image("ROI-COMPUTE", anchored_roi_grid_image);
    }

    void resize_rectangles(std::vector<cv::RotatedRect>& rectangles) {
        cv::Size face_rect_size = rectangles[0].size;
        cv::Size face_crop_size = cv::Size(224, 224);

        cv::Point2f src_vertices[4] = { cv::Point2f(0,face_rect_size.height), cv::Point2f(0,0), cv::Point2f(face_rect_size.width,0), cv::Point2f(face_rect_size.width,face_rect_size.height) };
        cv::Point2f tgt_vertices[4] = { cv::Point2f(0,face_crop_size.height), cv::Point2f(0,0), cv::Point2f(face_crop_size.width,0), cv::Point2f(face_crop_size.width,face_crop_size.height) };
        cv::Mat M = cv::getAffineTransform(src_vertices, tgt_vertices);

        cv::Point2f vertices[4];
        std::vector<cv::Point2f> vertices_transformed(4);
        // Skip face_rect for resize
        for (int i = 1; i < rectangles.size(); i++) {
            rectangles[i].points(vertices);
            cv::transform(std::vector<cv::Point2f>(std::begin(vertices), std::end(vertices)), vertices_transformed, M);
            // Using the 3-pt constructor fails occasionaaly when transformed rectangle 
            // is not exactly right-angled. We use minAreaRect instead.
            rectangles[i] = cv::minAreaRect(vertices_transformed);
        }
    }

    std::vector<cv::Mat> ROIExtraction(cv::Mat webcamImage, cv::Size downscaling) {

        // apply ROI extraction here
        std::vector<cv::Point2f> face_shape_vector;
        std::vector<cv::RotatedRect> rectangles;
        std::vector<cv::Mat> roi_images;
        bool is_valid;

        if (detector_type == DETECTOR_TYPE::DLIB)
            is_valid = find_primary_face_dlib(webcamImage, face_shape_vector, downscaling);
        else
            is_valid = find_primary_face_ultraFace(webcamImage, face_shape_vector, downscaling);

        //if (is_valid) {
        //    is_valid = landmarksToRects(face_shape_vector, rectangles);
        //    if (is_valid) {
        //        generate_face_eye_images(webcamImage, rectangles, roi_images);
        //        resize_ROI_images(roi_images);
        //        show_ROI_extraction(webcamImage, face_shape_vector, rectangles, roi_images);
        //    }
        //}

        // For edge devices
        if (is_valid) {
            is_valid = landmarksToRectsEDGE(face_shape_vector, rectangles);
            if (is_valid) {
                // At edge device
                cv::Mat face_image = generate_face_image_EDGE(webcamImage, rectangles);
                // At compute device - input {webcamImage.size(), face_image (original size), rectangles}
                generate_face_eye_images_COMPUTE(webcamImage.size(), face_image, rectangles, roi_images);
                resize_ROI_images(roi_images);
                privacy_mask(webcamImage, rectangles[0]);
                show_ROI_extraction_COMPUTE(webcamImage, face_shape_vector, rectangles, roi_images);
            }
        }

        for (auto& image : roi_images) {
            image.convertTo(image, CV_32FC3, 1.0 / 255.0);
        }

        return roi_images;
    }
};

