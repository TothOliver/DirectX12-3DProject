#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

bool SetupWindow(HINSTANCE instance, UINT width, UINT height, int nCmdShow, HWND& window);