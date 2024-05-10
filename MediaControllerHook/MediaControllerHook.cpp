#include "pch.h"
#include <windows.h>
#include <iostream>

#include "MediaControllerHook.h"
#include <stdio.h>

#pragma data_seg("SHARED")
static HHOOK hookHandle = NULL;
static HANDLE hMediaDevice = NULL;
static HANDLE hCurrentDevice = NULL;
static HWND hMainWnd = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:SHARED,RWS")

static HINSTANCE hInstance;



BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			hInstance = (HINSTANCE)hModule;
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode < 0) {
		return CallNextHookEx(hookHandle, nCode, wParam, lParam);
	}

	if (nCode == HC_NOREMOVE && hCurrentDevice == hMediaDevice) {
		UINT Button = WM_APP;
		WPARAM Param = NULL;
		switch (wParam) {
			case WM_LBUTTONDOWN:
				Button = WM_RMEDIABUTTON;
				break;
			case WM_MBUTTONDOWN:
				Button = WM_MMEDIABUTTON;
				break;
			case WM_RBUTTONDOWN:
				Button = WM_LMEDIABUTTON;
				break;
			case WM_MOUSEWHEEL:
				Button = WM_MEDIAWHEEL;
				Param = GET_WHEEL_DELTA_WPARAM(((MOUSEHOOKSTRUCTEX*)lParam)->mouseData);
				break;
			case WM_XBUTTONDOWN:
				Button = WM_XMEDIABUTTON;
				Param = GET_XBUTTON_WPARAM(((MOUSEHOOKSTRUCTEX*)lParam)->mouseData);
				break;
		}
		SendMessage(hMainWnd, Button, Param, NULL);
		hCurrentDevice = NULL;
		return 1;
	}

	return CallNextHookEx(hookHandle, nCode, wParam, lParam);
}

void InstallHook(HWND hWND) {
	AttachConsole(0);
	hookHandle = SetWindowsHookEx(WH_MOUSE, (HOOKPROC)MouseProc, hInstance, 0);
	hMainWnd = hWND;
}

void UninstallHook() {
	UnhookWindowsHookEx(hookHandle);
	hookHandle = NULL;
}

void SetMediaDevice(HANDLE hMD) {
	hMediaDevice = hMD;
}

void SetCurrentDevice(HANDLE hCur) {
	hCurrentDevice = hCur;
}