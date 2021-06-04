#pragma once
#include "framework.h";

enum CALIBRATION_TYPE { DELAUNAY, LINEAR_RBF };

class Calibrator {
protected:
    bool active = false;
    cv::Rect rect;
    std::vector<cv::Point2f> actual_coordinates = std::vector<cv::Point2f>();
    std::vector<cv::Point2f> predicted_coordinates = std::vector<cv::Point2f>();
    const wchar_t* calibrationModelPath = L"assets/calibrationModel.txt";

public:
    Calibrator() {}
    virtual ~Calibrator() {}

    void setActive(bool state) {
        this->active = state;
    }
    bool isActive() {
        return this->active;
    }

    bool save() {
        return this->serialize(this->calibrationModelPath);
    }

    bool load() {
        return this->deserialize(this->calibrationModelPath);
    }

    virtual void reset() = 0;
    virtual void add(cv::Point2f actualPt, cv::Point2f predictedPt, bool remap=true) = 0;
    virtual void add(std::vector<cv::Point2f> actualPts, std::vector<cv::Point2f> predictedPts, bool remap=true) = 0;
    virtual cv::Point2f evaluate(cv::Point2f predictedPt) = 0;
    virtual void drawDistortionMap() = 0;
    virtual bool serialize(const wchar_t* path) = 0;
    virtual bool deserialize(const wchar_t* path) = 0;
    
};

