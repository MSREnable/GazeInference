//// GazeInference_WinCpp.cpp : Defines the entry point for the application.
////
//
//#include "framework.h"
//#include "GazeInference_WinCpp.h"
//#include "mnist.h"
//
//
//#define MAX_LOADSTRING 100
//
//// Global Variables:
//HINSTANCE hInst;                                // current instance
//WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
//WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
//
//// Forward declarations of functions included in this code module:
//ATOM                MyRegisterClass(HINSTANCE hInstance);
//BOOL                InitInstance(HINSTANCE, int);
//LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
//INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
//
//int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
//    _In_opt_ HINSTANCE hPrevInstance,
//    _In_ LPWSTR    lpCmdLine,
//    _In_ int       nCmdShow)
//{
//    UNREFERENCED_PARAMETER(hPrevInstance);
//    UNREFERENCED_PARAMETER(lpCmdLine);
//
//    // TODO: Place code here.
//    try {
//        mnist = std::make_unique<MNIST>();
//    }
//    catch (const Ort::Exception& exception) {
//        MessageBoxA(nullptr, exception.what(), "Error:", MB_OK);
//        return 0;
//    }
//
//    // Initialize global strings
//    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
//    LoadStringW(hInstance, IDC_GAZEINFERENCEWINCPP, szWindowClass, MAX_LOADSTRING);
//    MyRegisterClass(hInstance);
//
//    //
//    {
//        BITMAPINFO bmi{};
//        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
//        bmi.bmiHeader.biWidth = MNIST::width;
//        bmi.bmiHeader.biHeight = -MNIST::height;
//        bmi.bmiHeader.biPlanes = 1;
//        bmi.bmiHeader.biBitCount = 32;
//        bmi.bmiHeader.biPlanes = 1;
//        bmi.bmiHeader.biCompression = BI_RGB;
//
//        void* bits;
//        dib = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
//    }
//    if (dib == nullptr) return -1;
//    hdc_dib = CreateCompatibleDC(nullptr);
//    SelectObject(hdc_dib, dib);
//    SelectObject(hdc_dib, CreatePen(PS_SOLID, 2, RGB(0, 0, 0)));
//    RECT rect{ 0, 0, MNIST::width, MNIST::height };
//    FillRect(hdc_dib, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
//
//    // Perform application initialization:
//    if (!InitInstance(hInstance, nCmdShow))
//    {
//        return FALSE;
//    }
//
//    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAZEINFERENCEWINCPP));
//
//    MSG msg;
//
//    // Main message loop:
//    while (GetMessage(&msg, nullptr, 0, 0))
//    {
//        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
//        {
//            TranslateMessage(&msg);
//            DispatchMessage(&msg);
//        }
//    }
//
//    // Cleanup
//    DeleteObject(dib);
//    DeleteDC(hdc_dib);
//    DeleteObject(brush_winner);
//    DeleteObject(brush_bars);
//
//    return (int)msg.wParam;
//}
//
//
//
////
////  FUNCTION: MyRegisterClass()
////
////  PURPOSE: Registers the window class.
////
//ATOM MyRegisterClass(HINSTANCE hInstance)
//{
//    WNDCLASSEXW wcex;
//
//    wcex.cbSize = sizeof(WNDCLASSEX);
//
//    wcex.style = CS_HREDRAW | CS_VREDRAW;
//    wcex.lpfnWndProc = WndProc;
//    wcex.cbClsExtra = 0;
//    wcex.cbWndExtra = 0;
//    wcex.hInstance = hInstance;
//    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAZEINFERENCEWINCPP));
//    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
//    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
//    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GAZEINFERENCEWINCPP);
//    wcex.lpszClassName = szWindowClass;
//    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
//
//    return RegisterClassExW(&wcex);
//}
//
////
////   FUNCTION: InitInstance(HINSTANCE, int)
////
////   PURPOSE: Saves instance handle and creates main window
////
////   COMMENTS:
////
////        In this function, we save the instance handle in a global variable and
////        create and display the main program window.
////
//BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
//{
//    hInst = hInstance; // Store instance handle in our global variable
//
//    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
//        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
//
//    if (!hWnd)
//    {
//        return FALSE;
//    }
//
//    ShowWindow(hWnd, nCmdShow);
//    UpdateWindow(hWnd);
//
//    return TRUE;
//}
//
////
////  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
////
////  PURPOSE: Processes messages for the main window.
////
////  WM_COMMAND  - process the application menu
////  WM_PAINT    - Paint the main window
////  WM_DESTROY  - post a quit message and return
////
////
//LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//    switch (message)
//    {
//    case WM_COMMAND:
//    {
//        int wmId = LOWORD(wParam);
//        // Parse the menu selections:
//        switch (wmId)
//        {
//        case IDM_ABOUT:
//            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
//            break;
//        case IDM_EXIT:
//            DestroyWindow(hWnd);
//            break;
//        default:
//            return DefWindowProc(hWnd, message, wParam, lParam);
//        }
//    }
//    break;
//    case WM_PAINT:
//    {
//        PAINTSTRUCT ps;
//        HDC hdc = BeginPaint(hWnd, &ps);
//        // TODO: Add any drawing code that uses hdc here...
//        // Draw the image
//        StretchBlt(hdc, drawing_area_inset, drawing_area_inset, drawing_area_width, drawing_area_height, hdc_dib, 0, 0, MNIST::width, MNIST::height, SRCCOPY);
//        SelectObject(hdc, GetStockObject(BLACK_PEN));
//        SelectObject(hdc, GetStockObject(NULL_BRUSH));
//        Rectangle(hdc, drawing_area_inset, drawing_area_inset, drawing_area_inset + drawing_area_width, drawing_area_inset + drawing_area_height);
//
//        constexpr int graphs_left = drawing_area_inset + drawing_area_width + 5;
//        constexpr int graph_width = 64;
//        SelectObject(hdc, brush_bars);
//
//        auto least = *std::min_element(mnist->results.begin(), mnist->results.end());
//        auto greatest = mnist->results[mnist->result];
//        auto range = greatest - least;
//
//        int graphs_zero = static_cast<int>(graphs_left - least * graph_width / range);
//
//        // Hilight the winner
//        RECT rc{ graphs_left, static_cast<LONG>(mnist->result) * 16, graphs_left + graph_width + 128, static_cast<LONG>(mnist->result + 1) * 16 };
//        FillRect(hdc, &rc, brush_winner);
//
//        // For every entry, draw the odds and the graph for it
//        SetBkMode(hdc, TRANSPARENT);
//        wchar_t value[80];
//        for (unsigned i = 0; i < 10; i++) {
//            int y = 16 * i;
//            float result = mnist->results[i];
//
//            auto length = wsprintf(value, L"%2d: %d.%02d", i, int(result), abs(int(result * 100) % 100));
//            TextOut(hdc, graphs_left + graph_width + 5, y, value, length);
//
//            Rectangle(hdc, graphs_zero, y + 1, static_cast<int>(graphs_zero + result * graph_width / range), y + 14);
//        }
//
//        // Draw the zero line
//        MoveToEx(hdc, graphs_zero, 0, nullptr);
//        LineTo(hdc, graphs_zero, 16 * 10);
//
//        EndPaint(hWnd, &ps);
//    }
//    break;
//
//    case WM_LBUTTONDOWN: {
//        SetCapture(hWnd);
//        painting = true;
//        int x = (GET_X_LPARAM(lParam) - drawing_area_inset) / drawing_area_scale;
//        int y = (GET_Y_LPARAM(lParam) - drawing_area_inset) / drawing_area_scale;
//        MoveToEx(hdc_dib, x, y, nullptr);
//    }
//    break;
//
//
//    case WM_MOUSEMOVE:
//        if (painting) {
//            int x = (GET_X_LPARAM(lParam) - drawing_area_inset) / drawing_area_scale;
//            int y = (GET_Y_LPARAM(lParam) - drawing_area_inset) / drawing_area_scale;
//            LineTo(hdc_dib, x, y);
//            InvalidateRect(hWnd, nullptr, false);
//        }
//        break;
//
//    case WM_CAPTURECHANGED:
//        painting = false;
//        break;
//
//    case WM_LBUTTONUP:
//        ReleaseCapture();
//        ConvertDibToMnist();
//        mnist->Run();
//        InvalidateRect(hWnd, nullptr, true);
//        break;
//
//    case WM_RBUTTONDOWN:  // Erase the image
//    {
//        RECT rect{ 0, 0, MNIST::width, MNIST::height };
//        FillRect(hdc_dib, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
//        InvalidateRect(hWnd, nullptr, false);
//    }
//    break;
//
//    case WM_DESTROY:
//        PostQuitMessage(0);
//        break;
//    default:
//        return DefWindowProc(hWnd, message, wParam, lParam);
//    }
//    return 0;
//}
//
//// Message handler for about box.
//INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
//{
//    UNREFERENCED_PARAMETER(lParam);
//    switch (message)
//    {
//    case WM_INITDIALOG:
//        return (INT_PTR)TRUE;
//
//    case WM_COMMAND:
//        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
//        {
//            EndDialog(hDlg, LOWORD(wParam));
//            return (INT_PTR)TRUE;
//        }
//        break;
//    }
//    return (INT_PTR)FALSE;
//}
