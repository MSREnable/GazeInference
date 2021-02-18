#pragma once
#include "Preview.h"
#include <WinUser.h>
#pragma comment(lib, "User32.lib")


#define IDR_MENU1                       101  
#define IDD_CHOOSE_DEVICE               102  
#define IDC_LIST1                       1001  
#define IDC_DEVICE_LIST                 1002  
#define ID_FILE_CHOOSEDEVICE            40001  
#define ID_FILE_CAPTURE                 50001  
#define IDC_STATIC                      -1  
#define IMAGE                          301

// Constants 
const WCHAR CLASS_NAME[] = L"SimpleCapture Window Class";
const WCHAR WINDOW_NAME[] = L"SimpleCapture Sample Application";

class FrameCapture
{

private:
    // Global variables
    CPreview* g_pPreview = NULL;
    HDEVNOTIFY  g_hdevnotify = NULL;
    HWND        g_hwnd;

public:
    FrameCapture(HWND hWnd);
    //~FrameCapture();

    // Window message handlers
    BOOL    OnCreate();
    void    OnClose();
    void    OnPaint();
    void    OnSize();
    void    OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    void    OnDeviceLost(DEV_BROADCAST_HDR* pHdr);

    // Command handlers
    void    OnChooseDevice();
};

HRESULT OnInitDialog(HWND hwnd, ChooseDeviceParam* pParam);
HRESULT OnOK(HWND hwnd, ChooseDeviceParam* pParam);
INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT OnInitDialog(HWND hwnd, ChooseDeviceParam* pParam);