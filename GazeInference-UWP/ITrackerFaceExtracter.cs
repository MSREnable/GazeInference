using DlibDotNet;
using DlibDotNet.Extensions;
using System;
using System.Drawing;
using System.Runtime.InteropServices;
using Windows.UI.ViewManagement.Core;

namespace GazeInference_Library
{
    public class ITrackerFaceExtracter
    {
        // Left Eye = 36-41
        const int LEFT_EYE_START = 36;
        const int LEFT_EYE_END = 41;

        // Right Eye = 42-47
        const int RIGHT_EYE_START = 42;
        const int RIGHT_EYE_END = 47;

        private static readonly FrontalFaceDetector detector = Dlib.GetFrontalFaceDetector();
        private static readonly ShapePredictor sp = ShapePredictor.Deserialize("shape_predictor_68_face_landmarks.dat");

        private static readonly bool debugData = false;

        public static bool ExtractFaceDataFromImage(
            Bitmap rgb_bitmap,
            ref Bitmap face_bitmap,
            ref Bitmap left_eye_bitmap,
            ref Bitmap right_eye_bitmap,
            ref float[] face_grid)
        {
            var rgb_array2d_img = BitmapToRgbPixel(rgb_bitmap);
            var ycbcr_array2d_img = RgbToYCbCr(rgb_array2d_img);

            Dlib.SaveJpeg(rgb_array2d_img, "face_rgb.jpg");
            Dlib.SaveJpeg(ycbcr_array2d_img, "face_ycbcr.jpg");

            var face_rects = detector.Operator(rgb_array2d_img);

            if (face_rects.Length != 1)
                return false;

            foreach (var face_rect in face_rects)
            {
                var shape = sp.Detect(rgb_array2d_img, face_rect);

                var left_eye_rect = GetRect(shape, LEFT_EYE_START, LEFT_EYE_END);
                var right_eye_rect = GetRect(shape, RIGHT_EYE_START, RIGHT_EYE_END);

                var left_eye_rect_normalized = GetEyeRectSizeNormalized(face_rect, left_eye_rect);
                var right_eye_rect_normalized = GetEyeRectSizeNormalized(face_rect, right_eye_rect);

                DrawDebugDataOnImage(rgb_array2d_img, shape, face_rect, left_eye_rect, right_eye_rect);

                GenerateInputs(
                    ycbcr_array2d_img,
                    face_rect,
                    left_eye_rect_normalized,
                    right_eye_rect_normalized,
                    ref face_bitmap,
                    ref left_eye_bitmap,
                    ref right_eye_bitmap,
                    ref face_grid);
            }

            return true;
        }

        private static void DrawDebugDataOnImage(
            Array2D<RgbPixel> img,
            FullObjectDetection shape,
            DlibDotNet.Rectangle face_rect,
            DlibDotNet.Rectangle left_eye_rect,
            DlibDotNet.Rectangle right_eye_rect)
        {
            if (debugData)
            {
                // For debugging, show face points
                for (uint i = 0; i < shape.Parts; ++i)
                {
                    var point = shape.GetPart(i);
                    var rect = new DlibDotNet.Rectangle(point);
                    var color = new RgbPixel(255, 255, 0);

                    if (i >= 36 && i <= 41)
                        color = new RgbPixel(0, 0, 255);
                    else if (i >= 42 && i <= 47)
                        color = new RgbPixel(0, 255, 0);
                    if (i >= 27 && i <= 35)
                        color = new RgbPixel(255, 0, 0);

                    Dlib.DrawRectangle(img, rect, color: color, thickness: 4);
                }

                Dlib.DrawRectangle(img, face_rect, color: new RgbPixel(0, 255, 0), thickness: 4);
                Dlib.DrawRectangle(img, left_eye_rect, color: new RgbPixel(0, 255, 255), thickness: 4);
                Dlib.DrawRectangle(img, right_eye_rect, color: new RgbPixel(0, 255, 255), thickness: 4);
            }
        }

        private static void GenerateInputs(
            Array2D<RgbPixel> img,
            DlibDotNet.Rectangle face_rect,
            DlibDotNet.Rectangle left_eye_rect,
            DlibDotNet.Rectangle right_eye_rect,
            ref Bitmap face_bitmap,
            ref Bitmap left_eye_bitmap,
            ref Bitmap right_eye_bitmap,
            ref float[] face_grid)
        {
            var face_image = GenerateCroppedImage(img, face_rect);
            var left_eye_image = GenerateCroppedImage(img, left_eye_rect);
            var right_eye_image = GenerateCroppedImage(img, right_eye_rect);
            face_grid = GenerateFaceGrid(img, face_rect);

            face_bitmap = face_image.ToBitmap<RgbPixel>();
            left_eye_bitmap = left_eye_image.ToBitmap<RgbPixel>();
            right_eye_bitmap = right_eye_image.ToBitmap<RgbPixel>();
        }

        private static float[] GenerateFaceGrid(
            Array2D<RgbPixel> img,
            DlibDotNet.Rectangle face_rect)
        {
            var image_width = img.Columns;
            var image_height = img.Rows;

            const int grid_size = 25;
            var face_grid_rect = new DlibDotNet.Rectangle(
                (int)((float)face_rect.TopLeft.X / image_width * grid_size),
                (int)((float)face_rect.TopLeft.Y / image_height * grid_size),
                (int)(((float)face_rect.TopLeft.X + face_rect.Width) / image_width * grid_size),
                (int)(((float)face_rect.TopLeft.Y + face_rect.Height) / image_height * grid_size));

            var face_grid = new float[grid_size * grid_size];
            for (int row = 0; row < grid_size; ++row)
            {
                for (int col = 0; col < grid_size; ++col)
                {
                    var bit_value = 0;
                    if (col >= face_grid_rect.TopLeft.X &&
                        col <= face_grid_rect.BottomRight.X &&
                        row >= face_grid_rect.TopLeft.Y &&
                        row <= face_grid_rect.BottomRight.Y)
                    {
                        bit_value = 1;
                    }
                    face_grid[row * grid_size + col] = bit_value;
                }
            }

            return face_grid;
        }

        private static Array2D<RgbPixel> GenerateCroppedImage(
            Array2D<RgbPixel> img,
            DlibDotNet.Rectangle rect)
        {
            DPoint[] dPoint = new DPoint[] {
                    new DPoint(rect.TopLeft.X, rect.TopLeft.Y),
                    new DPoint(rect.TopRight.X, rect.TopRight.Y),
                    new DPoint(rect.BottomLeft.X, rect.BottomLeft.Y),
                    new DPoint(rect.BottomRight.X, rect.BottomRight.Y),
                };
            int width = (int)rect.Width;
            int height = (int)rect.Height;
            var cropped_image = Dlib.ExtractImage4Points(img, dPoint, width, height);
            return cropped_image;
        }

        private static DlibDotNet.Rectangle GetEyeRectSizeNormalized(
            DlibDotNet.Rectangle face_rect,
            DlibDotNet.Rectangle eye_rect)
        {
            // matches face_utilities.py
            // find center of eye
            var eye_center = new DlibDotNet.Point(
                eye_rect.Left + (int)(eye_rect.Width / 2),
                eye_rect.Top + (int)(eye_rect.Height / 2));

            // eye box is 3/10 of the face width
            var eye_side = (int)(3 * face_rect.Width / 10);

            var eye_tl = new DlibDotNet.Point(
                eye_center.X - (int)(eye_side / 2),
                eye_center.Y - (int)(eye_side / 2));

            var eye_rect_size_normalized = new DlibDotNet.Rectangle(
                eye_tl.X,
                eye_tl.Y,
                eye_tl.X + eye_side,
                eye_tl.Y + eye_side);

            return eye_rect_size_normalized;
        }

        private static DlibDotNet.Rectangle GetRectangleRelative(
            DlibDotNet.Rectangle outer,
            DlibDotNet.Rectangle inner)
        {
            // inner rectangle must be within the outer rectangle
            var relative_rect = new DlibDotNet.Rectangle(
                inner.TopLeft.X - outer.TopLeft.X,
                inner.TopLeft.Y - outer.TopLeft.Y,
                inner.BottomRight.X - outer.TopLeft.X,
                inner.BottomRight.Y - outer.TopLeft.Y
                );

            return relative_rect;
        }

        private static DlibDotNet.Rectangle GetRect(
            FullObjectDetection shape,
            int start,
            int end)
        {
            var min_top = int.MaxValue;
            var min_left = int.MaxValue;
            var max_bottom = int.MinValue;
            var max_right = int.MinValue;

            for (var i = start; i <= end; i++)
            {
                var point = shape.GetPart((uint)i);

                if (point.Y < min_top)
                    min_top = point.Y;
                if (point.X < min_left)
                    min_left = point.X;
                if (point.Y > max_bottom)
                    max_bottom = point.Y;
                if (point.X > max_right)
                    max_right = point.X;
            }

            var rect = new DlibDotNet.Rectangle(min_left, min_top, max_right, max_bottom);

            return rect;
        }

        private static Array2D<RgbPixel> BitmapToRgbPixel(
            Bitmap input_bitmap)
        {
            Array2D<RgbPixel> array2d = null;

            var data = input_bitmap.LockBits(new System.Drawing.Rectangle(0, 0, input_bitmap.Width, input_bitmap.Height),
                System.Drawing.Imaging.ImageLockMode.ReadOnly, input_bitmap.PixelFormat);
            try
            {
                var array = new byte[data.Stride * data.Height];
                Marshal.Copy(data.Scan0, array, 0, array.Length);
                array2d = Dlib.LoadImageData<RgbPixel>(ImagePixelFormat.Bgr, array, (uint)input_bitmap.Height, (uint)input_bitmap.Width, (uint)data.Stride);
            }
            finally
            {
                input_bitmap.UnlockBits(data);
            }

            return array2d;
        }

        private static Array2D<RgbPixel> RgbToYCbCr(
            Array2D<RgbPixel> rgb_array)
        {
            var ycbcr_array = new Array2D<RgbPixel>(rgb_array.Rows, rgb_array.Columns);

            var width = rgb_array.Columns;
            var height = rgb_array.Rows;

            //Convert to YCbCr
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    var rgbPixel = rgb_array[y][x];

                    float blue = rgbPixel.Blue;
                    float green = rgbPixel.Green;
                    float red = rgbPixel.Red;

                    var data_y = (byte)((0.299 * red) + (0.587 * green) + (0.114 * blue));
                    var data_cb = (byte)(128 - (0.168736 * red) + (0.331264 * green) + (0.5 * blue));
                    var data_cr = (byte)(128 + (0.5 * red) + (0.418688 * green) + (0.081312 * blue));

                    var ycbcrPixel = ycbcr_array[y][x];

                    ycbcrPixel.Red = data_y;
                    ycbcrPixel.Green = data_cb;
                    ycbcrPixel.Blue = data_cr;

                    ycbcr_array[y][x] = ycbcrPixel;
                }
            }

            return ycbcr_array;
        }
    }
}
