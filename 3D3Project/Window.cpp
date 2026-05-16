#include "Window.h"

#include <iostream>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool SetupWindow(HINSTANCE instance, UINT width, UINT height, int nCmdShow, HWND& window)
{
    const wchar_t CLASS_NAME[] = L"DX12WindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassW(&wc))
    {
        std::cerr << "Failed to register window class: Error: " << GetLastError() << std::endl;
        return false;
    }

    window = CreateWindowExW(0, CLASS_NAME, L"DirectX12 Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, instance, nullptr);

    if (window == nullptr) 
    {
        std::cerr << "Failed to create window. Error: " << GetLastError() << std::endl;
        return false;
    }

    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    SetClassLongPtr(window, GCLP_HBRBACKGROUND, (LONG_PTR)brush);

    ShowWindow(window, nCmdShow);
    
    return true;
}
