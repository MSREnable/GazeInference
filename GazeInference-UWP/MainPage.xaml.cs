using DlibDotNet;
using GazeInference_Library;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Graphics.Imaging;
using Windows.Storage.Streams;
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

        private static byte[] Array2DtoByteArray(Array2D<RgbPixel> array2d_image)
        {
            var height = array2d_image.Rows;
            var width = array2d_image.Columns;

            byte[] byte_array = new byte[height * width * IMAGE_BIT_DEPTH];

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    var pixel = array2d_image[y][x];

                    var index = y * width * IMAGE_BIT_DEPTH + x * IMAGE_BIT_DEPTH;

                    var index_R = index;
                    var index_G = index + 1;
                    var index_B = index + 2;

                    byte_array[index_R] = pixel.Red;
                    byte_array[index_G] = pixel.Green;
                    byte_array[index_B] = pixel.Blue;
                }
            }

            return byte_array;
        }

        private static Array2D<RgbPixel> SoftwareBitmapToArray2D(
            SoftwareBitmap input_bitmap)
        {
            var input_bitmap_array = SoftwareBitmapToByteArray(input_bitmap).Result;

            return Dlib.LoadImageData<RgbPixel>(ImagePixelFormat.Rgb, input_bitmap_array, (uint)input_bitmap.PixelHeight, (uint)input_bitmap.PixelWidth, IMAGE_BIT_DEPTH);
        }

        private static async Task<byte[]> SoftwareBitmapToByteArray(SoftwareBitmap input_bitmap)
        {
            byte[] bitmap_data = null;

            using (var memory_stream = new InMemoryRandomAccessStream())
            {
                BitmapEncoder encoder = await BitmapEncoder.CreateAsync(BitmapEncoder.BmpEncoderId, memory_stream);
                encoder.SetSoftwareBitmap(input_bitmap);

                try
                {
                    await encoder.FlushAsync();
                }
                catch
                {
                    return new byte[0];
                }

                bitmap_data = new byte[memory_stream.Size];
                await memory_stream.ReadAsync(bitmap_data.AsBuffer(), (uint)memory_stream.Size, InputStreamOptions.None);
            }
            return bitmap_data;
        }
    }
}
