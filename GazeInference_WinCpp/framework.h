// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// Windows Header Files
#include <windows.h>
#include <windowsx.h>		// GET_X_LPARAM, GET_Y_LPARAM

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <strmif.h>			// IGraphBuilder, ICaptureGraphBuilder2
#include <dshow.h>			// DirectShow header file.  CLSID_CaptureGraphBuilder2, CLSID_FilterGraph
#include "atlbase.h"		// ATL smart pointers CCOMPtr
#pragma comment(lib, "strmiids")	// Link to DirectShow GUIDs.

#include <new>
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
#include <comutil.h>
#include <stdio.h>
#include <ctype.h>
#include <strsafe.h>
#include <thread>
#include <future>


// Headers for Media Foundation
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <Wmcodecdsp.h>
#include <assert.h>
#include <Dbt.h>
#include <shlwapi.h>
#include <mfobjects.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "dxva2.lib")
#pragma comment(lib, "evr.lib")


// ONNX
#include <onnxruntime_cxx_api.h>


// OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>


// Custom Headers
#include "logging.h"


template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}
