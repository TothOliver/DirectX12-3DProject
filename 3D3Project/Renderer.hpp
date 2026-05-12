#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class Renderer 
{
public:
	bool Initialize(HWND window, UINT width, UINT height);
	void Render();
	void Shutdown();

private:
#if defined(_DEBUG)
	void EnableDebugLayer();
#endif

private:
	HWND m_window = nullptr;
	UINT m_width = 0;
	UINT m_height = 0;
};