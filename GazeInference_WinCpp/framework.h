// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <windowsx.h> // GET_X_LPARAM, GET_Y_LPARAM
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <array>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <tuple>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>


#include <onnxruntime_cxx_api.h>
#pragma comment(lib, "onnxruntime.lib")



#include <opencv2/opencv.hpp>

//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc.hpp>
//#include <opencv2/dnn/dnn.hpp>
//#include <opencv2/imgcodecs.hpp>
//#include <opencv2/videoio.hpp>
//#include <opencv2/imgproc/imgproc.hpp>


#pragma comment(lib, "opencv_world451.lib")

//#include <Dshow.h>

using namespace cv;
using namespace std;


