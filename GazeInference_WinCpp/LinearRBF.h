#pragma once

#include <iostream>
#include <fstream>
#include "GenMatrix.h"
#include "LinearRBFTypes.h"



namespace GazeInference_WinCpp
{
	const int LINEARRBF_MAXHISTORY_DEFAULT = 20;

	class LinearRBF
	{
	public:

		LinearRBF(int maxHistory = LINEARRBF_MAXHISTORY_DEFAULT, float screenQuantizationFactor = 0.1f) : _c(0), _g(0)
		{
			_anchorTranslationCount = 0;

			_screenQuantizationFactor = screenQuantizationFactor;

			_maxHistory = std::max(maxHistory, 1);

			//This is set correctly based on screen size and ScreenQuantizationFactor in InitializeCalibrationTransform()
			//This default is used for cases where Screen size isn't set
			_screenQuantizationLength = 100;
		}

		~LinearRBF()
		{
			for (std::vector<CalibrationPoint*>::iterator it = _calibrationPoints.begin(); it != _calibrationPoints.end(); ++it)
				delete* it;
		}


		//
		//Methods
	public:

		//
		//This should be called before using this tranform componment or in the event of a the screen size/aspect change
		//
		void InitializeCalibrationTransform(double screenLeft, double screenTop, double screenWidth, double screenHeight, float screenQuantizationFactor, bool addAnchors, double anchorMargin)
		{
			Clear();
			_screenLeft = screenLeft;
			_screenTop = screenTop;
			_screenWidth = screenWidth;
			_screenHeight = screenHeight;
			/*_screenQuantizationFactor = max(min(float(.25), screenQuantizationFactor), float(.025));*/
			_screenQuantizationFactor = std::max(std::min(float(.25), screenQuantizationFactor), float(.025));

			_screenQuantizationLength = float(screenWidth > screenHeight ? screenWidth * _screenQuantizationFactor : screenHeight * _screenQuantizationFactor);

			if (!addAnchors)
				return;

			double x1 = _screenLeft + anchorMargin;
			double y1 = _screenTop + anchorMargin;
			double x2 = _screenLeft + _screenWidth - anchorMargin;
			double y2 = _screenTop + _screenHeight - anchorMargin;

			//Left Top
			AddAnchorTranslation(x1, y1, x1, y1);

			//Right Top
			AddAnchorTranslation(x2, y1, x2, y1);

			//Left Bottom
			AddAnchorTranslation(x1, y2, x1, y2);

			//Right AddAnchorTranslation
			AddAnchorTranslation(x2, y2, x2, y2);
		}

		float GetScreenQuantizationLength()
		{
			return _screenQuantizationLength;
		}

		//
		//Call this to get the transform 
		//
		void Evaluate(double x, double y, double& xOut, double& yOut)
		{
			//UWP apps run on the UI thread so no need to do locking
			//std::lock_guard<std::recursive_mutex> lock(_lock);

			if (_calibrationPoints.size() < 1)
			{
				xOut = x;
				yOut = y;
				return;
			}

			int calPointsCount = (int)_calibrationPoints.size();

			auto m = GenMatrix(2, 1);
			m(0) = x;
			m(1) = y;

			for (int i = 0; i < calPointsCount; i++)
				_g(i) = Distance(&m, &(_calibrationPoints[i])->Input);

			GenMatrix tranlated = _c * _g;
			xOut = tranlated(0) + x;
			yOut = tranlated(1) + y;
		}

		float AddTranslation(
			double inputX, double inputY,
			double outputX, double outputY,
			float confidence,
			double* pQuantizeInputX, double* pQuantizeInputY,
			double* pQuantizeOutputX, double* pQuantizeOutputY)
		{
			//Don't accept super low confidence's. There is no point setting these
			if (confidence <= .01)
				return 0;

			/*confidence = min(confidence, _maxHistory / 2 + 1);*/
			confidence = std::min(confidence, (float)(_maxHistory / 2 + 1));

			double screenQuantizationIntervalHalf = _screenQuantizationLength / 2;

			double outputXAdjusted = floor((outputX + screenQuantizationIntervalHalf) / _screenQuantizationLength) * _screenQuantizationLength;
			double outputYAdjusted = floor((outputY + screenQuantizationIntervalHalf) / _screenQuantizationLength) * _screenQuantizationLength;
			double outXDelta = outputX - outputXAdjusted;
			double outYDelta = outputY - outputYAdjusted;

			////Bias the input interpolation toward the existing transform 
			//const double QuantizationBias = 1;
			//double factorFromQp = Distance(outputX, outputY, outputXAdjusted, outputYAdjusted) / _screenQuantizationLength * QuantizationBias;
			//double xEval, yEval;
			//EvaluateTranslate(inputX, inputY, xEval, yEval, (int)_calibrationPoints.size());
			//inputX = inputX - (outputX - xEval - inputX) * factorFromQp;
			//inputY = inputY - (outputY - yEval - inputY) * factorFromQp;
			////


			//Lower the confidence as the output point moves away from the quantized output. 
			const float QuantizationConfidenceBias = .5; //0 - 1 where 0 lowers confidence the most based on distance to the quantized output to 1 confidence is unchanged 
			float quantWeight = (1 - float(Distance(outputX, outputY, outputXAdjusted, outputYAdjusted)) / _screenQuantizationLength);
			quantWeight = (1 - quantWeight) * QuantizationConfidenceBias + quantWeight;
			confidence = confidence * quantWeight;


			inputX = inputX - outXDelta;
			inputY = inputY - outYDelta;

			if (pQuantizeInputX != NULL)
				*pQuantizeInputX = inputX;
			if (pQuantizeInputY != NULL)
				*pQuantizeInputY = inputY;
			if (pQuantizeOutputX != NULL)
				*pQuantizeOutputX = outputXAdjusted;
			if (pQuantizeOutputY != NULL)
				*pQuantizeOutputY = outputYAdjusted;

			//std::lock_guard<std::recursive_mutex> lock(_lock);

			CalibrationPoint* pCalPt = FindCalibrationPoint(outputXAdjusted, outputYAdjusted);
			if (pCalPt == NULL)
			{
				pCalPt = new CalibrationPoint(inputX, inputY, outputXAdjusted - inputX, outputYAdjusted - inputY, outputXAdjusted, outputYAdjusted, confidence, true);
				_calibrationPoints.push_back(pCalPt);
			}
			else
			{
				//Add and recalculate this calibration point
				pCalPt->History.push_back(CalibrationHistory(inputX, inputY, confidence));
			}

			float appliedConfidence = MedianWeightedTranslation(pCalPt);
			return appliedConfidence; //confidence
		}

		//Adds a special point used to anchor the edges of the transform. 
		//Anchor points must be added before weighted points. For internal use only
		void AddAnchorTranslation(double inputX, double inputY, double outputX, double outputY)
		{
			const float confidence = 1.0f;
			double screenQuantizationIntervalHalf = _screenQuantizationLength / 2;

			double outputXAdjusted = floor((outputX + screenQuantizationIntervalHalf) / _screenQuantizationLength) * _screenQuantizationLength;
			double outputYAdjusted = floor((outputY + screenQuantizationIntervalHalf) / _screenQuantizationLength) * _screenQuantizationLength;
			double outXDelta = outputX - outputXAdjusted;
			double outYDelta = outputY - outputYAdjusted;

			inputX = inputX - outXDelta;
			inputY = inputY - outYDelta;

			CalibrationPoint* pCalPt = new CalibrationPoint(inputX, inputY, outputXAdjusted - inputX, outputYAdjusted - inputY, outputXAdjusted, outputYAdjusted, confidence, true);
			_calibrationPoints.push_back(pCalPt);

			_anchorTranslationCount++;
			Prepare((int)_calibrationPoints.size());
		}

		//
		//Removes any calibration points withing the specified hitRadius
		//
		int RemoveTranslation(double inputX, double inputY, double hitRadius)
		{
			int removed = 0;

			//std::lock_guard<std::recursive_mutex> lock(_lock);

			int cInputs = (int)_calibrationPoints.size();

			for (int i = cInputs - 1; i >= 0; i--)
			{
				double distance = Distance(_calibrationPoints[i]->AvgInputX, _calibrationPoints[i]->AvgInputY, inputX, inputY);

				if (distance <= hitRadius)
				{
					delete _calibrationPoints[i];
					_calibrationPoints.erase(_calibrationPoints.begin() + i);

					cInputs--;
					removed++;
				}
			}

			Prepare((int)_calibrationPoints.size());
			return removed;
		}

		//
		//Gets the output point and the Median of the input points that are transformed to this output point for the specified index starting at 0. 
		//Returns false if the specified index doesn't exist. The caller can loop from 0 to n until this function return false to browse all points
		//
		bool GetOutputPoint(int index, double& quantizeOutputX, double& quantizeOutputY, double& medianInputX, double& medianInputY)
		{
			if (index < 0 || index >= (int)_calibrationPoints.size())
				return false;

			auto pCalPt = _calibrationPoints[index];
			quantizeOutputX = pCalPt->AnchorX;
			quantizeOutputY = pCalPt->AnchorY;
			medianInputX = pCalPt->AvgInputX;
			medianInputY = pCalPt->AvgInputY;

			return true;
		}

		//
		//Gets the input point for the specified output index and input index starting at 0. 
		//Returns false if the specified index doesn't exist. The caller can loop from 0, 0 to n until this function return false to browse all points
		//
		bool GetInputPoint(int outputIndex, int inputIndex, double& inputX, double& inputY, float& confidence)
		{
			if (outputIndex < 0 || outputIndex >= (int)_calibrationPoints.size() || inputIndex < 0 || inputIndex >= (int)_calibrationPoints[outputIndex]->History.size())
				return false;

			CalibrationHistory& history = _calibrationPoints[outputIndex]->History[inputIndex];
			inputX = history.InputX;
			inputY = history.InputY;
			confidence = history.Confidence;

			return true;
		}

		//
		//Clears all calibration points
		//
		void Clear()
		{
			//std::lock_guard<std::recursive_mutex> lock(_lock);

			for (std::vector<CalibrationPoint*>::iterator it = _calibrationPoints.begin(); it != _calibrationPoints.end(); ++it)
				delete* it;

			_calibrationPoints.clear();
			_anchorTranslationCount = 0;
		}

		//
		//Serializes all points and cofiguration to the specified file path
		//
		bool Serialize(const wchar_t* filePath)
		{
			std::ofstream writer;
			writer.open(filePath, std::ios::out | std::ios::trunc | std::ios::binary);

			if (!writer.is_open())
				return false;

			LinearRBFData* dataBuf = Serialize();

			writer.write((char*)dataBuf, dataBuf->Size);
			writer.close();

			delete dataBuf;

			return true;
		}

		//
		//Serializes all points and cofiguration to a memory buffer
		//This buffer needs to be deleted when caller is done with it
		//
		LinearRBFData* Serialize()
		{
			//std::lock_guard<std::recursive_mutex> lock(_lock);

			int size = sizeof(LinearRBFData);

			for (std::vector<CalibrationPoint*>::iterator it = _calibrationPoints.begin(); it != _calibrationPoints.end(); ++it)
			{
				size += sizeof(CalibrationPointData) + int((*it)->History.size() * sizeof(CalibrationHistoryData));
			}

			char* pData = new char[size];
			LinearRBFData* pLinearRBFData = (LinearRBFData*)pData;

			pLinearRBFData->Version = LinearRBFDataVersion;
			pLinearRBFData->Size = size;
			pLinearRBFData->MaxHistory = _maxHistory;
			pLinearRBFData->AnchorTranslationCount = _anchorTranslationCount;
			pLinearRBFData->ScreenQuantizationFactor = _screenQuantizationFactor;
			pLinearRBFData->ScreenQuantizationLength = _screenQuantizationLength;
			pLinearRBFData->ScreenLeft = _screenLeft;
			pLinearRBFData->ScreenTop = _screenTop;
			pLinearRBFData->ScreenWidth = _screenWidth;
			pLinearRBFData->ScreenHeight = _screenHeight;
			pLinearRBFData->CalibrationPointCount = (int)_calibrationPoints.size();

			CalibrationPointData* pCalPointData = (CalibrationPointData*)(pData + sizeof(LinearRBFData));

			for (std::vector<CalibrationPoint*>::iterator it = _calibrationPoints.begin(); it != _calibrationPoints.end(); ++it)
			{
				pCalPointData->Size = sizeof(CalibrationPointData) + int((*it)->History.size() * sizeof(CalibrationHistoryData));
				pCalPointData->InputX = (*it)->Input(0);
				pCalPointData->InputY = (*it)->Input(1);
				pCalPointData->OutputX = (*it)->Output(0);
				pCalPointData->OutputY = (*it)->Output(1);
				pCalPointData->AnchorX = (*it)->AnchorX;
				pCalPointData->AnchorY = (*it)->AnchorY;
				pCalPointData->AvgInputX = (*it)->AvgInputX;
				pCalPointData->AvgInputY = (*it)->AvgInputY;
				pCalPointData->ConfidenceSum = (*it)->ConfidenceSum;

				pCalPointData->HistoryCount = (int)(*it)->History.size();
				CalibrationHistoryData* pHistData = (CalibrationHistoryData*)((char*)pCalPointData + sizeof(CalibrationPointData));

				for (std::vector<CalibrationHistory>::iterator itHist = (*it)->History.begin(); itHist != (*it)->History.end(); ++itHist)
				{
					pHistData->InputX = itHist->InputX;
					pHistData->InputY = itHist->InputY;
					pHistData->ApplyTime = itHist->ApplyTime;
					pHistData->Confidence = itHist->Confidence;

					pHistData = (CalibrationHistoryData*)((char*)pHistData + sizeof(CalibrationHistoryData));
				}

				pCalPointData = (CalibrationPointData*)((char*)pCalPointData + pCalPointData->Size);
			}

			return pLinearRBFData;
		}

		//
		//Restores the serialized state from the specified path 
		//
		bool Deserialize(const wchar_t* filePath)
		{
			std::ifstream reader;
			reader.open(filePath, std::ios::in | std::ios::binary | std::ios::ate);

			if (!reader.is_open())
				return false;

			std::streampos size = reader.tellg();
			char* memBuf = new char[(unsigned int)size];
			reader.seekg(0, std::ios::beg);
			reader.read(memBuf, size);
			reader.close();

			bool result = Deserialize((char*)memBuf);
			delete memBuf;

			return result;
		}

		//
		//Restores the serialized state from the memory buffer
		//
		bool Deserialize(char* pData)
		{
			LinearRBFData* pLinearRBFData = (LinearRBFData*)pData;
			if (pLinearRBFData->Version != LinearRBFDataVersion)
				return false;

			//std::lock_guard<std::recursive_mutex> lock(_lock);
			Clear();

			_maxHistory = pLinearRBFData->MaxHistory;
			_anchorTranslationCount = pLinearRBFData->AnchorTranslationCount;
			_screenQuantizationFactor = pLinearRBFData->ScreenQuantizationFactor;
			_screenQuantizationLength = pLinearRBFData->ScreenQuantizationLength;
			_screenLeft = pLinearRBFData->ScreenLeft;
			_screenTop = pLinearRBFData->ScreenTop;
			_screenWidth = pLinearRBFData->ScreenWidth;
			_screenHeight = pLinearRBFData->ScreenHeight;

			char* pCalPointData = pData + sizeof(LinearRBFData);

			while (pCalPointData < pData + pLinearRBFData->Size)
			{
				CalibrationPointData* pCalData = (CalibrationPointData*)pCalPointData;
				CalibrationHistoryData* pHistoryDataItems = (CalibrationHistoryData*)((char*)pCalData + sizeof(CalibrationPointData));

				if (pCalData->HistoryCount > 0)
				{
					CalibrationPoint* pCalPt = new CalibrationPoint(
						pCalData->InputX, pCalData->InputY,
						pCalData->OutputX, pCalData->OutputY,
						pCalData->AnchorX, pCalData->AnchorY,
						pCalData->ConfidenceSum, false);

					for (int h = 0; h < pCalData->HistoryCount; h++)
					{
						pCalPt->History.push_back(CalibrationHistory(
							pHistoryDataItems[h].InputX, pHistoryDataItems[h].InputY, pHistoryDataItems[h].Confidence, pHistoryDataItems[h].ApplyTime));
					}

					_calibrationPoints.push_back(pCalPt);
				}

				pCalPointData = pCalPointData + pCalData->Size;
			}

			Prepare((int)_calibrationPoints.size());
			return true;
		}

	private:
		//
		// Creates a new linear RBF from (Input, output) pairs.
		// Is called anytime the input/output pairs are changed 
		//
		void Prepare(int pointsToPrepare)
		{
			//Must be called in the context of this lock: 	std::lock_guard<std::recursive_mutex> lock(_lock);

			int calPointsCount = std::min((int)_calibrationPoints.size(), pointsToPrepare);

			if (calPointsCount < 1)
				return;

			int dimY = (_calibrationPoints[0])->Output.GetRowCount();

			auto yMatrix = GenMatrix(dimY, calPointsCount);
			for (int i = 0; i < calPointsCount; i++)
				Copy(&yMatrix, 0, i, &(_calibrationPoints[i])->Output, 0, 0, dimY, 1);

			auto g = GenMatrix(calPointsCount, calPointsCount);
			for (int i = 0; i < calPointsCount; i++)
				for (int j = 0; j < calPointsCount; j++)
					g(i, j) = Distance(&(_calibrationPoints[i])->Input, &(_calibrationPoints[j])->Input);

			g.Invert();


			_c = yMatrix * g;
			_g.SetDimensions(calPointsCount, 1);
		}

		float MedianWeightedTranslation(CalibrationPoint* pCalibrationPoint)
		{
			if (pCalibrationPoint->History.size() < 1)
				return 0;

			//Remove excess history items and age out confidence values.
			//Reduce the confidence between 0 and .5 based on the positon in the history list relative to LINEARRBF_MAXHISTORY_DEFAULT
			//1 for no confidence aging. Note: ConfidenceAgingFactor can't be 0
			const float ConfidenceAgingMin = 0.333f;
			//The screen height or width factor that is used to control confidence of a calibration point that is within distance of a higher confidence point.
			const float ConfidenceDistanceFactor = 0.5f;

			//Sort on time the time the item was added to the history
			std::sort(pCalibrationPoint->History.begin(), pCalibrationPoint->History.end(),
				[](CalibrationHistory const& a, CalibrationHistory const& b) { return a.ApplyTime > b.ApplyTime; });

			if ((int)pCalibrationPoint->History.size() > _maxHistory)
			{
				pCalibrationPoint->History.erase(pCalibrationPoint->History.begin() + _maxHistory, pCalibrationPoint->History.end());
			}

			float hcf = (1 - ConfidenceAgingMin) / std::max(_maxHistory - 1, 1);

			for (size_t i = 0; i < pCalibrationPoint->History.size(); i++)
			{
				CalibrationHistory& calHist = pCalibrationPoint->History[i];
				calHist.AgedConfidence = calHist.Confidence * (1 - hcf * i);
			}

			//Find the median based on a sort on X and then the center of the sum of point weights
			//
			std::sort(pCalibrationPoint->History.begin(), pCalibrationPoint->History.end(),
				[](CalibrationHistory const& a, CalibrationHistory const& b) { return a.InputX < b.InputX; });

			size_t windowBounds = std::max(pCalibrationPoint->History.size() / 4 + 1, (size_t)1); //The 1/4 in the middle with 2 points min

			double sum = 0;
			double sumWeightX = 0;
			int l = 0;
			int r = (int)pCalibrationPoint->History.size() - 1;
			float weightsSumL = pCalibrationPoint->History[l].AgedConfidence;
			float weightsSumR = pCalibrationPoint->History[r].AgedConfidence;;

			while (l < r)
			{
				if (weightsSumL < weightsSumR)
				{
					l++;
					float weight = pCalibrationPoint->History[l].AgedConfidence;
					weightsSumL += weight;
				}
				else
				{
					r--;
					float weight = pCalibrationPoint->History[r].AgedConfidence;
					weightsSumR += weight;
				}
			}

			bool leftSide = true;
			r = l + 1;

			for (size_t w = 0; w < windowBounds; w++)
			{
				if (leftSide)
				{
					if (l >= 0)
					{
						float weight = pCalibrationPoint->History[l].AgedConfidence;
						sum += pCalibrationPoint->History[l].InputX * weight;
						sumWeightX += weight;

						l--;
					}
					else
						break; //Don't keep averaging if any list limit is reached
				}
				else
				{
					if (r < (int)pCalibrationPoint->History.size())
					{
						float weight = pCalibrationPoint->History[r].AgedConfidence;
						sum += pCalibrationPoint->History[r].InputX * weight;
						sumWeightX += weight;

						r++;
					}
					else
						break; //Don't keep averaging if any list limit is reached
				}

				leftSide = !leftSide;
			}

			double medianX = sum / sumWeightX;
			//calculate the X weight based on point distance to the medianX 
			//
			double xExtent = std::max(pCalibrationPoint->History[pCalibrationPoint->History.size() - 1].InputX - pCalibrationPoint->History[0].InputX, (double)_screenQuantizationLength);
			if (xExtent > 0)
			{
				sumWeightX = 0;

				for (std::vector<CalibrationHistory>::iterator it = pCalibrationPoint->History.begin(); it != pCalibrationPoint->History.end(); ++it)
				{
					sumWeightX += (1 - abs((*it).InputX - medianX) / xExtent) * (*it).AgedConfidence;
				}
			}



			//Find the median based on a sort on Y and then the center of the sum of point weights
			//
			std::sort(pCalibrationPoint->History.begin(), pCalibrationPoint->History.end(),
				[](CalibrationHistory const& a, CalibrationHistory const& b) { return a.InputY < b.InputY; });

			sum = 0;
			double sumWeightY = 0;
			l = 0;
			r = (int)pCalibrationPoint->History.size() - 1;
			weightsSumL = pCalibrationPoint->History[l].AgedConfidence;
			weightsSumR = pCalibrationPoint->History[r].AgedConfidence;

			while (l < r)
			{
				if (weightsSumL < weightsSumR)
				{
					l++;
					float weight = pCalibrationPoint->History[l].AgedConfidence;
					weightsSumL += weight;
				}
				else
				{
					r--;
					float weight = pCalibrationPoint->History[r].AgedConfidence;
					weightsSumR += weight;
				}
			}

			leftSide = true;
			r = l + 1;

			for (size_t w = 0; w < windowBounds; w++)
			{
				if (leftSide)
				{
					if (l >= 0)
					{
						float weight = pCalibrationPoint->History[l].AgedConfidence;
						sum += pCalibrationPoint->History[l].InputY * weight;
						sumWeightY += weight;

						l--;
					}
					else
						break; //Don't keep averaging if any list limit is reached
				}
				else
				{
					if (r < (int)pCalibrationPoint->History.size())
					{
						float weight = pCalibrationPoint->History[r].AgedConfidence;
						sum += pCalibrationPoint->History[r].InputY * weight;
						sumWeightY += weight;

						r++;
					}
					else
						break; //Don't keep averaging if any list limit is reached
				}

				leftSide = !leftSide;
			}

			double medianY = sum / sumWeightY;

			//calculate the Y weight based on point distance to the medianY 
			//
			double yExtent = std::max(pCalibrationPoint->History[pCalibrationPoint->History.size() - 1].InputY - pCalibrationPoint->History[0].InputY, (double)_screenQuantizationLength);
			if (yExtent > 0)
			{
				sumWeightY = 0;

				for (std::vector<CalibrationHistory>::iterator it = pCalibrationPoint->History.begin(); it != pCalibrationPoint->History.end(); ++it)
				{
					sumWeightY += (1 - abs((*it).InputY - medianY) / yExtent) * (*it).AgedConfidence;
				}
			}


			pCalibrationPoint->AvgInputX = medianX;
			pCalibrationPoint->AvgInputY = medianY;

			//The stored ConfidenceSum is the max of the history confidences weighted on there distance to the median points (in x and y planes).
			pCalibrationPoint->ConfidenceSum = float(std::max(std::max(sumWeightX, sumWeightY), .001)); //Don't allow 0

			auto weightedPointBeginIt = _calibrationPoints.begin() + _anchorTranslationCount;
			//Sort CalibrationPoints from largest ConfidenceSum to smallest
			std::sort(weightedPointBeginIt, _calibrationPoints.end(),
				[](CalibrationPoint* a, CalibrationPoint* b) { return a->ConfidenceSum > b->ConfidenceSum; });

			float distMaxHorz = float(_screenWidth * ConfidenceDistanceFactor);
			float distMaxVert = float(_screenHeight * ConfidenceDistanceFactor);

			double xTranslate, yTranslate;
			float appliedConf = 1;

			//Initialize the transform with the first calibration point
			Prepare(_anchorTranslationCount);

			for (int i = _anchorTranslationCount; i < (int)_calibrationPoints.size(); i++)
			{
				CalibrationPoint* pOuter = _calibrationPoints[i];
				float distConfFactorMax = 0;

				for (int k = i - 1; k >= _anchorTranslationCount; k--)
				{
					CalibrationPoint* pInner = _calibrationPoints[k];

					float distFactor = float(std::max(abs(pOuter->AvgInputX - pInner->AvgInputX) / distMaxHorz, abs(pOuter->AvgInputY - pInner->AvgInputY) / distMaxVert));
					distConfFactorMax = std::max(distConfFactorMax, (1 - std::min(distFactor, 1.0f)) * pInner->ConfidenceSum);
				}

				float translationRatio = pOuter->ConfidenceSum / std::max(pOuter->ConfidenceSum + distConfFactorMax, 1.0f);
				EvaluateTranslate(pOuter->AnchorX, pOuter->AnchorY, xTranslate, yTranslate, i);

				double weightedTranslateX = (xTranslate * (1 - translationRatio)) + ((pOuter->AnchorX - pOuter->AvgInputX) * translationRatio);
				double weightedTranslateY = (yTranslate * (1 - translationRatio)) + ((pOuter->AnchorY - pOuter->AvgInputY) * translationRatio);

				pOuter->Input(0) = pOuter->AnchorX - weightedTranslateX;
				pOuter->Input(1) = pOuter->AnchorY - weightedTranslateY;

				pOuter->Output(0) = weightedTranslateX;
				pOuter->Output(1) = weightedTranslateY;

				Prepare(i + 1);

				if (pOuter == pCalibrationPoint)
					appliedConf = translationRatio;
			}

			return appliedConf;
		}

		CalibrationPoint* FindCalibrationPoint(double anchorX, double anchorY)
		{
			for (std::vector<CalibrationPoint*>::iterator it = _calibrationPoints.begin() + _anchorTranslationCount; it != _calibrationPoints.end(); ++it)
			{
				if (abs((*it)->AnchorX - anchorX) < 1 && abs((*it)->AnchorY - anchorY) < 1)
					return *it;
			}

			return NULL;
		}

		void EvaluateTranslate(double x, double y, double& xTranslate, double& yTranslate, int pointsToEval)
		{
			int calPointsCount = std::min((int)_calibrationPoints.size(), pointsToEval);

			if (calPointsCount < 1)
			{
				xTranslate = x;
				yTranslate = y;
				return;
			}

			auto m = GenMatrix(2, 1);
			m(0) = x;
			m(1) = y;

			for (int i = 0; i < calPointsCount; i++)
				_g(i) = Distance(&m, &(_calibrationPoints[i])->Input);

			GenMatrix tranlated = _c * _g;
			xTranslate = tranlated(0);
			yTranslate = tranlated(1);
		}

		static void Copy(GenMatrix* pTarget, int rowTarget, int colTarget, GenMatrix* pSource, int rowSource, int colSource, int rows, int cols)
		{
			for (int row = 0; row < rows; row++)
				for (int col = 0; col < cols; col++)
					(*pTarget)(rowTarget + row, colTarget + col) = (*pSource)(rowSource + row, colSource + col);
		}

		static double Distance(GenMatrix* pFrom, GenMatrix* pTo)
		{
			double s = 0.0;
			double d;
			for (int i = 0; i < pTo->GetElementCount(); i++)
			{
				d = (*pTo)(i) - (*pFrom)(i);
				s += d * d;
			}

			return sqrt(s);
		}

		static double Distance(double x1, double y1, double x2, double y2)
		{
			double xDelta = x1 - x2;
			double yDelta = y1 - y2;
			return sqrt(xDelta * xDelta + yDelta * yDelta);
		}

	private:

		const int LinearRBFDataVersion = 3;

		int _maxHistory;

		int _anchorTranslationCount;

		std::vector<CalibrationPoint*> _calibrationPoints;
		GenMatrix _c;
		GenMatrix _g;

		float _screenQuantizationFactor;
		float _screenQuantizationLength;

		//UWP apps run on the UI thread so no need to do locking
		//std::recursive_mutex _lock;

		double _screenLeft;
		double _screenTop;
		double _screenWidth;
		double _screenHeight;

	};

}