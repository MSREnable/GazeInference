#include "FrameCapture.h"



FrameCapture::FrameCapture(HWND hWnd) : 
    g_hwnd(hWnd)
{

}

//-------------------------------------------------------------------
// OnCreate: Handler for WM_CREATE
//-------------------------------------------------------------------

BOOL FrameCapture::OnCreate()
{
    DEV_BROADCAST_DEVICEINTERFACE di = { 0 };
    di.dbcc_size = sizeof(di);
    di.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    di.dbcc_classguid = KSCATEGORY_CAPTURE;

    g_hdevnotify = RegisterDeviceNotification(
        g_hwnd,
        &di,
        DEVICE_NOTIFY_WINDOW_HANDLE
    );

    if (g_hdevnotify == NULL)
    {
        ShowErrorMessage(
            g_hwnd,
            L"RegisterDeviceNotification failed.",
            HRESULT_FROM_WIN32(GetLastError())
        );
        return FALSE;
    }

    return TRUE;
}



//-------------------------------------------------------------------
// OnClose: Handler for WM_CLOSE 
//-------------------------------------------------------------------

void FrameCapture::OnClose()
{
    if (g_hdevnotify)
    {
        UnregisterDeviceNotification(g_hdevnotify);
    }

    PostQuitMessage(0);
}


//-------------------------------------------------------------------
// OnPaint: Handler for WM_PAINT
//-------------------------------------------------------------------

void FrameCapture::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = 0;

    hdc = BeginPaint(g_hwnd, &ps);

    if (hdc)
    {
        if (g_pPreview && g_pPreview->HasVideo())
        {
            g_pPreview->UpdateVideo();
        }
        else
        {
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_APPWORKSPACE + 1));
        }
    }
    EndPaint(g_hwnd, &ps);
}


//-------------------------------------------------------------------
// OnSize: Handler for WM_SIZE
//-------------------------------------------------------------------

void FrameCapture::OnSize()
{
    if (g_pPreview)
    {
        // Resize the video to cover the entire client area.
        g_pPreview->UpdateVideo();
    }
}


//-------------------------------------------------------------------
// OnCommand: Handler for WM_COMMAND
//-------------------------------------------------------------------

void FrameCapture::OnCommand(int id, HWND /*hwndCtl*/, UINT /*codeNotify*/)
{
    switch (id)
    {
    case ID_FILE_CHOOSEDEVICE:
        OnChooseDevice();
        break;
    }
}

//-------------------------------------------------------------------
// OnChooseDevice
// 
// Displays a dialog for the user to select a capture device.
//-------------------------------------------------------------------

void FrameCapture::OnChooseDevice()
{
    HRESULT hr = S_OK;
    ChooseDeviceParam param = { 0 };

    IMFAttributes* pAttributes = NULL;

    // Release the previous instance of the preview object, if any.
    if (g_pPreview)
    {
        g_pPreview->CloseDevice();
        SafeRelease(&g_pPreview);
    }

    // Create a new instance of the preview object.
    hr = CPreview::CreateInstance(g_hwnd, &g_pPreview);

    // Create an attribute store to specify the enumeration parameters.

    if (SUCCEEDED(hr))
    {
        hr = MFCreateAttributes(&pAttributes, 1);
    }

    // Ask for source type = video capture devices

    if (SUCCEEDED(hr))
    {
        hr = pAttributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );
    }

    // Enumerate devices.

    if (SUCCEEDED(hr))
    {
        hr = MFEnumDeviceSources(pAttributes, &param.ppDevices, &param.count);
    }

    if (SUCCEEDED(hr))
    {
        // Ask the user to select one.
        INT_PTR result = DialogBoxParam(
            GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_CHOOSE_DEVICE),
            g_hwnd,
            DlgProc,
            (LPARAM)&param
        );

        //TODO: DialogBox isn't working for x64 using param.selection
        //if ((result == IDOK) && (param.selection != (UINT32)-1))
        if ((param.selection != (UINT32)-1))
        {
            UINT iDevice = param.selection;

            if (iDevice >= param.count)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                // Give this source to the CPreview object for preview.
                hr = g_pPreview->SetDevice(param.ppDevices[iDevice]);
            }
        }
    }

    SafeRelease(&pAttributes);

    for (DWORD i = 0; i < param.count; i++)
    {
        SafeRelease(&param.ppDevices[i]);
    }
    CoTaskMemFree(param.ppDevices);

    if (FAILED(hr))
    {
        ShowErrorMessage(g_hwnd, L"Cannot create the video capture device", hr);
    }
}

void FrameCapture::OnDeviceLost(DEV_BROADCAST_HDR* pHdr)
{
    if (g_pPreview == NULL || pHdr == NULL)
    {
        return;
    }

    HRESULT hr = S_OK;
    BOOL bDeviceLost = FALSE;

    hr = g_pPreview->CheckDeviceLost(pHdr, &bDeviceLost);

    if (FAILED(hr) || bDeviceLost)
    {
        g_pPreview->CloseDevice();

        MessageBox(g_hwnd, L"Lost the capture device.", WINDOW_NAME, MB_OK);
    }
}



//-------------------------------------------------------------------
// DlgProc: Window procedure for the dialog.
//-------------------------------------------------------------------

INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static ChooseDeviceParam* pParam = NULL;

    switch (msg)
    {
    case WM_INITDIALOG:
        pParam = (ChooseDeviceParam*)lParam;
        OnInitDialog(hwnd, pParam);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            OnOK(hwnd, pParam);
            EndDialog(hwnd, LOWORD(wParam));
            return TRUE;

        case IDCANCEL:
            EndDialog(hwnd, LOWORD(wParam));
            return TRUE;
        }
        break;
    }

    return FALSE;
}

//-------------------------------------------------------------------
// OnInitDialog: Handler for WM_INITDIALOG
//-------------------------------------------------------------------

HRESULT OnInitDialog(HWND hwnd, ChooseDeviceParam* pParam)
{
    HRESULT hr = S_OK;

    HWND hList = GetDlgItem(hwnd, IDC_DEVICE_LIST);

    // Display a list of the devices.

    for (DWORD i = 0; i < pParam->count; i++)
    {
        WCHAR* szFriendlyName = NULL;

        hr = pParam->ppDevices[i]->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
            &szFriendlyName,
            NULL
        );

        if (FAILED(hr))
        {
            break;
        }

        int index = ListBox_AddString(hList, szFriendlyName);

        ListBox_SetItemData(hList, index, i);

        CoTaskMemFree(szFriendlyName);
    }

    // Assume no selection for now.
    pParam->selection = (UINT32)-1;

    if (pParam->count == 0)
    {
        // If there are no devices, disable the "OK" button.
        EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
    }
    else
    {
        // Select the first device in the list.
        ListBox_SetCurSel(hList, 0);
    }

    return hr;
}


//-------------------------------------------------------------------
// OnOK: Handler for the OK button
//-------------------------------------------------------------------

HRESULT OnOK(HWND hwnd, ChooseDeviceParam* pParam)
{
    HWND hList = GetDlgItem(hwnd, IDC_DEVICE_LIST);

    // Get the current selection and return it to the application.
    int sel = ListBox_GetCurSel(hList);

    if (sel != LB_ERR)
    {
        pParam->selection = (UINT32)ListBox_GetItemData(hList, sel);
    }

    return S_OK;
}


