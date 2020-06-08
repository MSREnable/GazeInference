using GazeInference_Library;
using System;
using System.Drawing;
using System.IO;

namespace GazeInference_Console
{
    class Program
    {
        static void Main(
            FileInfo inputFrame, 
            FileInfo model, 
            FileInfo datasetPath)
        {
            ITrackerPredictionEngine.InitializePredictionEngine(model?.FullName);

            if (!string.IsNullOrEmpty(inputFrame?.FullName))
            {
                var prediction = RunPredictionOnImage(inputFrame?.FullName);

                Console.WriteLine($"{inputFrame?.FullName} ({prediction.Item1}, {prediction.Item2})");
            }
            else
            {
                var dataset_base_path = datasetPath?.FullName;

                var recording_sessions = Directory.GetDirectories(dataset_base_path);

                foreach (var session in recording_sessions)
                {
                    var session_path = Path.Combine(dataset_base_path, session);
                    var session_frames_path = Path.Combine(session_path, "frames");

                    var frames = Directory.GetFiles(session_frames_path, "*.jpg");

                    foreach (var frame in frames)
                    {
                        var frame_path = Path.Combine(session_path, frame);

                        var prediction = RunPredictionOnImage(frame_path);
                        Console.WriteLine($"{frame} ({prediction.Item1}, {prediction.Item2})");
                    }
                }
            }
        }

        public static Tuple<float, float> RunPredictionOnImage(
            string imagePath)
        {
            Bitmap face_bitmap = null;
            Bitmap left_eye_bitmap = null;
            Bitmap right_eye_bitmap = null;
            float[] face_grid = null;
            var rgb_bitmap = (Bitmap)Image.FromFile(imagePath);

            var isValid = ITrackerFaceExtracter.ExtractFaceDataFromImage(rgb_bitmap, ref face_bitmap, ref left_eye_bitmap, ref right_eye_bitmap, ref face_grid);

            if (isValid)
            {
                var prediction = ITrackerPredictionEngine.RunPredictionOnImage(face_bitmap, left_eye_bitmap, right_eye_bitmap, face_grid);

                return prediction;
            }

            return null;
        }
    }
}
