#include <iostream>

#include "Window.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    const UINT WIDTH = 1024;
    const UINT HEIGHT = 576;
    HWND window = nullptr;
    MSG message = {};

    if (!SetupWindow(hInstance, WIDTH, HEIGHT, nCmdShow, window))
    {
        std::cerr << "Error: SetupWindow" << std::endl;
        return -1;
    }


    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    std::cout << "Git commit?" << std::endl;
    return 0;
}