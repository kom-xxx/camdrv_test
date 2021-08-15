#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdexcept>

#include <Windows.h>

#include "debug.h"

void
debug_cons(void)
{
#ifdef _DEBUG
	errno_t err;
	char errmsg[128];
	FILE *fp;

	
	if (!AllocConsole()) {
		snprintf(errmsg, sizeof errmsg,
			 "AllocConsole:%d", GetLastError());
		throw std::runtime_error(errmsg);
	}
	err = freopen_s(&fp, "CONIN$", "r", stdin);
	if (err) {
		snprintf(errmsg, sizeof errmsg, "freopen_s(stdin):%d", err);
		throw std::runtime_error(errmsg);
	}
	err = freopen_s(&fp, "CONOUT$", "w", stdout);
	if (err) {
		snprintf(errmsg, sizeof errmsg, "freopen_s(stdout):%d", err);
		throw std::runtime_error(errmsg);
	}
	err = freopen_s(&fp, "CONOUT$", "w", stderr);
	if (err) {
		snprintf(errmsg, sizeof errmsg, "freopen_s(stderr):%d", err);
		throw std::runtime_error(errmsg);
	}
#endif
}

LRESULT
win_proc(HWND win, int32_t msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(win, msg, wparam, lparam);

}

HWND
create_window(size_t width, size_t height, const wchar_t **class_name)
{
	HINSTANCE hInst = GetModuleHandle(nullptr);

	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)win_proc;
	w.lpszClassName = L"CamraDriverTest";
	w.hInstance = GetModuleHandle(nullptr);
	RegisterClassEx(&w);

	RECT wrc = { 0,0, (int32_t)width, (int32_t)height};
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	HWND win = CreateWindow(w.lpszClassName,
				L"DX12_test",
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				(int32_t)width,
				(int32_t)height,
				nullptr,
				nullptr,
				w.hInstance,
				nullptr);
	if (!win) {
		uint32_t err = GetLastError();
		DPRINTF("CreateWindow:%x", err);
		throw std::runtime_error("CreateWindow");
	}

	*class_name = L"CamraDriverTest";
	return win;
}

