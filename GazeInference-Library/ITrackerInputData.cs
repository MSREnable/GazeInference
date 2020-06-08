using Microsoft.ML.Data;
using Microsoft.ML.Transforms.Image;
using System.Drawing;

namespace GazeInference_Library
{
    public class ITrackerInputData
    {
        [ImageType(ITrackerImageSettings.imageHeight, ITrackerImageSettings.imageWidth)]
        public Bitmap face { get; set; }

        [ImageType(ITrackerImageSettings.imageHeight, ITrackerImageSettings.imageWidth)]
        public Bitmap eyesLeft { get; set; }

        [ImageType(ITrackerImageSettings.imageHeight, ITrackerImageSettings.imageWidth)]
        public Bitmap eyesRight { get; set; }

        [VectorType(new int[] { 1, 625 })]
        public float[] faceGrid { get; set; }
    }
}
