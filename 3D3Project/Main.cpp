#include <iostream>

#include "Window.h"
#include "Renderer.hpp"

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
    Renderer renderer;
    renderer.Initialize(window, WIDTH, HEIGHT);

    bool running = true;
    while (running) 
    {
        renderer.Render();

        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (!running) { break; }
    }
    renderer.Shutdown();


    return static_cast<int>(message.wParam);
}