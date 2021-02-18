// GazeInference_WinCpp.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "GazeInference_WinCpp.h"
#include "FrameCapture.h"
#include "ITrackerModel.h"
#include "SqueezeNet.h"
#include "UltraFaceNet.h"



#define MAX_LOADSTRING 100
#define DEFAULT_VIDEO_WIDTH 300
#define DEFAULT_VIDEO_HEIGHT 300

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


FrameCapture* g_pFrameCapture = NULL;
//const wchar_t* modelFilepath = L"assets/SqueezeNet.onnx";
//const wchar_t* labelFilepath = L"assets/Labels.txt";

const wchar_t* modelFilepath = L"assets/itracker.onnx";
const wchar_t* labelFilepath = NULL;
std::unique_ptr<ITrackerModel> model;

std::unique_ptr<ITrackerModel> OnCreate(HWND hwnd);
void OnPaint(HWND hwnd);
void OnChar(HWND hwnd, wchar_t c);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	(void)HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	// Initialize COM
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
	{
		//Msg(TEXT("CoInitialize Failed!\r\n"));
		exit(1);
	}


	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_GAZEINFERENCEWINCPP, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Use the initialized variables here

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAZEINFERENCEWINCPP));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Cleanup here
	CoUninitialize(); // Release COM

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAZEINFERENCEWINCPP));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GAZEINFERENCEWINCPP);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, DEFAULT_VIDEO_WIDTH, DEFAULT_VIDEO_HEIGHT, nullptr, nullptr, hInstance, nullptr);
	
	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		model = OnCreate(hWnd);
		/*g_pFrameCapture = new (std::nothrow) FrameCapture(hWnd);
		if (!g_pFrameCapture)
			return -1;
		else {
			g_pFrameCapture->OnCreate();
			g_pFrameCapture->OnChooseDevice();
			return 0;
		}*/
		break;

	case WM_PAINT:
		OnPaint(hWnd);
		//g_pFrameCapture->OnPaint();
		break;

	case WM_CLOSE:// Close(X) button;
		// Destroy window is called by default by DefWinProc
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		// Hide the main window while the graph is destroyed
		ShowWindow(hWnd, SW_HIDE);
		//g_pFrameCapture->OnClose();
		//delete g_pFrameCapture;
		PostQuitMessage(0);
		break;

	case WM_CHAR:
		OnChar(hWnd, (wchar_t)wParam);
		return 0;

	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		//Parse the menu selections:
		switch (wmId)
		{
		case 1://Button1
			InvalidateRect(hWnd, nullptr, true); // redraw (sends a WM_PAINT message)
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;


	case WM_SIZE:
		//g_pFrameCapture->OnSize();
		break;

	case WM_WINDOWPOSCHANGED:
		break;

	case WM_DEVICECHANGE:
		//g_pFrameCapture->OnDeviceLost((PDEV_BROADCAST_HDR)lParam);
		return TRUE;

	case WM_ERASEBKGND:
		return 1;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);;
}


std::unique_ptr<ITrackerModel> OnCreate(HWND hWnd) {
	std::unique_ptr<ITrackerModel> model;
	try {
		model = std::make_unique<ITrackerModel>(modelFilepath);
	}
	catch (const Ort::Exception& exception) {
		MessageBoxA(nullptr, exception.what(), "Error:", MB_OK);
	}
	return model;
}

void OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);

	// TODO: Add any drawing code that uses hdc here...

	//// Ex.1: Using an imagepath to load and fill the inputTensor
	//std::unique_ptr<SqueezeNet> squeezeNet = std::make_unique<SqueezeNet>(L"assets/SqueezeNet.onnx", L"assets/Labels.txt");
	//squeezeNet->getFrameFromImagePath("assets/kitten_224.png");
	//squeezeNet->applyTransformations();
	//squeezeNet->fillInputTensor();
	//squeezeNet->run();
	//squeezeNet->processOutput(true);


	//// Ex.2: Using the default camera to get frame and fill the inputTensor in a loop
	//std::unique_ptr<SqueezeNet> squeezeNet = std::make_unique<SqueezeNet>(L"assets/SqueezeNet.onnx", L"assets/Labels.txt");
	//squeezeNet->init();
	//while (squeezeNet->getCameraFrame()) {
	//	squeezeNet->applyTransformations();
	//	squeezeNet->fillInputTensor();
	//	squeezeNet->run();
	//	squeezeNet->processOutput(true);
	//}

	// Ex.4: Working iTracker model inference to generate (x,y) coordinates
	model->init();
	cv::Mat frame;
	std::vector<cv::Mat> roi_images;
	std::vector<float> coordinates_XY;
	bool is_valid;

	while (model->getFrame()) { //reads a new frame
		is_valid = model->applyTransformations();
		if (!is_valid)
			continue;
		model->fillInputTensor();
		model->run();
		coordinates_XY = model->processOutput();
	}

	//model->benchmark2();
	//model->benchmark_dlib();
	//model->benchmark_ultraLight();

	//std::unique_ptr<UltraFaceNet> ultraFaceNet = std::make_unique<UltraFaceNet>(L"assets/version-RFB-320.onnx");
	//ultraFaceNet->init();
	//while (ultraFaceNet->getCameraFrame()) {
	//	ultraFaceNet->applyTransformations();
	//	ultraFaceNet->fillInputTensor();
	//	ultraFaceNet->run();
	//	ultraFaceNet->processOutput(true);
	//}
	
	EndPaint(hWnd, &ps);
}

void OnChar(HWND hWnd, wchar_t c)
{
	switch (c)
	{
	case L'o':
	case L'O':
		break;

	case L'p':
	case L'P':
		//if (g_pFrameCapture->State() == Running)
		//{
		//	g_pFrameCapture->ChangePreviewState(0);//Pause
		//}
		//else
		//{	
		//	g_pFrameCapture->ChangePreviewState(1);//Play
		//}
		break;
	}
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
