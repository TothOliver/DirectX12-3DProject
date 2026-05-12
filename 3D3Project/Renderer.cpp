#include "Renderer.hpp"

#include <string>
#include <wrl/client.h>
#include <d3d12.h>

#pragma comment(lib, "d3d12.lib") //links d3d12.lib

bool Renderer::Initialize(HWND window, UINT width, UINT height)
{
    m_window = window;
    m_width = width;
    m_height = height;

    OutputDebugStringW(L"Renderer initialized.\n");
    return true;
}

void Renderer::Render()
{
    static UINT frameCounter = 0;
    frameCounter++;

    if (frameCounter % 120 == 0)
    {
        OutputDebugStringW(L"Renderer::Render() is running.\n");
    }
}

void Renderer::Shutdown()
{
    OutputDebugStringW(L"Renderer shutdown.\n");
}

#if defined(_DEBUG)
void Renderer::EnableDebugLayer()
{
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;

    HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));

    if (SUCCEEDED(result))
    {
        debugController->EnableDebugLayer();
        OutputDebugStringW(L"D3D12 debug layer enabled.\n");
    }
    else
    {
        OutputDebugStringW(L"D3D12 debug layer not available.\n");
    }

}
#endif