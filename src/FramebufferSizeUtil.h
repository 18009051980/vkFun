#pragma once

#include <windows.h>
#include <array>
class FramebufferSizeUtil
{
public:
    FramebufferSizeUtil()=default;
    ~FramebufferSizeUtil()=default;    
    std::array<uint32_t, 2> get(void * nwh)
    {
        HWND hwnd = (HWND)nwh;
        RECT rect;
        GetClientRect(hwnd, &rect);
        uint32_t framebufferWidth = static_cast<uint32_t>(rect.right);
        uint32_t framebufferHeight = static_cast<uint32_t>(rect.bottom);
        return {framebufferWidth, framebufferHeight};
    }
private:

}; // class FramebufferSizeUtil

