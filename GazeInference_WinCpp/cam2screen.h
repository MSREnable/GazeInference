#pragma once
#include "framework.h"
#include <fstream>

std::map<std::string, float> deviceMetrics = {  {"xCameraToScreenDisplacementInCm", 13.0},
                                                {"yCameraToScreenDisplacementInCm", 0.6},
                                                {"widthScreenInCm", 26.0},
                                                {"heightScreenInCm", 17.35},
                                                {"ppi", 267}
                                            };

cv::Point cam2screen(cv::Point predictedPoint, int widthScreenInPoints, int heightScreenInPoints)
{
    
    float xDisplacementFromCameraInCm = predictedPoint.x;
    float yDisplacementFromCameraInCm = predictedPoint.y;

    float xCameraToScreenDisplacementInCm = deviceMetrics["xCameraToScreenDisplacementInCm"];
    float yCameraToScreenDisplacementInCm = deviceMetrics["yCameraToScreenDisplacementInCm"];
    float widthScreenInCm = deviceMetrics["widthScreenInCm"];
    float heightScreenInCm = deviceMetrics["heightScreenInCm"];

    if (!xCameraToScreenDisplacementInCm || !yCameraToScreenDisplacementInCm ||
        !widthScreenInCm || !heightScreenInCm) {
        return cv::Point(0,0);
    }

    float xScreenInCm;
    float yScreenInCm;
    float xScreenInPoints;
    float yScreenInPoints;
    // Camera Offset and Screen Orientation compensation
    // Camera above screen
    // - Portrait on iOS devices
    // - Landscape on Surface devices
    xScreenInCm = xDisplacementFromCameraInCm + xCameraToScreenDisplacementInCm;
    yScreenInCm = -yDisplacementFromCameraInCm - yCameraToScreenDisplacementInCm;
    xScreenInPoints = xScreenInCm / widthScreenInCm * widthScreenInPoints;
    yScreenInPoints = yScreenInCm / heightScreenInCm * heightScreenInPoints;

    return cv::Point(xScreenInPoints, yScreenInPoints);
}

cv::Point screen2cam(float xScreenInPoints,
    float yScreenInPoints,
    float orientation,
    float widthScreenInPoints,
    float heightScreenInPoints)
{

    float xCameraToScreenDisplacementInCm = deviceMetrics["xCameraToScreenDisplacementInCm"];
    float yCameraToScreenDisplacementInCm = deviceMetrics["yCameraToScreenDisplacementInCm"];
    float widthScreenInCm = deviceMetrics["widthScreenInCm"];
    float heightScreenInCm = deviceMetrics["heightScreenInCm"];

    if (!xCameraToScreenDisplacementInCm ||
        !yCameraToScreenDisplacementInCm ||
        !widthScreenInCm ||
        !heightScreenInCm) {
        return cv::Point(0, 0);
    }

    float xScreenInCm;
    float yScreenInCm;
    float xDisplacementFromCameraInCm;
    float yDisplacementFromCameraInCm;

    // Camera Offset and Screen Orientation compensation
    // Camera above screen
    // - Portrait on iOS devices
    // - Landscape on Surface devices
    xScreenInCm = xScreenInPoints * widthScreenInCm / widthScreenInPoints;
    yScreenInCm = yScreenInPoints * heightScreenInCm / heightScreenInPoints;
    xDisplacementFromCameraInCm = xScreenInCm - xCameraToScreenDisplacementInCm;
    yDisplacementFromCameraInCm = -yScreenInCm - yCameraToScreenDisplacementInCm;
    
    return cv::Point(xDisplacementFromCameraInCm, yDisplacementFromCameraInCm);
}


