#pragma once
#include <dlib/opencv.h> 
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#pragma comment(lib, "dlib19.21.0_debug_64bit_msvc1928.lib") 

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
    const int SKIP_FRAMES = 2;

    int frame_count = 0;
    std::vector<dlib::rectangle> face_rectangles;

public:
    DlibFaceDetector() {
        detector = dlib::get_frontal_face_detector();
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

    bool find_primary_face(cv::Mat inputImage, std::vector<cv::Point2f>& landmarks, cv::Size downscaling) {
        bool is_valid = false;

        // Check for invalid input
        if (!inputImage.data) {
            LOG_ERROR("Image is empty.");
        }

        // Convert mat to dlib's image format
        dlib::cv_image<dlib::bgr_pixel> inputImage_dlib(inputImage);

        //// Full frame face detection
        //face_rectangles = detector(inputImage_dlib, 0);
        //dlib::full_object_detection shape;
        //if (face_rectangles.size() == 1) {
        //    is_valid = true;
        //    dlib::rectangle rect = face_rectangles[0];
        //    shape = predictor(inputImage_dlib, rect);
        //}

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

    //template <typename T>
    std::vector<cv::Point2f> slice(std::vector<cv::Point2f> v, std::tuple<int, int> regionBounds)
    {
        return std::vector<cv::Point2f>(v.begin() + std::get<0>(regionBounds), v.begin() + std::get<1>(regionBounds) + 1);
    }

    // TODO: Test the calibration logic to work as expected
    cv::RotatedRect calibrate(cv::RotatedRect rotatedRect) {

        // get the parameters of the rotated rectangle
        auto angle = rotatedRect.angle;
        auto size = rotatedRect.size;
        auto center = rotatedRect.center;

        //// The function minAreaRect seems to give angles ranging in(-90, 0].
        //// This is based on the long edge of the rectangle
        //if (angle < -45) {
        //    angle = 90 + angle;
        //    size = cv::Size2f(size.height, size.width);//Swap values of (W,H)
        //}

        return cv::RotatedRect(center, size, angle);//center, size, angle
    }

    //TODO: Implement
    bool check_negative_coordinates(cv::RotatedRect) {
        return true;
    }

    bool landmarksToRects(std::vector<cv::Point2f> face_shape_vector, std::vector<cv::RotatedRect>& rectangles) {
        cv::RotatedRect face_rect = { {0, 0}, {0, 0}, 0 };
        cv::RotatedRect left_eye_rect = { {0, 0}, {0, 0}, 0 };
        cv::RotatedRect right_eye_rect = { {0, 0}, {0, 0}, 0 };


        auto left_eye_shape_vector = slice(face_shape_vector, FACIAL_LANDMARKS_IDXS["left_eye"]);
        auto right_eye_shape_vector = slice(face_shape_vector, FACIAL_LANDMARKS_IDXS["right_eye"]);

        face_rect = calibrate(cv::minAreaRect(face_shape_vector));
        left_eye_rect = calibrate(cv::minAreaRect(left_eye_shape_vector));
        right_eye_rect = calibrate(cv::minAreaRect(right_eye_shape_vector));

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
        auto size = rotatedRect.size;
        auto center = rotatedRect.center;

        // TODO: See if we need to remove 10px padding 
        // get a square crop of the detected region with 10px padding
        auto max_dim = cv::max(size.height, size.width);
        size = cv::Size2f(max_dim + 10, max_dim + 10);

        // get row and col num in img
        int height = img.size().height;//img.rows()
        int width = img.size().width;//img.cols()

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

    cv::Mat generate_grid(cv::Mat webcamImage, cv::RotatedRect face_rect) {
        cv::Mat image = webcamImage.clone();
        image.setTo(cv::Scalar(255, 255, 255));// fill with white 
        //cv::Mat mat = cv::Mat(webcamImage, 100, CV_8UC3, cv::Scalar(255, 255, 255));

        // Extract 4 corner vertices 
        cv::Point2f vertices2f[4];
        face_rect.points(vertices2f);
        // Convert them so we can use them in a fillConvexPoly
        cv::Point vertices[4];
        for (int i = 0; i < 4; ++i) {
            vertices[i] = vertices2f[i];
        }
        // Now we can fill the rotated rectangle with our specified color
        cv::fillConvexPoly(image, vertices, 4, cv::Scalar(0, 0, 0) /*Black*/);

        return image;
    }

    void generate_face_eye_images(cv::Mat webcam_image, std::vector<cv::RotatedRect> rectangles, std::vector<cv::Mat>& roi_images) {
        auto face_rect = rectangles[0];
        auto left_eye_rect = rectangles[1];
        auto right_eye_rect = rectangles[2];

        cv::Mat face_image = crop_rect(webcam_image, face_rect);
        cv::Mat left_eye_image = crop_rect(webcam_image, left_eye_rect);
        cv::Mat right_eye_image = crop_rect(webcam_image, right_eye_rect);
        cv::Mat face_grid_image = generate_grid(webcam_image, face_rect);

        cv::resize(face_image, face_image, cv::Size(224, 224), cv::InterpolationFlags::INTER_AREA);
        cv::resize(left_eye_image, left_eye_image, cv::Size(224, 224), cv::InterpolationFlags::INTER_AREA);
        cv::resize(right_eye_image, right_eye_image, cv::Size(224, 224), cv::InterpolationFlags::INTER_AREA);
        cv::resize(face_grid_image, face_grid_image, cv::Size(224, 224), cv::InterpolationFlags::INTER_AREA);

        roi_images.clear();
        roi_images.push_back(face_image);
        roi_images.push_back(left_eye_image);
        roi_images.push_back(right_eye_image);
        roi_images.push_back(face_grid_image);

        return;
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
        cv::Scalar color1 = cv::Scalar(0, 0, 255);
        cv::Scalar color2 = cv::Scalar(255, 0, 0);
        for (auto& rotatedRect : rectangles) {

            // Extract 4 corner vertices and draw lines to join them
            cv::Point2f vertices2f[4];
            rotatedRect.points(vertices2f);
            for (int i = 0; i < 4; i++) {
                cv::line(image, vertices2f[i], vertices2f[(i + 1) % 4], color1, 2);
            }

            //// Convert them so we can use them in a fillConvexPoly
            //cv::Point vertices[4];
            //for (int i = 0; i < 4; ++i) {
            //    vertices[i] = vertices2f[i];
            //}
            //// Now we can fill the rotated rectangle with our specified color
            //cv::fillConvexPoly(webcamImage, vertices, 4, color1);
        }
        return image;
    }

    void show_image(cv::String windowTitle, cv::Mat image) {
        cv::imshow(windowTitle, image);
        cv::waitKey(1);
    }

    void show_ROI_images(std::vector<cv::Mat> roi_images) {
        cv::Mat out;
        cv::hconcat(roi_images, out);
        show_image("ROI", out);
    }

    std::vector<cv::Mat> ROIExtraction(cv::Mat webcamImage, cv::Size downscaling) {

        // apply ROI extraction here
        std::vector<cv::Point2f> face_shape_vector;
        std::vector<cv::RotatedRect> rectangles;
        std::vector<cv::Mat> roi_images;
        bool is_valid;


        is_valid = find_primary_face(webcamImage, face_shape_vector, downscaling);
        if (is_valid) {
            is_valid = landmarksToRects(face_shape_vector, rectangles);
            if (is_valid) {
                //show_image("Rectangles", draw_rectangles(draw_landmarks(webcamImage, face_shape_vector), rectangles));
                generate_face_eye_images(webcamImage, rectangles, roi_images);
                //show_ROI_images(roi_images);
            }
        }
        return roi_images;
    }
};

