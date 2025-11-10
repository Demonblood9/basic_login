#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <sstream>
#include <commctrl.h>
#include <windowsx.h>
#include <gdiplus.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "Msimg32.lib")

// Control IDs
#define IDC_KEY_EDIT        1001
#define IDC_LOGIN_BUTTON    1002
#define IDC_STATUS_LABEL    1003
#define IDC_RESULT_LABEL    1004
#define IDC_CLEAR_BUTTON    1005

// Modern color scheme (Windows 11 inspired)
#define COLOR_BACKGROUND RGB(243, 243, 243)
#define COLOR_PANEL RGB(255, 255, 255)
#define COLOR_ACCENT RGB(0, 120, 212)
#define COLOR_ACCENT_HOVER RGB(0, 103, 192)
#define COLOR_SUCCESS RGB(16, 124, 16)
#define COLOR_ERROR RGB(196, 43, 28)
#define COLOR_WARNING RGB(244, 147, 0)
#define COLOR_TEXT_PRIMARY RGB(32, 32, 32)
#define COLOR_TEXT_SECONDARY RGB(96, 96, 96)
#define COLOR_BORDER RGB(229, 229, 229)

// Global variables
HWND hKeyEdit;
HWND hLoginButton;
HWND hClearButton;
HWND hStatusLabel;
HWND hResultLabel;
HWND hKeyLabel;
HFONT hTitleFont;
HFONT hNormalFont;
HFONT hLabelFont;
HFONT hButtonFont;
bool isButtonHovered = false;
bool isClearButtonHovered = false;
ULONG_PTR gdiplusToken;

// Server configuration
const wchar_t* SERVER_HOST = L"localhost";
const int SERVER_PORT = 5000;
const wchar_t* VALIDATE_PATH = L"/api/validate";

// Helper function to create rounded rectangle path
void CreateRoundRectPath(Gdiplus::GraphicsPath* path, Gdiplus::Rect rect, int radius) {
    path->AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180.0f, 90.0f);
    path->AddArc(rect.X + rect.Width - radius * 2, rect.Y, radius * 2, radius * 2, 270.0f, 90.0f);
    path->AddArc(rect.X + rect.Width - radius * 2, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 0.0f, 90.0f);
    path->AddArc(rect.X, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 90.0f, 90.0f);
    path->CloseFigure();
}

// JSON parsing helpers
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

    std::wstring headers;
    std::string responseBody;
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;

    std::wstring jsonKey = key;
    std::wstring jsonBody = L"{\"key\":\"" + jsonKey + L"\"}";
    std::string jsonBodyUtf8 = WStringToString(jsonBody);

    hSession = WinHttpOpen(L"SecureLoginClient/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS,
                           0);

    if (!hSession) {
        message = L"Failed to initialize HTTP client";
        goto cleanup;
    }

    hConnect = WinHttpConnect(hSession, SERVER_HOST, SERVER_PORT, 0);
    if (!hConnect) {
        message = L"Failed to connect to server";
        goto cleanup;
    }

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

    headers = L"Content-Type: application/json\r\n";
    WinHttpAddRequestHeaders(hRequest,
                            headers.c_str(),
                            (DWORD)headers.length(),
                            WINHTTP_ADDREQ_FLAG_ADD);

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

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        message = L"Failed to receive response";
        goto cleanup;
    }

    WinHttpQueryHeaders(hRequest,
                       WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                       NULL,
                       &statusCode,
                       &statusCodeSize,
                       NULL);

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

// Custom button drawing procedure
void DrawModernButton(HDC hdc, RECT rect, const wchar_t* text, bool isHovered, bool isPressed, COLORREF baseColor) {
    Gdiplus::Graphics graphics(hdc);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    // Calculate color based on state
    int r = GetRValue(baseColor);
    int g = GetGValue(baseColor);
    int b = GetBValue(baseColor);

    if (isPressed) {
        r = max(0, r - 30);
        g = max(0, g - 30);
        b = max(0, b - 30);
    } else if (isHovered) {
        r = max(0, r - 15);
        g = max(0, g - 15);
        b = max(0, b - 15);
    }

    Gdiplus::Color buttonColor(255, (BYTE)r, (BYTE)g, (BYTE)b);
    Gdiplus::SolidBrush brush(buttonColor);

    // Draw rounded rectangle button
    Gdiplus::GraphicsPath path;
    Gdiplus::Rect buttonRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    CreateRoundRectPath(&path, buttonRect, 6);

    graphics.FillPath(&brush, &path);

    // Draw subtle shadow/border
    Gdiplus::Pen borderPen(Gdiplus::Color(40, 0, 0, 0), 1);
    graphics.DrawPath(&borderPen, &path);

    // Draw text
    Gdiplus::FontFamily fontFamily(L"Segoe UI");
    Gdiplus::Font font(&fontFamily, 14, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat stringFormat;
    stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
    stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);

    Gdiplus::RectF layoutRect((Gdiplus::REAL)rect.left, (Gdiplus::REAL)rect.top, (Gdiplus::REAL)(rect.right - rect.left), (Gdiplus::REAL)(rect.bottom - rect.top));
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));
    graphics.DrawString(text, -1, &font, layoutRect, &stringFormat, &textBrush);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Create title label
            HWND hTitle = CreateWindowW(L"STATIC", L"Secure Authentication",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                0, 40, 600, 50,
                hwnd, NULL, NULL, NULL);
            SendMessage(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

            // Subtitle
            HWND hSubtitle = CreateWindowW(L"STATIC", L"Enter your access key to continue",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                0, 95, 600, 25,
                hwnd, NULL, NULL, NULL);
            SendMessage(hSubtitle, WM_SETFONT, (WPARAM)hLabelFont, TRUE);

            // Create key input
            hKeyEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_VISIBLE | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL,
                80, 150, 440, 40,
                hwnd, (HMENU)IDC_KEY_EDIT, NULL, NULL);
            SendMessage(hKeyEdit, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
            SendMessage(hKeyEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"Paste your access key here...");

            // Create login button (owner-drawn)
            hLoginButton = CreateWindowW(L"BUTTON", L"Login",
                WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                200, 215, 120, 45,
                hwnd, (HMENU)IDC_LOGIN_BUTTON, NULL, NULL);

            // Create clear button
            hClearButton = CreateWindowW(L"BUTTON", L"Clear",
                WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                330, 215, 80, 45,
                hwnd, (HMENU)IDC_CLEAR_BUTTON, NULL, NULL);

            // Create status label
            hStatusLabel = CreateWindowW(L"STATIC", L"",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                50, 290, 500, 35,
                hwnd, (HMENU)IDC_STATUS_LABEL, NULL, NULL);
            SendMessage(hStatusLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

            // Create result label
            hResultLabel = CreateWindowW(L"STATIC", L"",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                50, 335, 500, 100,
                hwnd, (HMENU)IDC_RESULT_LABEL, NULL, NULL);
            SendMessage(hResultLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            Gdiplus::Graphics graphics(hdc);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

            // Get client rect
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            // Draw background gradient
            Gdiplus::LinearGradientBrush bgBrush(
                Gdiplus::Point(0, 0),
                Gdiplus::Point(0, clientRect.bottom),
                Gdiplus::Color(255, 243, 243, 243),
                Gdiplus::Color(255, 250, 250, 250)
            );
            graphics.FillRectangle(&bgBrush, 0, 0, clientRect.right, clientRect.bottom);

            // Draw main card panel with shadow
            Gdiplus::Rect cardRect(40, 125, 520, 330);

            // Shadow
            Gdiplus::GraphicsPath shadowPath;
            Gdiplus::Rect shadowRect(cardRect.X + 3, cardRect.Y + 3, cardRect.Width, cardRect.Height);
            CreateRoundRectPath(&shadowPath, shadowRect, 12);
            Gdiplus::PathGradientBrush shadowBrush(&shadowPath);
            Gdiplus::Color centerColor(80, 0, 0, 0);
            Gdiplus::Color edgeColor(0, 0, 0, 0);
            shadowBrush.SetCenterColor(centerColor);
            int count = 1;
            shadowBrush.SetSurroundColors(&edgeColor, &count);
            graphics.FillPath(&shadowBrush, &shadowPath);

            // Card
            Gdiplus::GraphicsPath cardPath;
            CreateRoundRectPath(&cardPath, cardRect, 12);
            Gdiplus::SolidBrush cardBrush(Gdiplus::Color(255, 255, 255, 255));
            graphics.FillPath(&cardBrush, &cardPath);

            // Card border
            Gdiplus::Pen cardBorder(Gdiplus::Color(255, (BYTE)GetRValue(COLOR_BORDER), (BYTE)GetGValue(COLOR_BORDER), (BYTE)GetBValue(COLOR_BORDER)), 1);
            graphics.DrawPath(&cardBorder, &cardPath);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;

            if (pDIS->CtlID == IDC_LOGIN_BUTTON) {
                bool isPressed = (pDIS->itemState & ODS_SELECTED);
                DrawModernButton(pDIS->hDC, pDIS->rcItem, L"Login", isButtonHovered, isPressed, COLOR_ACCENT);
                return TRUE;
            }
            else if (pDIS->CtlID == IDC_CLEAR_BUTTON) {
                bool isPressed = (pDIS->itemState & ODS_SELECTED);
                DrawModernButton(pDIS->hDC, pDIS->rcItem, L"Clear", isClearButtonHovered, isPressed, RGB(108, 117, 125));
                return TRUE;
            }
            break;
        }

        case WM_MOUSEMOVE: {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            RECT buttonRect;
            GetWindowRect(hLoginButton, &buttonRect);
            POINT screenPt = pt;
            ClientToScreen(hwnd, &screenPt);
            bool wasHovered = isButtonHovered;
            isButtonHovered = PtInRect(&buttonRect, screenPt);
            if (wasHovered != isButtonHovered) {
                InvalidateRect(hLoginButton, NULL, FALSE);
            }

            GetWindowRect(hClearButton, &buttonRect);
            bool wasClearHovered = isClearButtonHovered;
            isClearButtonHovered = PtInRect(&buttonRect, screenPt);
            if (wasClearHovered != isClearButtonHovered) {
                InvalidateRect(hClearButton, NULL, FALSE);
            }

            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);

            break;
        }

        case WM_MOUSELEAVE: {
            if (isButtonHovered) {
                isButtonHovered = false;
                InvalidateRect(hLoginButton, NULL, FALSE);
            }
            if (isClearButtonHovered) {
                isClearButtonHovered = false;
                InvalidateRect(hClearButton, NULL, FALSE);
            }
            break;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_CLEAR_BUTTON) {
                SetWindowTextW(hKeyEdit, L"");
                SetWindowTextW(hStatusLabel, L"");
                SetWindowTextW(hResultLabel, L"");
                SetFocus(hKeyEdit);
                return 0;
            }

            if (LOWORD(wParam) == IDC_LOGIN_BUTTON) {
                wchar_t key[512];
                GetWindowTextW(hKeyEdit, key, 512);
                std::wstring keyStr(key);

                if (keyStr.empty()) {
                    SetWindowTextW(hStatusLabel, L"");
                    SetWindowTextW(hResultLabel, L"");
                    InvalidateRect(hStatusLabel, NULL, TRUE);
                    InvalidateRect(hResultLabel, NULL, TRUE);
                    UpdateWindow(hStatusLabel);
                    UpdateWindow(hResultLabel);

                    SetWindowTextW(hStatusLabel, L"⚠ Please enter an access key");
                    InvalidateRect(hStatusLabel, NULL, TRUE);
                    UpdateWindow(hStatusLabel);
                    return 0;
                }

                SetWindowTextW(hStatusLabel, L"");
                SetWindowTextW(hResultLabel, L"");
                InvalidateRect(hStatusLabel, NULL, TRUE);
                InvalidateRect(hResultLabel, NULL, TRUE);
                UpdateWindow(hStatusLabel);
                UpdateWindow(hResultLabel);

                EnableWindow(hLoginButton, FALSE);
                SetWindowTextW(hStatusLabel, L"⌛ Validating your key...");
                InvalidateRect(hStatusLabel, NULL, TRUE);
                UpdateWindow(hStatusLabel);

                std::wstring username, message;
                bool success = ValidateKey(keyStr, username, message);

                SetWindowTextW(hStatusLabel, L"");
                SetWindowTextW(hResultLabel, L"");
                InvalidateRect(hStatusLabel, NULL, TRUE);
                InvalidateRect(hResultLabel, NULL, TRUE);
                UpdateWindow(hStatusLabel);
                UpdateWindow(hResultLabel);

                EnableWindow(hLoginButton, TRUE);

                if (success) {
                    SetWindowTextW(hStatusLabel, L"✓ Authentication Successful!");
                    std::wstring resultText = L"Welcome back, " + username + L"!\n\nAccess granted. You are now authenticated.";
                    SetWindowTextW(hResultLabel, resultText.c_str());
                    InvalidateRect(hStatusLabel, NULL, TRUE);
                    InvalidateRect(hResultLabel, NULL, TRUE);
                    UpdateWindow(hStatusLabel);
                    UpdateWindow(hResultLabel);

                    MessageBoxW(hwnd, resultText.c_str(), L"Login Successful", MB_OK | MB_ICONINFORMATION);
                } else {
                    SetWindowTextW(hStatusLabel, L"✗ Authentication Failed");
                    SetWindowTextW(hResultLabel, message.c_str());
                    InvalidateRect(hStatusLabel, NULL, TRUE);
                    InvalidateRect(hResultLabel, NULL, TRUE);
                    UpdateWindow(hStatusLabel);
                    UpdateWindow(hResultLabel);
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
                    SetTextColor(hdcStatic, COLOR_SUCCESS);
                } else if (statusText.find(L"✗") != std::wstring::npos) {
                    SetTextColor(hdcStatic, COLOR_ERROR);
                } else if (statusText.find(L"⚠") != std::wstring::npos) {
                    SetTextColor(hdcStatic, COLOR_WARNING);
                } else if (statusText.find(L"⌛") != std::wstring::npos) {
                    SetTextColor(hdcStatic, COLOR_ACCENT);
                } else {
                    SetTextColor(hdcStatic, COLOR_TEXT_SECONDARY);
                }
                SetBkMode(hdcStatic, TRANSPARENT);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }

            if (hwndStatic == hResultLabel) {
                SetTextColor(hdcStatic, COLOR_TEXT_SECONDARY);
                SetBkMode(hdcStatic, TRANSPARENT);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }

            SetTextColor(hdcStatic, COLOR_TEXT_PRIMARY);
            SetBkMode(hdcStatic, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetBkColor(hdcEdit, RGB(255, 255, 255));
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }

        case WM_DESTROY: {
            DeleteObject(hTitleFont);
            DeleteObject(hNormalFont);
            DeleteObject(hLabelFont);
            DeleteObject(hButtonFont);
            Gdiplus::GdiplusShutdown(gdiplusToken);
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// WinMain entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Create fonts
    hTitleFont = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    hNormalFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    hLabelFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    hButtonFont = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Register window class
    const wchar_t CLASS_NAME[] = L"ModernSecureLoginWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassW(&wc);

    // Create window with exact client size
    RECT windowRect = {0, 0, 600, 500};
    AdjustWindowRect(&windowRect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Secure Login",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Center window on screen
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    int xPos = (GetSystemMetrics(SM_CXSCREEN) - (rcWindow.right - rcWindow.left)) / 2;
    int yPos = (GetSystemMetrics(SM_CYSCREEN) - (rcWindow.bottom - rcWindow.top)) / 2;
    SetWindowPos(hwnd, NULL, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

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
