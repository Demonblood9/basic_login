#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <sstream>
#include <commctrl.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "comctl32.lib")

// Control IDs
#define IDC_KEY_EDIT        1001
#define IDC_LOGIN_BUTTON    1002
#define IDC_STATUS_LABEL    1003
#define IDC_RESULT_LABEL    1004

// Global variables
HWND hKeyEdit;
HWND hLoginButton;
HWND hStatusLabel;
HWND hResultLabel;
HFONT hTitleFont;
HFONT hNormalFont;

// Server configuration
const wchar_t* SERVER_HOST = L"localhost";
const int SERVER_PORT = 5000;
const wchar_t* VALIDATE_PATH = L"/api/validate";

// JSON parsing helper - simple extraction for our use case
std::wstring ExtractJsonValue(const std::wstring& json, const std::wstring& key) {
    size_t keyPos = json.find(L"\"" + key + L"\"");
    if (keyPos == std::wstring::npos) return L"";

    size_t colonPos = json.find(L":", keyPos);
    if (colonPos == std::wstring::npos) return L"";

    size_t startQuote = json.find(L"\"", colonPos);
    if (startQuote == std::wstring::npos) return L"";

    size_t endQuote = json.find(L"\"", startQuote + 1);
    if (endQuote == std::wstring::npos) return L"";

    return json.substr(startQuote + 1, endQuote - startQuote - 1);
}

bool ExtractJsonBool(const std::wstring& json, const std::wstring& key) {
    size_t keyPos = json.find(L"\"" + key + L"\"");
    if (keyPos == std::wstring::npos) return false;

    size_t colonPos = json.find(L":", keyPos);
    if (colonPos == std::wstring::npos) return false;

    size_t truePos = json.find(L"true", colonPos);

    if (truePos != std::wstring::npos && truePos < colonPos + 10) return true;
    return false;
}

// Convert string to wstring
std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}

// Convert wstring to string
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    // Don't include null terminator (-1) in the conversion for JSON data
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &str[0], size, NULL, NULL);
    return str;
}

// Validate key with backend server
bool ValidateKey(const std::wstring& key, std::wstring& username, std::wstring& message) {
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    bool success = false;

    // Declare variables before any goto statements to avoid MSVC errors
    std::wstring headers;
    std::string responseBody;
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;

    // Create JSON request body
    std::wstring jsonKey = key;
    // Escape any special characters in key if needed
    std::wstring jsonBody = L"{\"key\":\"" + jsonKey + L"\"}";
    std::string jsonBodyUtf8 = WStringToString(jsonBody);

    // Initialize WinHTTP
    hSession = WinHttpOpen(L"SecureLoginClient/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS,
                           0);

    if (!hSession) {
        message = L"Failed to initialize HTTP client";
        goto cleanup;
    }

    // Connect to server
    hConnect = WinHttpConnect(hSession, SERVER_HOST, SERVER_PORT, 0);
    if (!hConnect) {
        message = L"Failed to connect to server";
        goto cleanup;
    }

    // Create HTTP request
    hRequest = WinHttpOpenRequest(hConnect,
                                   L"POST",
                                   VALIDATE_PATH,
                                   NULL,
                                   WINHTTP_NO_REFERER,
                                   WINHTTP_DEFAULT_ACCEPT_TYPES,
                                   0);

    if (!hRequest) {
        message = L"Failed to create HTTP request";
        goto cleanup;
    }

    // Set headers
    headers = L"Content-Type: application/json\r\n";
    WinHttpAddRequestHeaders(hRequest,
                            headers.c_str(),
                            (DWORD)headers.length(),
                            WINHTTP_ADDREQ_FLAG_ADD);

    // Send request
    if (!WinHttpSendRequest(hRequest,
                           WINHTTP_NO_ADDITIONAL_HEADERS,
                           0,
                           (LPVOID)jsonBodyUtf8.c_str(),
                           (DWORD)jsonBodyUtf8.length(),
                           (DWORD)jsonBodyUtf8.length(),
                           0)) {
        message = L"Failed to send request";
        goto cleanup;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        message = L"Failed to receive response";
        goto cleanup;
    }

    // Get status code
    WinHttpQueryHeaders(hRequest,
                       WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                       NULL,
                       &statusCode,
                       &statusCodeSize,
                       NULL);

    // Read response body
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
            message = L"Error querying data";
            goto cleanup;
        }

        if (dwSize == 0) break;

        char* buffer = new char[dwSize + 1];
        ZeroMemory(buffer, dwSize + 1);

        if (!WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded)) {
            delete[] buffer;
            message = L"Error reading data";
            goto cleanup;
        }

        responseBody.append(buffer, dwDownloaded);
        delete[] buffer;

    } while (dwSize > 0);

    // Parse JSON response
    {
        std::wstring responseWStr = StringToWString(responseBody);
        bool successFlag = ExtractJsonBool(responseWStr, L"success");
        message = ExtractJsonValue(responseWStr, L"message");
        username = ExtractJsonValue(responseWStr, L"username");

        if (statusCode == 200 && successFlag) {
            success = true;
        } else {
            success = false;
            if (message.empty()) {
                message = L"Authentication failed";
            }
        }
    }

cleanup:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return success;
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Create title label
            HWND hTitle = CreateWindowW(L"STATIC", L"Secure Key Authentication",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                0, 20, 500, 40,
                hwnd, NULL, NULL, NULL);
            SendMessage(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

            // Create key label
            CreateWindowW(L"STATIC", L"Enter your access key:",
                WS_VISIBLE | WS_CHILD,
                50, 80, 200, 20,
                hwnd, NULL, NULL, NULL);

            // Create key input (edit control)
            hKeyEdit = CreateWindowW(L"EDIT", L"",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
                50, 105, 400, 30,
                hwnd, (HMENU)IDC_KEY_EDIT, NULL, NULL);
            SendMessage(hKeyEdit, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

            // Create login button
            hLoginButton = CreateWindowW(L"BUTTON", L"Login",
                WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                200, 150, 100, 35,
                hwnd, (HMENU)IDC_LOGIN_BUTTON, NULL, NULL);
            SendMessage(hLoginButton, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

            // Create status label (larger and better positioned)
            hStatusLabel = CreateWindowW(L"STATIC", L"",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                30, 210, 440, 30,
                hwnd, (HMENU)IDC_STATUS_LABEL, NULL, NULL);
            SendMessage(hStatusLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

            // Create result label (multiline support)
            hResultLabel = CreateWindowW(L"STATIC", L"",
                WS_VISIBLE | WS_CHILD | SS_CENTER | SS_CENTERIMAGE,
                30, 250, 440, 80,
                hwnd, (HMENU)IDC_RESULT_LABEL, NULL, NULL);
            SendMessage(hResultLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_LOGIN_BUTTON) {
                // Get key from edit control
                wchar_t key[512];
                GetWindowTextW(hKeyEdit, key, 512);

                std::wstring keyStr(key);

                // Validate input
                if (keyStr.empty()) {
                    SetWindowTextW(hStatusLabel, L"Please enter an access key");
                    SetWindowTextW(hResultLabel, L"");
                    return 0;
                }

                // Disable button during validation
                EnableWindow(hLoginButton, FALSE);
                SetWindowTextW(hStatusLabel, L"Validating...");
                SetWindowTextW(hResultLabel, L"");
                UpdateWindow(hwnd);

                // Validate key with server
                std::wstring username, message;
                bool success = ValidateKey(keyStr, username, message);

                // Enable button
                EnableWindow(hLoginButton, TRUE);

                if (success) {
                    SetWindowTextW(hStatusLabel, L"✓ Authentication Successful!");
                    std::wstring resultText = L"Welcome, " + username + L"!\n\nYou have been successfully authenticated.";
                    SetWindowTextW(hResultLabel, resultText.c_str());

                    // You could proceed to open main application here
                    MessageBoxW(hwnd, resultText.c_str(), L"Login Successful", MB_OK | MB_ICONINFORMATION);
                } else {
                    SetWindowTextW(hStatusLabel, L"✗ Authentication Failed");
                    SetWindowTextW(hResultLabel, message.c_str());
                }

                return 0;
            }
            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            HWND hwndStatic = (HWND)lParam;

            if (hwndStatic == hStatusLabel) {
                wchar_t text[256];
                GetWindowTextW(hwndStatic, text, 256);
                std::wstring statusText(text);

                if (statusText.find(L"✓") != std::wstring::npos) {
                    SetTextColor(hdcStatic, RGB(0, 128, 0)); // Green
                } else if (statusText.find(L"✗") != std::wstring::npos) {
                    SetTextColor(hdcStatic, RGB(192, 0, 0)); // Red
                } else {
                    SetTextColor(hdcStatic, RGB(0, 0, 0)); // Black
                }
                SetBkMode(hdcStatic, TRANSPARENT);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }

            SetBkMode(hdcStatic, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        case WM_DESTROY: {
            DeleteObject(hTitleFont);
            DeleteObject(hNormalFont);
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// WinMain entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Create fonts
    hTitleFont = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    hNormalFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Register window class
    const wchar_t CLASS_NAME[] = L"SecureLoginWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    // Create window
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Secure Login System",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
