#pragma once
#include "framework.h"
#include "GenMatrix.h"
#include <ppltasks.h>


namespace GazeInference_WinCpp
{
	class CalibrationHistory
	{
	public:
		CalibrationHistory(double inputX, double inputY, float confidence)
		{
			InputX = inputX;
			InputY = inputY;
			Confidence = std::max(confidence, 0.0f);
			AgedConfidence = Confidence; //Not serialized. Calculated when needed
			ApplyTime = time(NULL);
		}

		CalibrationHistory(double inputX, double inputY, float confidence, time_t applyTime)
		{
			InputX = inputX;
			InputY = inputY;
			Confidence = std::max(confidence, 0.0f);
			AgedConfidence = Confidence;
			ApplyTime = applyTime;
		}

		double InputX;
		double InputY;
		time_t ApplyTime;
		float Confidence;
		float AgedConfidence;

		//Only used by angular cluster weighting 
		//TODO: Remove these when finished testing the MedianWeighting model- it seems to be the best
		int ClusterGroupId;
		bool IsClusterGroup;
		//
	};

	class CalibrationPoint
	{
	public:
		CalibrationPoint(double inputX, double inputY, double outputX, double outputY, double anchorX, double anchorY, float confidence, bool addHistory)
			: Input(2, 1), Output(2, 1)

		{
			Input(0) = inputX;
			Input(1) = inputY;

			Output(0) = outputX;
			Output(1) = outputY;

			AvgInputX = inputX;
			AvgInputY = inputY;

			AnchorX = anchorX;
			AnchorY = anchorY;

			ConfidenceSum = confidence;

			if (addHistory)
				History.push_back(CalibrationHistory(inputX, inputY, confidence));
		}

	public:
		GenMatrix Input;
		GenMatrix Output;

		double AvgInputX;
		double AvgInputY;

		double AnchorX;
		double AnchorY;

		float ConfidenceSum;

		std::vector<CalibrationHistory> History;
	};


	//
	//Serialization support
	//


#pragma pack(8) //Check this is correct for all platforms

	struct CalibrationHistoryData
	{
		double InputX;
		double InputY;
		time_t ApplyTime;
		float Confidence;
	};

	struct CalibrationPointData
	{
		int Size;
		double InputX;
		double InputY;

		double OutputX;
		double OutputY;

		double AnchorX;
		double AnchorY;

		double AvgInputX;
		double AvgInputY;

		float ConfidenceSum;
		int HistoryCount;
	};

	struct LinearRBFData
	{
		int Version;
		int Size;
		int MaxHistory;
		int AnchorTranslationCount;

		float ScreenQuantizationFactor;
		float ScreenQuantizationLength;

		double ScreenLeft;
		double ScreenTop;
		double ScreenWidth;
		double ScreenHeight;

		int CalibrationPointCount;
	};

}



