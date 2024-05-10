#include <iostream>
#include <fstream>
#include <windows.h>
#include <vector>
#include <string>
#include "MediaControllerHook.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>

IAudioEndpointVolume *audioEndpoint;

IAudioEndpointVolume *getMicAudioEndpoint()
{
    IMMDeviceEnumerator *pEnumerator = NULL;
    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
    HRESULT hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator, NULL,
        CLSCTX_ALL, IID_IMMDeviceEnumerator,
        (void **)&pEnumerator);

    IMMDevice *endpoint;
    pEnumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &endpoint);
    const IID IID_IAudioEndpointVolume = __uuidof(IAudioEndpointVolume);
    IAudioEndpointVolume *audioEndpoint = NULL;
    endpoint->Activate(IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void **)&audioEndpoint);
    return audioEndpoint;
}

HRESULT toggleMuteMic(IAudioEndpointVolume *audioEndpoint)
{
    BOOL bMute;
    audioEndpoint->GetMute(&bMute);
    return audioEndpoint->SetMute(!bMute, NULL);
}

void SimulateKey(WORD wVK)
{
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    // Key down
    ip.ki.wVk = wVK;
    ip.ki.dwFlags = 0;
    SendInput(1, &ip, sizeof(INPUT));

    // Key up
    ip.ki.wVk = wVK;
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        UninstallHook();
        PostQuitMessage(0);
        return 0;

    case WM_INPUT:
    {
        UINT dwSize;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        LPBYTE lpb = new BYTE[dwSize];
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

        RAWINPUT *raw = (RAWINPUT *)lpb;
        HANDLE deviceHandle = raw->header.hDevice;

        if (raw->header.dwType == RIM_TYPEMOUSE)
        {
            USHORT buttonFlag = raw->data.mouse.usButtonFlags;
            if (!buttonFlag == 0)
            {
                SetCurrentDevice(deviceHandle);
            }
        }
        return 0;
    }
    case WM_LMEDIABUTTON:
        SimulateKey(VK_MEDIA_PREV_TRACK);
        return 0;
    case WM_MMEDIABUTTON:
        SimulateKey(VK_MEDIA_PLAY_PAUSE);
        return 0;
    case WM_RMEDIABUTTON:
        SimulateKey(VK_MEDIA_NEXT_TRACK);
        return 0;
    case WM_MEDIAWHEEL:
        if ((INT)wParam < 0)
            SimulateKey(VK_VOLUME_DOWN);
        else
            SimulateKey(VK_VOLUME_UP);
        return 0;
    case WM_XMEDIABUTTON:
        if (wParam == 1)
            SimulateKey(VK_VOLUME_MUTE);
        else
        {
            toggleMuteMic(audioEndpoint);
        }

        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int readConfig(std::wstring &s)
{
    std::wifstream file;
    file.open("device.cfg");
    if (file)
    {
        std::getline(file, s);
        file.close();
        return 0;
    }
    return -1;
}

HANDLE getDeviceHandleByName(std::wstring deviceName, std::vector<RAWINPUTDEVICELIST> devices)
{
    std::vector<wchar_t> deviceNameData;
    for (int i = 0; i < devices.size(); i++)
    {
        UINT dataSize;
        GetRawInputDeviceInfo(devices[i].hDevice, RIDI_DEVICENAME, nullptr, &dataSize);
        if (dataSize)
        {
            deviceNameData.resize(dataSize);
            UINT result = GetRawInputDeviceInfo(devices[i].hDevice, RIDI_DEVICENAME, &deviceNameData[0], &dataSize);
            if (result > 0)
            {
                wchar_t *name = deviceNameData.data();
                std::wstring string(name);
                if (wcscmp(deviceName.c_str(), name) == 0)
                    return devices[i].hDevice;
            }
        }
    }
    return NULL;
}

std::vector<RAWINPUTDEVICELIST> getMouseDevices()
{
    UINT numDevices;
    GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST)); // get number of devices
    std::vector<RAWINPUTDEVICELIST> devices(numDevices);
    GetRawInputDeviceList(&devices[0], &numDevices, sizeof(RAWINPUTDEVICELIST)); // populate with all devices
    auto onlyMice = std::remove_if(devices.begin(), devices.end(), [](RAWINPUTDEVICELIST &x)
                                   { return x.dwType != RIM_TYPEMOUSE; }); // remove non-mouse devices
    devices.erase(onlyMice, devices.end());
    return devices;
}

int listDevices()
{
    std::wstring s;
    int x = readConfig(s);
    std::vector<RAWINPUTDEVICELIST> devices = getMouseDevices();

    for (const auto i : devices)
    {
        std::vector<wchar_t> deviceNameData;
        UINT dataSize;
        GetRawInputDeviceInfo(i.hDevice, RIDI_DEVICENAME, nullptr, &dataSize);
        if (dataSize)
        {
            deviceNameData.resize(dataSize);
            UINT result = GetRawInputDeviceInfo(i.hDevice, RIDI_DEVICENAME, &deviceNameData[0], &dataSize);
        }
    }
    return 0;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    CoInitialize(NULL);

    HANDLE hMediaDevice;
    audioEndpoint = getMicAudioEndpoint();

    // Debugging
    // FILE *console_output;
    // AllocConsole();
    // freopen_s(&console_output, "CONOUT$", "w", stdout);

    HINSTANCE instance = GetModuleHandle(0);
    const wchar_t *class_name = L"MMController Class";

    WNDCLASS window_class = {};
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = instance;
    window_class.lpszClassName = class_name;
    RegisterClass(&window_class);
    HWND hWnd = CreateWindow(class_name, L"MMController", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
    ShowWindow(hWnd, 0);

    RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x02;
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = hWnd;
    RegisterRawInputDevices(rid, 1, sizeof(rid[0]));

    std::wstring s;
    int x = readConfig(s);
    std::vector<RAWINPUTDEVICELIST> devices = getMouseDevices();
    hMediaDevice = getDeviceHandleByName(s, devices);

    InstallHook(hWnd);
    SetMediaDevice(hMediaDevice);

    MSG msg;
    while (GetMessage(&msg, hWnd, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    UninstallHook();
    CoUninitialize();

    return 0;
}
