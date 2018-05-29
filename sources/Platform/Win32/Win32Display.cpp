/*
 * Win32Display.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2018 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "Win32Display.h"
#include "../../Core/Helper.h"
#include <locale>
#include <codecvt>


namespace LLGL
{


// Thread local reference to the output list of the Display::QueryList function
thread_local static std::vector<std::unique_ptr<Display>>* g_displayListRef;

static BOOL CALLBACK Win32MonitorEnumProc(HMONITOR monitor, HDC hDC, LPRECT rect, LPARAM data)
{
    if (g_displayListRef)
    {
        g_displayListRef->push_back(MakeUnique<Win32Display>(monitor));
        return TRUE;
    }
    return FALSE;
}

std::vector<std::unique_ptr<Display>> Display::QueryList()
{
    std::vector<std::unique_ptr<Display>> displayList;

    g_displayListRef = (&displayList);
    {
        EnumDisplayMonitors(nullptr, nullptr, Win32MonitorEnumProc, 0);
    }
    g_displayListRef = nullptr;

    return displayList;
}

Win32Display::Win32Display(HMONITOR monitor) :
    monitor_ { monitor }
{
}

bool Win32Display::IsPrimary() const
{
    MONITORINFO info;
    GetInfo(info);
    return ((info.dwFlags & MONITORINFOF_PRIMARY) != 0);
}

std::wstring Win32Display::GetDeviceName() const
{
    MONITORINFOEX infoEx;
    GetInfo(infoEx);
    #ifdef UNICODE
    return std::wstring(infoEx.szDevice);
    #else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(infoEx.szDevice);
    #endif
}

Offset2D Win32Display::GetOffset() const
{
    MONITORINFO info;
    GetInfo(info);
    return
    {
        static_cast<std::int32_t>(info.rcMonitor.left),
        static_cast<std::int32_t>(info.rcMonitor.top),
    };
}

bool Win32Display::SetDisplayMode(const DisplayModeDescriptor& displayModeDesc)
{
    //TODO
    return false;
}

DisplayModeDescriptor Win32Display::GetDisplayMode() const
{
    MONITORINFO info;
    GetInfo(info);
    DisplayModeDescriptor displayModeDesc;
    {
        displayModeDesc.resolution.width    = static_cast<std::uint32_t>(info.rcMonitor.right - info.rcMonitor.left);
        displayModeDesc.resolution.height   = static_cast<std::uint32_t>(info.rcMonitor.bottom - info.rcMonitor.top);
        displayModeDesc.refreshRate         = 60;
    }
    return displayModeDesc;
}

std::vector<DisplayModeDescriptor> Win32Display::QuerySupportedDisplayModes() const
{
    std::vector<DisplayModeDescriptor> displayModeDescs;

    /* Get display device name */
    MONITORINFOEX infoEx;
    GetInfo(infoEx);

    /* Enumerate all display settings for this display */
    const DWORD fieldBits = (DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY);

    DEVMODE devMode;
    for (DWORD modeNum = 0; EnumDisplaySettings(infoEx.szDevice, modeNum, &devMode) != FALSE; ++modeNum)
    {
        /* Only enumerate display settings where the width, height, and frequency fields have been initialized */
        if ((devMode.dmFields & fieldBits) == fieldBits)
        {
            DisplayModeDescriptor outputDesc;
            {
                outputDesc.resolution.width   = static_cast<std::uint32_t>(devMode.dmPelsWidth);
                outputDesc.resolution.height  = static_cast<std::uint32_t>(devMode.dmPelsHeight);
                outputDesc.refreshRate        = static_cast<std::uint32_t>(devMode.dmDisplayFrequency);
            }
            displayModeDescs.push_back(outputDesc);
        }
    }

    /* Sort final display mode list and remove duplciate entries */
    FinalizeDisplayModes(displayModeDescs);

    return displayModeDescs;
}


/*
 * ======= Private: =======
 */

void Win32Display::GetInfo(MONITORINFO& info) const
{
    info.cbSize = sizeof(info);
    GetMonitorInfo(monitor_, &info);
}

void Win32Display::GetInfo(MONITORINFOEX& info) const
{
    info.cbSize = sizeof(info);
    GetMonitorInfo(monitor_, &info);
}


} // /namespace LLGL



// ================================================================================
