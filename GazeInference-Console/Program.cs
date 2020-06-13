using DlibDotNet;
using DlibDotNet.Dnn;
using DlibDotNet.Extensions;
using GazeInference_Library;
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

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
            Array2D<RgbPixel> rgb_array2d_img = LoadArray2DfromFile(imagePath);

            Array2D<RgbPixel> face_array2d_image = null;
            Array2D<RgbPixel> left_eye_array2d_image = null;
            Array2D<RgbPixel> right_eye_array2d_image = null;
            float[] face_grid = null;

            var isValid = ITrackerFaceExtracter.ExtractFaceDataFromImage(rgb_array2d_img, ref face_array2d_image, ref left_eye_array2d_image, ref right_eye_array2d_image, ref face_grid);

            var face_bitmap_array = Array2DtoByteArray(face_array2d_image);
            var left_eye_bitmap_array = Array2DtoByteArray(face_array2d_image);
            var right_eye_bitmap_array = Array2DtoByteArray(face_array2d_image);

            if (isValid)
            {
                var prediction = ITrackerPredictionEngine.RunPredictionOnImage(face_bitmap_array, left_eye_bitmap_array, right_eye_bitmap_array, face_grid);

                return prediction;
            }

            return null;
        }

        const uint IMAGE_BIT_DEPTH = 3;

        private static Array2D<RgbPixel> LoadArray2DfromFile(string imagePath)
        {
            return Dlib.LoadImage<RgbPixel>(imagePath);
        }

        private static byte[] Array2DtoByteArray(Array2D<RgbPixel> face_array2d_image)
        {
            return face_array2d_image.ToBitmap<RgbPixel>().ToByteArray(ImageFormat.Bmp);
        }

        private static Array2D<RgbPixel> BitmapToArray2D(
            Bitmap input_bitmap)
        {
            return Dlib.LoadImageData<RgbPixel>(ImagePixelFormat.Rgb, input_bitmap.ToByteArray(ImageFormat.Bmp), (uint)input_bitmap.Height, (uint)input_bitmap.Width, IMAGE_BIT_DEPTH);
        }
    }
}
