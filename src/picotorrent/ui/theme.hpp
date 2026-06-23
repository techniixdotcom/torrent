#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/colour.h>
#include <wx/font.h>

namespace nx::Theme
{
    inline wxColour Green()      { return wxColour(0x0a, 0xff, 0x9e); }
    inline wxColour Pink()       { return wxColour(0xff, 0x0a, 0x6c); }
    inline wxColour Cyan()       { return wxColour(0x00, 0xd4, 0xff); }
    inline wxColour Red()        { return wxColour(0xff, 0x33, 0x33); }
    inline wxColour Background() { return wxColour(0x08, 0x0b, 0x0f); }
    inline wxColour Panel()      { return wxColour(0x0d, 0x11, 0x17); }
    inline wxColour PanelHover() { return wxColour(0x11, 0x18, 0x20); }
    inline wxColour Border()     { return wxColour(0x1a, 0x25, 0x30); }
    inline wxColour Dim()        { return wxColour(0x3a, 0x4a, 0x5a); }
    inline wxColour Text()       { return wxColour(0xe8, 0xf0, 0xf8); }

    inline wxColour Accent()     { return Green(); }

    inline wxFont MonoFont(int pointSize = 9)
    {
        return wxFont(
            wxFontInfo(pointSize)
                .Family(wxFONTFAMILY_TELETYPE)
                .FaceName("JetBrains Mono"));
    }

    inline void Apply(wxWindow* window)
    {
        if (window == nullptr) { return; }
        window->SetBackgroundColour(Panel());
        window->SetForegroundColour(Text());
        for (wxWindow* child : window->GetChildren())
        {
            Apply(child);
        }
    }
}
