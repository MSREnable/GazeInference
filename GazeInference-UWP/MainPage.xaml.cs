using GazeInference_Library;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Graphics.Imaging;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace GazeInference_UWP
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        private void ButtonToClick_Click(object sender, RoutedEventArgs e)
        {
            FileInfo model = new FileInfo("Assets\\Model\\itracker.onnx");
            FileInfo inputFrame = new FileInfo("Assets\\Frames\\00000.jpg");
            FileInfo datasetPath = null;

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
            //SoftwareBitmap face_bitmap = null;
            //SoftwareBitmap left_eye_bitmap = null;
            //SoftwareBitmap right_eye_bitmap = null;
            //float[] face_grid = null;
            //var rgb_bitmap = (SoftwareBitmap)Image.FromFile(imagePath);

            //var isValid = ITrackerFaceExtracter.ExtractFaceDataFromImage(rgb_bitmap, ref face_bitmap, ref left_eye_bitmap, ref right_eye_bitmap, ref face_grid);

            //if (isValid)
            //{
            //    var prediction = ITrackerPredictionEngine.RunPredictionOnImage(face_bitmap, left_eye_bitmap, right_eye_bitmap, face_grid);

            //    return prediction;
            //}

            return null;
        }
    }
}
