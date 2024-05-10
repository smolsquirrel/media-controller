#pragma once

#ifdef MEDIACONTROLLERHOOK_EXPORTS
#define MEDIACONTROLLERHOOK_API __declspec(dllexport)
#else
#define MEDIACONTROLLERHOOK_API __declspec(dllimport)
#endif

#define WM_LMEDIABUTTON		WM_APP + 1
#define WM_MMEDIABUTTON		WM_APP + 2
#define WM_RMEDIABUTTON		WM_APP + 3
#define WM_MEDIAWHEELCLK	WM_APP + 4
#define WM_MEDIAWHEEL		WM_APP + 5
#define WM_XMEDIABUTTON		WM_APP + 6

extern "C" MEDIACONTROLLERHOOK_API void InstallHook(HWND hWND);

extern "C" MEDIACONTROLLERHOOK_API void UninstallHook();

extern "C" MEDIACONTROLLERHOOK_API void SetMediaDevice(HANDLE hMD);

extern "C" MEDIACONTROLLERHOOK_API void SetCurrentDevice(HANDLE hCur);