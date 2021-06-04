#pragma once
#include "framework.h";
#include "LinearRBF.h"
#include "Calibrator.h"

class LinearRBFCalibrator : public Calibrator {

private:
	float _screenFactor = 0.01F;
	GazeInference_WinCpp::LinearRBF _linearRBF;

public:
	LinearRBFCalibrator(cv::Rect rect) :
		_linearRBF(20, _screenFactor)
	{
		this->rect = rect;

		//Init
		_linearRBF.Clear();
		_linearRBF.InitializeCalibrationTransform(0, 0, this->rect.width, this->rect.height, _screenFactor, true, 0);
	}

	~LinearRBFCalibrator(){}

	void add(std::vector<cv::Point2f> actualPts, std::vector<cv::Point2f> predictedPts, bool remap = true) override final {

	}

	void add(cv::Point2f actualPt, cv::Point2f predictedPt, bool remap = true) override final {
		// Add to the lists
		this->actual_coordinates.push_back(actualPt);
		this->predicted_coordinates.push_back(predictedPt);
		// Add to the calibration grid
		double quantizeInputX = predictedPt.x;
		double quantizeInputY = predictedPt.y;
		double quantizeOutputX = actualPt.x;
		double quantizeOutputY = actualPt.y;
		_linearRBF.AddTranslation(predictedPt.x, predictedPt.y, actualPt.x, actualPt.y, 1, &quantizeInputX, &quantizeInputY, &quantizeOutputX, &quantizeOutputY);
	}

	cv::Point2f evaluate (cv::Point2f predictedPt) override final {
		//Evaluate
		double xTranlated, yTranlated;
		_linearRBF.Evaluate(predictedPt.x, predictedPt.y, xTranlated, yTranlated);
		return cv::Point2f(xTranlated, yTranlated);
		
	}

	void drawDistortionMap() override final {
	}

	void reset() override final {
		this->actual_coordinates = std::vector<cv::Point2f>();
		this->predicted_coordinates = std::vector<cv::Point2f>();
		_linearRBF.Clear();
	}

    bool serialize(const wchar_t* path) override final {
		return _linearRBF.Serialize(path);
    }

    bool deserialize(const wchar_t* path) override final {

		bool status = _linearRBF.Deserialize(path);

		////////////////
		GazeInference_WinCpp::LinearRBFData* pLinearRBFData = _linearRBF.Serialize();
		BYTE* pData = (BYTE*)pLinearRBFData;
		BYTE* pCalPointData = pData + sizeof(GazeInference_WinCpp::LinearRBFData);

		while (pCalPointData < pData + pLinearRBFData->Size)
		{
			GazeInference_WinCpp::CalibrationPointData* pCalData = (GazeInference_WinCpp::CalibrationPointData*)pCalPointData;
			GazeInference_WinCpp::CalibrationHistoryData* pHistoryDataItems = (GazeInference_WinCpp::CalibrationHistoryData*)((BYTE*)pCalData + sizeof(GazeInference_WinCpp::CalibrationPointData));
			pCalPointData = pCalPointData + pCalData->Size;
		}
		delete pLinearRBFData;
		////////////////
		
		return status;
    }
};