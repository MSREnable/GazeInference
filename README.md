# GazeInference
Code for running inference on images, from webcams, etc.

# Building

We recommend building the release build first. That build is standalone and 
therefore has the least dependencies. You will need to install [vcpkg](https://vcpkg.io/), 
integrate the vcpkg into your msbuild/visual studio environment via `vcpkg integrate install`
and then install dlib via `vcpkg install dlib:x86-windows dlib:x64-windows`.

# Running

You will need three additional files to run the inference pipeline. Please download them and
put them in the assets directory:

- [shape_predictor_68_face_landmarks.dat](https://github.com/italojs/facial-landmarks-recognition/raw/master/shape_predictor_68_face_landmarks.dat)
- [version-RFB-320_without_postprocessing.onnx](https://github.com/Linzaer/Ultra-Light-Fast-Generic-Face-Detector-1MB/raw/master/models/onnx/version-RFB-320_without_postprocessing.onnx)
- [version-slim-320_without_postprocessing.onnx](https://github.com/Linzaer/Ultra-Light-Fast-Generic-Face-Detector-1MB/raw/master/models/onnx/version-slim-320_without_postprocessing.onnx)

