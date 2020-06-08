using Microsoft.ML.Data;

namespace GazeInference_Library
{
    public class ITrackerPrediction
    {
        [ColumnName("data")]
        [VectorType(new int[] { 1, 2 })]
        public float[] Data { get; set; }
    }
}
