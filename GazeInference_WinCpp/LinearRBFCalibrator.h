#pragma once
#include "framework.h";
#include "LinearRBF.h"

class LinearRBFCalibrator : public Calibrator {

private:
	float _screenFactor = 0.075F;
	GazeInference_WinCpp::LinearRBF _linearRBF;

public:
	LinearRBFCalibrator(cv::Rect rect) :
		_linearRBF(12, _screenFactor)
	{
		this->rect = rect;

		//Init
		_linearRBF.Clear();
		_linearRBF.InitializeCalibrationTransform(0, 0, this->rect.width, this->rect.height, _screenFactor, true, 0);
	}

	void add(std::vector<cv::Point2f> actualPts, std::vector<cv::Point2f> predictedPts) {

	}

	void add(cv::Point2f actualPt, cv::Point2f predictedPt) {
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

	cv::Point2f evaluate(cv::Point2f predictedPt) {
		//Evaluate
		double xTranlated, yTranlated;
		_linearRBF.Evaluate(predictedPt.x, predictedPt.y, xTranlated, yTranlated);
		return cv::Point2f(xTranlated, yTranlated);
	}

	void drawDistortionMap() {
	}

	void save() {
		 const wchar_t* calibrationModelPath = L"assets/calibrationModel.txt";
		_linearRBF.Serialize(calibrationModelPath);
	}

	void load() {
		const wchar_t* calibrationModelPath = L"assets/calibrationModel.txt";
		_linearRBF.Deserialize(calibrationModelPath);

		GazeInference_WinCpp::LinearRBFData* pLinearRBFData = _linearRBF.Serialize();
		BYTE* pData = (BYTE*)pLinearRBFData;
		BYTE* pCalPointData = pData + sizeof(GazeInference_WinCpp::LinearRBFData);

		while (pCalPointData < pData + pLinearRBFData->Size)
		{
			GazeInference_WinCpp::CalibrationPointData* pCalData = (GazeInference_WinCpp::CalibrationPointData*)pCalPointData;
			GazeInference_WinCpp::CalibrationHistoryData* pHistoryDataItems = (GazeInference_WinCpp::CalibrationHistoryData*)((BYTE*)pCalData + sizeof(GazeInference_WinCpp::CalibrationPointData));
			pCalPointData = pCalPointData + pCalData->Size;
		}

		////////////////

		delete pLinearRBFData;

		//Run_Click(nullptr, nullptr);
	}

	void reset(){}
};