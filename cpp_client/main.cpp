#include <windows.h>
#include <winhttp.h>
#include <wincred.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <iphlpapi.h>
#include <intrin.h>
#include "resource.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "comctl32.lib")

// Constants
#define APP_TITLE L"Secure License Client"
#define SERVER_URL L"localhost"
#define SERVER_PORT 5000

// Global variables
HWND g_hWndEdit, g_hWndButton, g_hWndCheck, g_hWndStatus;
HBRUSH g_hBrushBg, g_hBrushEdit, g_hBrushButton;
HFONT g_hFontTitle, g_hFontNormal, g_hFontStatus;

// Function to get HWID
std::wstring GetHWID() {
    std::wstringstream hwid;

    // Get CPU info
    int cpuInfo[4];
    __cpuid(cpuInfo, 0);
    hwid << std::hex << cpuInfo[1] << cpuInfo[3] << cpuInfo[2];

    // Get MAC address
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufferSize = sizeof(adapterInfo);
    if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
        for (int i = 0; i < 6; i++) {
            hwid << std::hex << std::setw(2) << std::setfill(L'0')
                 << (int)adapterInfo[0].Address[i];
        }
    }

    return hwid.str();
}

// Save license key securely
void SaveLicenseKey(const std::wstring& key) {
    CREDENTIAL cred = {0};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = (LPWSTR)L"SecureLicense_Key";
    cred.CredentialBlobSize = (DWORD)(key.length() * sizeof(wchar_t));
    cred.CredentialBlob = (LPBYTE)key.c_str();
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

    CredWriteW(&cred, 0);
}

// Load license key
std::wstring LoadLicenseKey() {
    PCREDENTIALW cred;
    if (CredReadW(L"SecureLicense_Key", CRED_TYPE_GENERIC, 0, &cred)) {
        std::wstring key((wchar_t*)cred->CredentialBlob,
                        cred->CredentialBlobSize / sizeof(wchar_t));
        CredFree(cred);
        return key;
    }
    return L"";
}

// Delete license key
void DeleteLicenseKey() {
    CredDeleteW(L"SecureLicense_Key", CRED_TYPE_GENERIC, 0);
}

// HTTP POST request
std::wstring HttpPost(const std::wstring& path, const std::wstring& data) {
    std::wstring response;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

    hSession = WinHttpOpen(L"Secure License Client/1.0",
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME,
                          WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession) {
        hConnect = WinHttpConnect(hSession, SERVER_URL, SERVER_PORT, 0);
    }

    if (hConnect) {
        hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
                                     NULL, WINHTTP_NO_REFERER,
                                     WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    }

    if (hRequest) {
        std::wstring headers = L"Content-Type: application/json\r\n";

        if (WinHttpSendRequest(hRequest, headers.c_str(), -1L,
                              (LPVOID)data.c_str(),
                              (DWORD)(data.length() * sizeof(wchar_t)),
                              (DWORD)(data.length() * sizeof(wchar_t)), 0)) {

            if (WinHttpReceiveResponse(hRequest, NULL)) {
                DWORD bytesAvailable = 0;
                std::vector<char> buffer;

                do {
                    bytesAvailable = 0;
                    WinHttpQueryDataAvailable(hRequest, &bytesAvailable);

                    if (bytesAvailable > 0) {
                        std::vector<char> temp(bytesAvailable + 1);
                        DWORD bytesRead = 0;

                        if (WinHttpReadData(hRequest, temp.data(),
                                           bytesAvailable, &bytesRead)) {
                            buffer.insert(buffer.end(), temp.begin(),
                                        temp.begin() + bytesRead);
                        }
                    }
                } while (bytesAvailable > 0);

                if (!buffer.empty()) {
                    buffer.push_back('\0');
                    int size = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), -1, NULL, 0);
                    std::vector<wchar_t> wbuffer(size);
                    MultiByteToWideChar(CP_UTF8, 0, buffer.data(), -1, wbuffer.data(), size);
                    response = wbuffer.data();
                }
            }
        }
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return response;
}

// Validate license
void ValidateLicense(HWND hwnd) {
    wchar_t key[256];
    GetWindowText(g_hWndEdit, key, 256);

    if (wcslen(key) == 0) {
        SetWindowText(g_hWndStatus, L"Please enter your license key");
        return;
    }

    SetWindowText(g_hWndStatus, L"Validating...");
    EnableWindow(g_hWndButton, FALSE);

    std::wstring hwid = GetHWID();
    std::wstringstream json;
    json << L"{\"key\":\"" << key << L"\",\"hwid\":\"" << hwid << L"\"}";

    std::wstring response = HttpPost(L"/api/validate", json.str());

    EnableWindow(g_hWndButton, TRUE);

    if (response.find(L"\"success\":true") != std::wstring::npos) {
        // Save key if remember is checked
        if (SendMessage(g_hWndCheck, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            SaveLicenseKey(key);
        }

        // Parse response for details
        std::wstring message = L"✓ Authentication Successful!\n\n";

        size_t userPos = response.find(L"\"username\":\"");
        if (userPos != std::wstring::npos) {
            size_t start = userPos + 13;
            size_t end = response.find(L"\"", start);
            message += L"User: " + response.substr(start, end - start) + L"\n";
        }

        size_t expiresPos = response.find(L"\"expires_at\":\"");
        if (expiresPos != std::wstring::npos) {
            size_t start = expiresPos + 15;
            size_t end = response.find(L"\"", start);
            message += L"Expires: " + response.substr(start, end - start) + L"\n";
        }

        size_t timePos = response.find(L"\"time_remaining\":\"");
        if (timePos != std::wstring::npos) {
            size_t start = timePos + 19;
            size_t end = response.find(L"\"", start);
            message += L"Time Remaining: " + response.substr(start, end - start);
        }

        if (response.find(L"\"is_permanent\":true") != std::wstring::npos) {
            message = L"✓ Authentication Successful!\n\nLicense Type: Permanent\nStatus: Active";
        }

        MessageBox(hwnd, message.c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
        SetWindowText(g_hWndStatus, L"✓ Authentication Successful");
    }
    else if (response.find(L"suspended") != std::wstring::npos) {
        std::wstring message = L"Your license has been suspended.\n\n";

        size_t msgPos = response.find(L"\"message\":\"");
        if (msgPos != std::wstring::npos) {
            size_t start = msgPos + 12;
            size_t end = response.find(L"\"", start);
            message += response.substr(start, end - start) + L"\n\n";
        }

        message += L"Please contact your administrator.";

        MessageBox(hwnd, message.c_str(), L"License Suspended", MB_OK | MB_ICONERROR);
        SetWindowText(g_hWndStatus, L"✗ License Suspended");

        DeleteLicenseKey();
        SendMessage(g_hWndCheck, BM_SETCHECK, BST_UNCHECKED, 0);
    }
    else {
        std::wstring message = L"Authentication Failed\n\n";

        size_t msgPos = response.find(L"\"message\":\"");
        if (msgPos != std::wstring::npos) {
            size_t start = msgPos + 12;
            size_t end = response.find(L"\"", start);
            message += response.substr(start, end - start);
        }

        MessageBox(hwnd, message.c_str(), L"Error", MB_OK | MB_ICONERROR);
        SetWindowText(g_hWndStatus, L"✗ Authentication Failed");
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Create fonts
            g_hFontTitle = CreateFont(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                     CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            g_hFontNormal = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            g_hFontStatus = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            // Create brushes
            g_hBrushBg = CreateSolidBrush(RGB(30, 30, 46));
            g_hBrushEdit = CreateSolidBrush(RGB(49, 50, 68));
            g_hBrushButton = CreateSolidBrush(RGB(137, 180, 250));

            // Title
            HWND hTitle = CreateWindow(L"STATIC", L"Secure License",
                                      WS_CHILD | WS_VISIBLE | SS_CENTER,
                                      50, 30, 400, 40, hwnd, NULL, NULL, NULL);
            SendMessage(hTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

            // Label
            HWND hLabel = CreateWindow(L"STATIC", L"License Key:",
                                      WS_CHILD | WS_VISIBLE,
                                      50, 100, 400, 25, hwnd, NULL, NULL, NULL);
            SendMessage(hLabel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            // Edit box
            g_hWndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                       WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                       50, 130, 400, 35, hwnd, NULL, NULL, NULL);
            SendMessage(g_hWndEdit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            // Checkbox
            g_hWndCheck = CreateWindow(L"BUTTON", L"Remember my key",
                                      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                      50, 180, 200, 25, hwnd, NULL, NULL, NULL);
            SendMessage(g_hWndCheck, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            // Login button
            g_hWndButton = CreateWindow(L"BUTTON", L"Authenticate",
                                       WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                       50, 220, 400, 40, hwnd, (HMENU)1, NULL, NULL);
            SendMessage(g_hWndButton, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            // Clear button
            HWND hClear = CreateWindow(L"BUTTON", L"Clear",
                                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      50, 270, 400, 35, hwnd, (HMENU)2, NULL, NULL);
            SendMessage(hClear, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            // Status label
            g_hWndStatus = CreateWindow(L"STATIC", L"",
                                       WS_CHILD | WS_VISIBLE | SS_CENTER,
                                       50, 320, 400, 30, hwnd, NULL, NULL, NULL);
            SendMessage(g_hWndStatus, WM_SETFONT, (WPARAM)g_hFontStatus, TRUE);

            // Load saved key
            std::wstring savedKey = LoadLicenseKey();
            if (!savedKey.empty()) {
                SetWindowText(g_hWndEdit, savedKey.c_str());
                SendMessage(g_hWndCheck, BM_SETCHECK, BST_CHECKED, 0);
            }

            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(205, 214, 244));
            SetBkColor(hdcStatic, RGB(30, 30, 46));
            return (LRESULT)g_hBrushBg;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, RGB(205, 214, 244));
            SetBkColor(hdcEdit, RGB(49, 50, 68));
            return (LRESULT)g_hBrushEdit;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == 1) { // Login button
                ValidateLicense(hwnd);
            }
            else if (LOWORD(wParam) == 2) { // Clear button
                SetWindowText(g_hWndEdit, L"");
                SetWindowText(g_hWndStatus, L"");

                if (SendMessage(g_hWndCheck, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    if (MessageBox(hwnd, L"Remove saved license key?", L"Confirm",
                                  MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        DeleteLicenseKey();
                        SendMessage(g_hWndCheck, BM_SETCHECK, BST_UNCHECKED, 0);
                    }
                }
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            DeleteObject(g_hFontTitle);
            DeleteObject(g_hFontNormal);
            DeleteObject(g_hFontStatus);
            DeleteObject(g_hBrushBg);
            DeleteObject(g_hBrushEdit);
            DeleteObject(g_hBrushButton);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// WinMain entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 46));
    wc.lpszClassName = L"SecureLicenseClass";

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error",
                  MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowEx(0, L"SecureLicenseClass", APP_TITLE,
                               WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                               CW_USEDEFAULT, CW_USEDEFAULT, 520, 420,
                               NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error",
                  MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
