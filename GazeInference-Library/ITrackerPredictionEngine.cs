using Microsoft.ML;
using Microsoft.ML.Transforms.Image;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;

namespace GazeInference_Library
{
    public class ITrackerPredictionEngine
    {
        private static PredictionEngine<ITrackerInputData, ITrackerPrediction> mlNetPrediction = null;

        public static void InitializePredictionEngine(string modelFilePath)
        {
            if (string.IsNullOrEmpty(modelFilePath))
            {
                string assetsPath = "assets";
                modelFilePath = Path.Combine(assetsPath, "Model", "itracker.onnx");
            }

            var mlContext = new MLContext();

            var dataView = mlContext.Data.LoadFromEnumerable(new List<ITrackerInputData>());

            // TODO figure out how to quiet debug spew from model load
            var model = mlContext.Transforms.ApplyOnnxModel(
                modelFile: modelFilePath,
                inputColumnNames: new[] { "face", "eyesLeft", "eyesRight", "faceGrid" },
                outputColumnNames: new[] { "data" },
                fallbackToCpu: true);

            var pipeline = mlContext.Transforms.ResizeImages(resizing: ImageResizingEstimator.ResizingKind.Fill,
                                                             outputColumnName: nameof(ITrackerInputData.face),
                                                             imageWidth: ITrackerImageSettings.imageWidth,
                                                             imageHeight: ITrackerImageSettings.imageHeight,
                                                             inputColumnName: nameof(ITrackerInputData.face))
                            .Append(mlContext.Transforms.ResizeImages(resizing: ImageResizingEstimator.ResizingKind.Fill,
                                                             outputColumnName: nameof(ITrackerInputData.eyesLeft),
                                                             imageWidth: ITrackerImageSettings.imageWidth,
                                                             imageHeight: ITrackerImageSettings.imageHeight,
                                                             inputColumnName: nameof(ITrackerInputData.eyesLeft)))
                            .Append(mlContext.Transforms.ResizeImages(resizing: ImageResizingEstimator.ResizingKind.Fill,
                                                             outputColumnName: nameof(ITrackerInputData.eyesRight),
                                                             imageWidth: ITrackerImageSettings.imageWidth,
                                                             imageHeight: ITrackerImageSettings.imageHeight,
                                                             inputColumnName: nameof(ITrackerInputData.eyesRight)))
                            .Append(mlContext.Transforms.ExtractPixels(inputColumnName: nameof(ITrackerInputData.face),
                                                                       outputColumnName: nameof(ITrackerInputData.face)))
                            .Append(mlContext.Transforms.ExtractPixels(inputColumnName: nameof(ITrackerInputData.eyesLeft),
                                                                       outputColumnName: nameof(ITrackerInputData.eyesLeft)))
                            .Append(mlContext.Transforms.ExtractPixels(inputColumnName: nameof(ITrackerInputData.eyesRight),
                                                                       outputColumnName: nameof(ITrackerInputData.eyesRight)))
                            .Append(model);

            var mlNetModel = pipeline.Fit(dataView);

            mlNetPrediction = mlContext.Model.CreatePredictionEngine<ITrackerInputData, ITrackerPrediction>(mlNetModel);
        }

        public static Tuple<float, float> RunPredictionOnImage(
            Bitmap face_bitmap,
            Bitmap left_eye_bitmap,
            Bitmap right_eye_bitmap,
            float[] face_grid)
        {
            Tuple<float, float> result = null;

            var inputData = new ITrackerInputData
            {
                face = face_bitmap,
                eyesLeft = left_eye_bitmap,
                eyesRight = right_eye_bitmap,
                faceGrid = face_grid
            };
            ITrackerPrediction prediction = mlNetPrediction.Predict(inputData);
            if (prediction != null)
            {
                float x = prediction.Data[0];
                float y = prediction.Data[1];

                result = new Tuple<float, float>(x, y);
            }

            return result;
        }       
    }
}
