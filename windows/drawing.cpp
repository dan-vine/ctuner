////////////////////////////////////////////////////////////////////////////////
//
//  drawing.cpp - Drawing functions for Tuner UI components
//
//  Copyright (C) 2009  Bill Farmer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
///////////////////////////////////////////////////////////////////////////////

#include "Tuner.h"

// Draw item - dispatcher for WM_DRAWITEM messages
BOOL DrawItem(WPARAM wParam, LPARAM lParam)
{
    LPDRAWITEMSTRUCT lpdi = (LPDRAWITEMSTRUCT)lParam;
    RECT rect = lpdi->rcItem;
    HDC hdc = lpdi->hDC;

    SetGraphicsMode(hdc, GM_ADVANCED);

    switch (wParam)
    {
        // Scope
    case SCOPE_ID:
        return DrawScope(hdc, rect);
        break;

        // Spectrum
    case SPECTRUM_ID:
        return DrawSpectrum(hdc, rect);
        break;

        // Strobe
    case STROBE_ID:
        return DrawStrobe(hdc, rect);
        break;

        // Staff
    case STAFF_ID:
        return DrawStaff(hdc, rect);
        break;

        // Display
    case DISPLAY_ID:
        return DrawDisplay(hdc, rect);
        break;

        // Meter
    case METER_ID:
        return DrawMeter(hdc, rect);
        break;
    }

    return false;
}

// Draw scope
BOOL DrawScope(HDC hdc, RECT rect)
{
    using Gdiplus::SmoothingModeAntiAlias;
    using Gdiplus::Graphics;
    using Gdiplus::PointF;
    using Gdiplus::Color;
    using Gdiplus::Pen;

    static HBITMAP bitmap;
    static HFONT font;
    static SIZE size;
    static HDC hbdc;

    enum
    {FONT_HEIGHT   = 10};

    // Bold font
    static LOGFONT lf =
        {0, 0, 0, 0,
         FW_BOLD,
         false, false, false,
         DEFAULT_CHARSET,
         OUT_DEFAULT_PRECIS,
         CLIP_DEFAULT_PRECIS,
         DEFAULT_QUALITY,
         DEFAULT_PITCH | FF_DONTCARE,
         ""};

    // Create font
    if (font == NULL)
    {
        lf.lfHeight = FONT_HEIGHT;
        font = CreateFontIndirect(&lf);
    }

    // Draw nice etched edge
    DrawEdge(hdc, &rect , EDGE_SUNKEN, BF_ADJUST | BF_RECT);

    // Calculate bitmap dimensions
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Create DC
    if (hbdc == NULL)
    {
        hbdc = CreateCompatibleDC(hdc);
        SelectObject(hbdc, GetStockObject(DC_PEN));

        // Select font
        SelectObject(hbdc, font);
        SetTextAlign(hbdc, TA_LEFT | TA_BOTTOM);
        SetBkMode(hbdc, TRANSPARENT);
    }

    // Create new bitmaps if resized
    if (width != size.cx || height != size.cy)
    {
        // Delete old bitmap
        if (bitmap != NULL)
            DeleteObject(bitmap);

        // Create new bitmap
        bitmap = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(hbdc, bitmap);

        size.cx = width;
        size.cy = height;
    }

    // Erase background
    // SetViewportOrgEx(hbdc, 0, 0, NULL);
    RECT brct = {0, 0, width, height};
    FillRect(hbdc, &brct, (HBRUSH)GetStockObject(BLACK_BRUSH));

    // Dark green graticule
    SetDCPenColor(hbdc, RGB(0, 64, 0));

    // Draw graticule
    for (int i = 4; i < width; i += 5)
    {
        MoveToEx(hbdc, i, 0, NULL);
        LineTo(hbdc, i, height);
    }

    for (int i = 4; i < height; i += 5)
    {
        MoveToEx(hbdc, 0, i, NULL);
        LineTo(hbdc, width, i);
    }

    // Don't attempt the trace until there's a buffer
    if (scope.data == NULL)
    {
        // Copy the bitmap
        BitBlt(hdc, rect.left, rect.top, width, height,
               hbdc, 0, 0, SRCCOPY);

        return true;
    }

    // Initialise sync
    int maxdx = 0;
    int dx = 0;
    int n = 0;

    for (int i = 1; i < width; i++)
    {
        dx = scope.data[i] - scope.data[i - 1];
        if (maxdx < dx)
        {
            maxdx = dx;
            n = i;
        }

        if (maxdx > 0 && dx < 0)
            break;
    }

    static float max;

    if (max < 4096)
        max = 4096;

    float yscale = max / (height / 2);
    max = 0.0;

    // Graphics
    Graphics graphics(hbdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    // Move the origin
    graphics.TranslateTransform(0.0, height / 2.0);

    // Green pen for scope trace
    Pen pen(Color(0, 255, 0), 1);

    // Draw the trace
    PointF last(-1.0, 0.0);

    for (unsigned int i = 0; i < width && i < scope.length; i++)
    {
        if (max < abs(scope.data[n + i]))
            max = abs(scope.data[n + i]);

        float y = -scope.data[n + i] / yscale;
        PointF point(i, y);
        graphics.DrawLine(&pen, last, point);

        last = point;
    }

    // Move the origin back
    // SetViewportOrgEx(hbdc, 0, 0, NULL);

    // Show F if filtered
    if (audio.filter)
    {
        // Yellow text
        SetTextColor(hbdc, RGB(255, 255, 0));
        TextOut(hbdc, 0, height + 1, "F", 1);
    }

    // Copy the bitmap
    BitBlt(hdc, rect.left, rect.top, width, height,
           hbdc, 0, 0, SRCCOPY);

    return true;
}

// Draw spectrum
BOOL DrawSpectrum(HDC hdc, RECT rect)
{
    using Gdiplus::SmoothingModeAntiAlias;
    using Gdiplus::GraphicsPath;
    using Gdiplus::SolidBrush;
    using Gdiplus::Graphics;
    using Gdiplus::PointF;
    using Gdiplus::Color;
    using Gdiplus::Pen;

    static HBITMAP bitmap;
    static HFONT font;
    static SIZE size;
    static HDC hbdc;

    enum
    {FONT_HEIGHT   = 10};

    // Bold font
    static LOGFONT lf =
        {0, 0, 0, 0,
         FW_BOLD,
         false, false, false,
         DEFAULT_CHARSET,
         OUT_DEFAULT_PRECIS,
         CLIP_DEFAULT_PRECIS,
         DEFAULT_QUALITY,
         DEFAULT_PITCH | FF_DONTCARE,
         ""};

    static TCHAR s[16];

    // Create font
    if (font == NULL)
    {
        lf.lfHeight = FONT_HEIGHT;
        font = CreateFontIndirect(&lf);
    }

    // Draw nice etched edge
    DrawEdge(hdc, &rect, EDGE_SUNKEN, BF_ADJUST | BF_RECT);

    // Calculate bitmap dimensions
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Create DC
    if (hbdc == NULL)
    {
        hbdc = CreateCompatibleDC(hdc);
        SelectObject(hbdc, GetStockObject(DC_PEN));

        // Select font
        SelectObject(hbdc, font);
        SetBkMode(hbdc, TRANSPARENT);
    }

    // Create new bitmaps if resized
    if (width != size.cx || height != size.cy)
    {
        // Delete old bitmap
        if (bitmap != NULL)
            DeleteObject(bitmap);

        // Create new bitmap
        bitmap = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(hbdc, bitmap);

        size.cx = width;
        size.cy = height;
    }

    // Erase background
    SetViewportOrgEx(hbdc, 0, 0, NULL);
    RECT brct = {0, 0, width, height};
    FillRect(hbdc, &brct, (HBRUSH)GetStockObject(BLACK_BRUSH));

    // Dark green graticule
    SetDCPenColor(hbdc, RGB(0, 64, 0));

    // Draw graticule
    for (int i = 4; i < width; i += 5)
    {
        MoveToEx(hbdc, i, 0, NULL);
        LineTo(hbdc, i, height);
    }

    for (int i = 4; i < height; i += 5)
    {
        MoveToEx(hbdc, 0, i, NULL);
        LineTo(hbdc, width, i);
    }

    // Don't attempt the trace until there's a buffer
    if (spectrum.data == NULL)
    {
        // Copy the bitmap
        BitBlt(hdc, rect.left, rect.top, width, height,
               hbdc, 0, 0, SRCCOPY);

        return true;
    }

    static float max;

    if (max < 1.0)
        max = 1.0;

    // Calculate the scaling
    float yscale = (float)height / max;

    max = 0.0;

    // Graphics
    Graphics graphics(hbdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.TranslateTransform(0.0, height - 1.0);
    // Green pen for spectrum trace
    Pen pen(Color(0, 255, 0), 1);
    // Transparent green brush for spectrum fill
    SolidBrush brush(Color(63, 0, 255, 0));

    // Draw the spectrum
    PointF lastp(0.0, 0.0);
    GraphicsPath path;

    if (spectrum.zoom)
    {
        // Calculate scale
        float xscale = ((float)width / (spectrum.r - spectrum.l)) / 2.0;

        for (unsigned int i = floor(spectrum.l); i <= ceil(spectrum.h); i++)
        {
            if (i > 0 && i < spectrum.length)
            {
                float value = spectrum.data[i];

                if (max < value)
                    max = value;

                PointF point((i - spectrum.l) * xscale, -value * yscale);
                path.AddLine(lastp, point);
                lastp = point;
            }
        }

        graphics.DrawPath(&pen, &path);
        path.AddLine(lastp, PointF(width, 0.0));
        path.CloseFigure();
        graphics.FillPath(&brush, &path);

        SetViewportOrgEx(hbdc, 0, height - 1, NULL);
        SetDCPenColor(hbdc, RGB(0, 255, 0));
        MoveToEx(hbdc, width / 2, 0, NULL);
        LineTo(hbdc, width / 2, -height);

        // Yellow pen for frequency trace
        SetDCPenColor(hbdc, RGB(255, 255, 0));
        SetTextColor(hbdc, RGB(255, 255, 0));
        SetTextAlign(hbdc, TA_CENTER | TA_BOTTOM);

        // Draw lines for each frequency
        for (int i = 0; i < spectrum.count; i++)
        {
            // Draw line for each that are in range
            if (spectrum.values[i] > spectrum.l &&
                spectrum.values[i] < spectrum.h)
            {
                int x = round((spectrum.values[i] - spectrum.l) * xscale);
                MoveToEx(hbdc, x, 0, NULL);
                LineTo(hbdc, x, -height);

                double f = display.maxima[i].f;

                // Reference freq
                double fr = display.maxima[i].fr;
                double c = -12.0 * log2(fr / f);

                // Ignore silly values
                if (!isfinite(c))
                    continue;

                sprintf(s, "%+0.0f", c * 100.0);
                TextOut(hbdc, x, 2, s, lstrlen(s));
            }
        }
    }

    else
    {
        float xscale = log((float)spectrum.length /
                           (float)spectrum.expand) / width;
        int last = 1;
        for (int x = 0; x < width; x++)
        {
            float value = 0.0;

            int index = (int)round(pow(M_E, x * xscale));
            for (unsigned int i = last; i <= index; i++)
            {
                // Don't show DC component
                if (i > 0 && i < spectrum.length)
                {
                    if (value < spectrum.data[i])
                        value = spectrum.data[i];
                }
            }

            // Update last index
            last = index;

            if (max < value)
                max = value;

            float y = -value * yscale;

            PointF point(x, y);
            path.AddLine(lastp, point);

            lastp = point;
        }

        graphics.DrawPath(&pen, &path);
        path.AddLine(lastp, PointF(width, 0.0));
        path.CloseFigure();
        graphics.FillPath(&brush, &path);

        // Yellow pen for frequency trace
        SetViewportOrgEx(hbdc, 0, height - 1, NULL);
        SetDCPenColor(hbdc, RGB(255, 255, 0));
        SetTextColor(hbdc, RGB(255, 255, 0));
        SetTextAlign(hbdc, TA_CENTER | TA_BOTTOM);

        // Draw lines for each frequency
        for (int i = 0; i < spectrum.count; i++)
        {
            // Draw line for each
            int x = round(log(spectrum.values[i]) / xscale);
            MoveToEx(hbdc, x, 0, NULL);
            LineTo(hbdc, x, -height);

            double f = display.maxima[i].f;

            // Reference freq
            double fr = display.maxima[i].fr;
            double c = -12.0 * log2(fr / f);

            // Ignore silly values
            if (!isfinite(c))
                continue;

            sprintf(s,  "%+0.0f", c * 100.0);
            TextOut(hbdc, x, 2, s, lstrlen(s));
        }

        SetTextAlign(hbdc, TA_LEFT | TA_BOTTOM);

        if (spectrum.expand > 1)
        {
            sprintf(s, "x%d", spectrum.expand);
            TextOut(hbdc, 0, 2, s, lstrlen(s));
        }
    }

    // D for downsample
    if (audio.down)
    {
        SetTextAlign(hbdc, TA_LEFT | TA_BOTTOM);
        TextOut(hbdc, 0, 10 - height, "D", 1);
    }

    // Move the origin back
    SetViewportOrgEx(hbdc, 0, 0, NULL);

    // Copy the bitmap
    BitBlt(hdc, rect.left, rect.top, width, height,
           hbdc, 0, 0, SRCCOPY);

    return true;
}

// Draw display
BOOL DrawDisplay(HDC hdc, RECT rect)
{
    static HBITMAP bitmap;
    static HFONT medium;
    static HFONT larger;
    static HFONT large;
    static HFONT music;
    static HFONT half;
    static HFONT font;
    static SIZE size;
    static HDC hbdc;

    static const TCHAR *notes[] =
        {"C", "C", "D", "E", "E", "F",
         "F", "G", "A", "A", "B", "B"};

    static const TCHAR *sharps[] =
        {"", "#", "", "b", "", "",
         "#", "", "b", "", "b", ""};

    // Bold font
    static LOGFONT lf =
        {0, 0, 0, 0,
         FW_BOLD,
         false, false, false,
         DEFAULT_CHARSET,
         OUT_DEFAULT_PRECIS,
         CLIP_DEFAULT_PRECIS,
         ANTIALIASED_QUALITY,
         DEFAULT_PITCH | FF_DONTCARE,
         ""};

    static TCHAR s[64];

    // Draw nice etched edge
    DrawEdge(hdc, &rect , EDGE_SUNKEN, BF_ADJUST | BF_RECT);

    // Calculate dimensions
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Create new fonts if resized
    if (width != size.cx || height != size.cy)
    {
        if (font != NULL)
        {
            DeleteObject(font);
            DeleteObject(half);
            DeleteObject(large);
            DeleteObject(larger);
            DeleteObject(medium);
            DeleteObject(music);
        }

        lf.lfHeight = height / 3;
        lf.lfWeight = FW_BOLD;
        strcpy(lf.lfFaceName, "");
        large = CreateFontIndirect(&lf);

        lf.lfHeight = height / 2;
        larger = CreateFontIndirect(&lf);

        lf.lfHeight = height / 5;
        medium = CreateFontIndirect(&lf);

        lf.lfHeight = height / 8;
        lf.lfWeight = FW_NORMAL;
        font = CreateFontIndirect(&lf);

        lf.lfHeight = height / 4;
        lf.lfWeight = FW_BOLD;
        half = CreateFontIndirect(&lf);

        lf.lfHeight = height / 4;
        strcpy(lf.lfFaceName, "Musica");
        music = CreateFontIndirect(&lf);
    }

    // Create DC
    if (hbdc == NULL)
        hbdc = CreateCompatibleDC(hdc);

    // Create new bitmaps if resized
    if (width != size.cx || height != size.cy)
    {
        // Delete old bitmap
        if (bitmap != NULL)
            DeleteObject(bitmap);

        // Create new bitmap
        bitmap = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(hbdc, bitmap);

        size.cx = width;
        size.cy = height;
    }

    // Erase background
    RECT brct = {0, 0, width, height};
    FillRect(hbdc, &brct, (HBRUSH)GetStockObject(WHITE_BRUSH));

    if (display.mult)
    {
        // Select font
        SelectObject(hbdc, font);

        // Set text align
        SetTextAlign(hbdc, TA_TOP);

        if (display.count == 0)
        {
            int x = 4;

            // Display note
            sprintf(s, "%s%s%d", notes[(display.n - display.transpose +
                                        OCTAVE) % OCTAVE],
                sharps[(display.n - display.transpose +
                        OCTAVE) % OCTAVE], display.n / 12);
            TextOut(hbdc, x, 0, s, lstrlen(s));

            GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
            x += size.cx + 4;

            // Display cents
            sprintf(s, "%+4.2lfc", display.c * 100.0);
            TextOut(hbdc, x, 0, s, lstrlen(s));

            GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
            x += size.cx + 4;

            // Display reference
            sprintf(s, "%4.2lfHz", display.fr);
            TextOut(hbdc, x, 0, s, lstrlen(s));

            GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
            x += size.cx + 4;

            // Display frequency
            sprintf(s, "%4.2lfHz", display.f);
            TextOut(hbdc, x, 0, s, lstrlen(s));

            GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
            x += size.cx + 4;

            // Display difference
            sprintf(s, "%+4.2lfHz", display.f - display.fr);
            TextOut(hbdc, x, 0, s, lstrlen(s));
        }

        int y = 0;

        double f_prev = 0;
        double c_prev = 0;

        for (int i = 0; i < display.count; i++)
        {
            double f = display.maxima[i].f;

            // Reference freq
            double fr = display.maxima[i].fr;

            int n = display.maxima[i].n;

            if (n < 0)
                n = 0;

            double c = -12.0 * log2(fr / f);

            // Ignore silly values
            if (!isfinite(c))
                continue;

            int x = 4;

            // Display note
            sprintf(s, "%s%s%d", notes[(n - display.transpose +
                                        OCTAVE) % OCTAVE],
                    sharps[(n  - display.transpose +
                            OCTAVE) % OCTAVE], n / 12);
            TextOut(hbdc, x, y, s, lstrlen(s));

            GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
            x += size.cx + 4;

            // Display frequency
            sprintf(s, "%4.2lf Hz", f);
            TextOut(hbdc, x, y, s, lstrlen(s));

            GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
            x += size.cx + 4;

            // Display cents
            sprintf(s, ", %+4.2lf c", c * 100.0);
            TextOut(hbdc, x, y, s, lstrlen(s));


            if (i > 0)
            {
                GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
                x += size.cx + 4;

                // Display beat
                sprintf(s, ", %4.2lf beat", f - f_prev);
                TextOut(hbdc, x, y, s, lstrlen(s));

                GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
                x += size.cx + 4;

                // Display cent difference
                sprintf(s, ", %4.2lf dc", (c - c_prev) * 100.0);
                TextOut(hbdc, x, y, s, lstrlen(s));

            }

            GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
            y += size.cy;

            f_prev = f;
            c_prev = c;
        }
    }

    else
    {
        // Select larger font
        SelectObject(hbdc, larger);

        // Text size
        SIZE size = {0};

        // Set text align
        SetTextAlign(hbdc, TA_BOTTOM|TA_LEFT);
        SetBkMode(hbdc, TRANSPARENT);

        // Display note
        sprintf(s, "%s", notes[(display.n  - display.transpose +
                                OCTAVE) % OCTAVE]);
        GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);

        int y = size.cy;
        TextOut(hbdc, 8, y, s, lstrlen(s));

        int x = size.cx + 8;

        // Select music font
        SelectObject(hbdc, half);

        sprintf(s, "%d", display.n / 12);
        TextOut(hbdc, x, y, s, lstrlen(s));

        // Select music font
        SelectObject(hbdc, music);

        sprintf(s, "%s", sharps[(display.n  - display.transpose +
                                 OCTAVE) % OCTAVE]);
        GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
        TextOut(hbdc, x, y - size.cy, s, lstrlen(s));

        // Select large font
        SelectObject(hbdc, large);
        SetTextAlign(hbdc, TA_BOTTOM|TA_RIGHT);

        // Display cents
        sprintf(s, "%+4.2fc", display.c * 100.0);
        TextOut(hbdc, width - 8, y, s, lstrlen(s));

        // Select medium font
        SelectObject(hbdc, medium);
        SetTextAlign(hbdc, TA_BOTTOM|TA_LEFT);

        // Display reference frequency
        sprintf(s, "%4.2fHz", display.fr);
        GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
        y += size.cy;
        TextOut(hbdc, 8, y, s, lstrlen(s));

        // Display actual frequency
        SetTextAlign(hbdc, TA_BOTTOM|TA_RIGHT);
        sprintf(s, "%4.2fHz", display.f);
        TextOut(hbdc, width - 8, y, s, lstrlen(s));


        // Display reference
        SetTextAlign(hbdc, TA_BOTTOM|TA_LEFT);
        sprintf(s, "%4.2fHz", audio.reference);
        GetTextExtentPoint32(hbdc, s, lstrlen(s), &size);
        y += size.cy;
        TextOut(hbdc, 8, y, s, lstrlen(s));

        // Display frequency difference
        SetTextAlign(hbdc, TA_BOTTOM|TA_RIGHT);
        sprintf(s, "%+4.2lfHz", display.f - display.fr);
        TextOut(hbdc, width - 8, y, s, lstrlen(s));
    }

    // Show lock
    if (display.lock)
        DrawLock(hbdc, -1, height + 1);

    // Copy the bitmap
    BitBlt(hdc, rect.left, rect.top, width, height,
           hbdc, 0, 0, SRCCOPY);

    return true;
}

// Draw lock icon
BOOL DrawLock(HDC hdc, int x, int y)
{
    POINT point;
    POINT body[] =
        {{2, -3}, {8, -3}, {8, -8}, {2, -8}, {2, -3}};
    POINT hasp[] =
        {{3, -8}, {3, -11}, {7, -11}, {7, -8}};

    SetViewportOrgEx(hdc, x, y, &point);

    Polyline(hdc, body, Length(body));
    Polyline(hdc, hasp, Length(hasp));

    SetPixel(hdc, 3, -11, RGB(255, 170, 85));
    SetPixel(hdc, 6, -10, RGB(255, 170, 85));

    SetPixel(hdc, 4, -10, RGB(85, 170, 255));
    SetPixel(hdc, 7, -11, RGB(85, 170, 255));

    SetPixel(hdc, 7, -7, RGB(255, 170, 85));
    SetPixel(hdc, 7, -4, RGB(255, 170, 85));

    SetPixel(hdc, 3, -7, RGB(85, 170, 255));
    SetPixel(hdc, 3, -4, RGB(85, 170, 255));

    SetViewportOrgEx(hdc, point.x, point.y, NULL);
    return true;
}

// Draw meter
BOOL DrawMeter(HDC hdc, RECT rect)
{
    using Gdiplus::SmoothingModeAntiAlias;
    using Gdiplus::LinearGradientBrush;
    using Gdiplus::WrapModeTileFlipX;
    using Gdiplus::GraphicsPath;
    using Gdiplus::SolidBrush;
    using Gdiplus::Graphics;
    using Gdiplus::Matrix;
    using Gdiplus::Color;
    using Gdiplus::Point;
    using Gdiplus::Pen;

    static HBITMAP bitmap;
    static HFONT font;
    static SIZE size;
    static HDC hbdc;

    static float mc;

    // Plain vanilla font
    static LOGFONT lf =
        {0, 0, 0, 0,
         FW_NORMAL,
         false, false, false,
         DEFAULT_CHARSET,
         OUT_DEFAULT_PRECIS,
         CLIP_DEFAULT_PRECIS,
         ANTIALIASED_QUALITY,
         DEFAULT_PITCH | FF_DONTCARE,
         ""};

    // Draw nice etched edge
    DrawEdge(hdc, &rect , EDGE_SUNKEN, BF_ADJUST | BF_RECT);

    // Calculate bitmap dimensions
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Create DC
    if (hbdc == NULL)
    {
        // Create DC
        hbdc = CreateCompatibleDC(hdc);
        SetTextAlign(hbdc, TA_CENTER);
    }

    // Create new font and bitmap if resized
    if (width != size.cx || height != size.cy)
    {
        if (font != NULL)
            DeleteObject(font);

        lf.lfHeight = height / 3;
        font = CreateFontIndirect(&lf);
        SelectObject(hbdc, font);

        // Delete old bitmap
        if (bitmap != NULL)
            DeleteObject(bitmap);

        // Create new bitmap
        bitmap = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(hbdc, bitmap);

        size.cx = width;
        size.cy = height;
    }

    // Erase background
    SetViewportOrgEx(hbdc, 0, 0, NULL);
    RECT brct = {0, 0, width, height};
    FillRect(hbdc, &brct, (HBRUSH)GetStockObject(WHITE_BRUSH));

    // Move origin
    SetViewportOrgEx(hbdc, width / 2, 0, NULL);

    // Draw the meter scale
    for (int i = 0; i < 6; i++)
    {
        static TCHAR s[16];
        int x = width / 11 * i;

        sprintf(s, "%d", i * 10);
        TextOut(hbdc, x + 1, 0, s, lstrlen(s));
        TextOut(hbdc, -x + 1, 0, s, lstrlen(s));

        MoveToEx(hbdc, x, height / 3, NULL);
        LineTo(hbdc, x, height / 2);
        MoveToEx(hbdc, -x, height / 3, NULL);
        LineTo(hbdc, -x, height / 2);

        for (int j = 1; j < 5; j++)
        {
            if (i < 5)
            {
                MoveToEx(hbdc, x + j * width / 55, height * 3 / 8, NULL);
                LineTo(hbdc, x + j * width / 55, height / 2);
            }

            MoveToEx(hbdc, -x + j * width / 55, height * 3 / 8, NULL);
            LineTo(hbdc, -x + j * width / 55, height / 2);
        }
    }

    RECT bar = {-width * 5 / 11, (height * 3 / 4) - 2,
                (width * 5 / 11) + 1, (height * 3 / 4) + 2};
    FrameRect(hbdc, &bar, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

    // Do calculation
    mc = ((mc * 19.0) + meter.c) / 20.0;

    GraphicsPath path;
    path.AddLine(0, 2, 1, 1);
    path.AddLine(1, 1, 1, -2);
    path.AddLine(1, -2, -1, -2);
    path.AddLine(-1, -2, -1, 1);
    path.CloseFigure();

    LinearGradientBrush brush(Point(0, 2), Point(0, -2),
                              Color(255, 255, 255), Color(63, 63, 63));
    brush.SetWrapMode(WrapModeTileFlipX);

    Matrix matrix;
    matrix.Translate(mc * width * 10.0 / 11.0, (height * 3 / 4) - 2);
    matrix.Scale(height / 12.0, -height / 12.0);

    path.Transform(&matrix);
    brush.ScaleTransform(height / 24.0, height / 24.0);

    Pen pen(Color(127, 127, 127));

    Graphics graphics(hbdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.FillPath(&brush, &path);
    graphics.DrawPath(&pen, &path);

    // Move origin back
    SetViewportOrgEx(hbdc, 0, 0, NULL);

    // Copy the bitmap
    BitBlt(hdc, rect.left, rect.top, width, height,
           hbdc, 0, 0, SRCCOPY);

    return true;
}

// Draw strobe
BOOL DrawStrobe(HDC hdc, RECT rect)
{
    static float mc = 0.0;
    static float mx = 0.0;

    static HBRUSH sbrush;
    static HBRUSH sshade;
    static HBRUSH mbrush;
    static HBRUSH mshade;
    static HBRUSH lbrush;
    static HBRUSH lshade;
    static HBRUSH ebrush;

    static SIZE size;

    // Colours
    static int colours[][2] =
        {{RGB(63, 63, 255), RGB(63, 255, 255)},
         {RGB(111, 111, 0), RGB(191, 255, 191)},
         {RGB(255, 63, 255), RGB(255, 255, 63)}};

    // Draw nice etched edge
    DrawEdge(hdc, &rect , EDGE_SUNKEN, BF_ADJUST | BF_RECT);

    // Calculate dimensions
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    int block = height / 4;

    // Create brushes
    int foreground = colours[strobe.colours][0];
    int background = colours[strobe.colours][1];

    if (sbrush == NULL || strobe.changed ||
        size.cx != width || size.cy != height)
    {
        if (sbrush != NULL)
            DeleteObject(sbrush);

        if (sshade != NULL)
            DeleteObject(sshade);

        if (mbrush != NULL)
            DeleteObject(mbrush);

        if (mshade != NULL)
            DeleteObject(mshade);

        if (lbrush != NULL)
            DeleteObject(lbrush);

        if (lshade != NULL)
            DeleteObject(lshade);

        if (ebrush != NULL)
            DeleteObject(ebrush);

        HDC hbdc = CreateCompatibleDC(hdc);

        SelectObject(hbdc, CreateCompatibleBitmap(hdc, block * 2, block));
        SelectObject(hbdc, GetStockObject(DC_PEN));
        SelectObject(hbdc, GetStockObject(DC_BRUSH));

        SetDCPenColor(hbdc, foreground);
        SetDCBrushColor(hbdc, foreground);
        Rectangle(hbdc, 0, 0, block, block);
        SetDCPenColor(hbdc, background);
        SetDCBrushColor(hbdc, background);
        Rectangle(hbdc, block, 0, block * 2, block);

        sbrush = CreatePatternBrush((HBITMAP)GetCurrentObject(hbdc,
                                                              OBJ_BITMAP));
        DeleteObject(GetCurrentObject(hbdc, OBJ_BITMAP));

        SelectObject(hbdc, CreateCompatibleBitmap(hdc, block * 2, block));

        TRIVERTEX vertex[] =
            {{0, 0,
              (COLOR16)(GetRValue(foreground) << 8),
              (COLOR16)(GetGValue(foreground) << 8),
              (COLOR16)(GetBValue(foreground) << 8),
              0},
             {block, block,
              (COLOR16)(GetRValue(background) << 8),
              (COLOR16)(GetGValue(background) << 8),
              (COLOR16)(GetBValue(background) << 8),
              0},
             {block * 2, 0,
              (COLOR16)(GetRValue(foreground) << 8),
              (COLOR16)(GetGValue(foreground) << 8),
              (COLOR16)(GetBValue(foreground) << 8),
              0}};

        GRADIENT_RECT gradient[] =
            {{0, 1}, {1, 2}};

        GradientFill(hbdc, vertex, Length(vertex),
                     gradient, Length(gradient), GRADIENT_FILL_RECT_H);

        sshade = CreatePatternBrush((HBITMAP)GetCurrentObject(hbdc,
                                                              OBJ_BITMAP));
        DeleteObject(GetCurrentObject(hbdc, OBJ_BITMAP));

        SelectObject(hbdc, CreateCompatibleBitmap(hdc, block * 4, block));

        SetDCPenColor(hbdc, foreground);
        SetDCBrushColor(hbdc, foreground);
        Rectangle(hbdc, 0, 0, block * 2, block);
        SetDCPenColor(hbdc, background);
        SetDCBrushColor(hbdc, background);
        Rectangle(hbdc, block * 2, 0, block * 4, block);

        mbrush = CreatePatternBrush((HBITMAP)GetCurrentObject(hbdc,
                                                              OBJ_BITMAP));
        DeleteObject(GetCurrentObject(hbdc, OBJ_BITMAP));

        SelectObject(hbdc, CreateCompatibleBitmap(hdc, block * 4, block));

        vertex[1].x = block * 2;
        vertex[2].x = block * 4;

        GradientFill(hbdc, vertex, 3, gradient, 2, GRADIENT_FILL_RECT_H);

        mshade = CreatePatternBrush((HBITMAP)GetCurrentObject(hbdc,
                                                              OBJ_BITMAP));
        DeleteObject(GetCurrentObject(hbdc, OBJ_BITMAP));

        SelectObject(hbdc, CreateCompatibleBitmap(hdc, block * 8, block));

        SetDCPenColor(hbdc, foreground);
        SetDCBrushColor(hbdc, foreground);
        Rectangle(hbdc, 0, 0, block * 4, block);
        SetDCPenColor(hbdc, background);
        SetDCBrushColor(hbdc, background);
        Rectangle(hbdc, block * 4, 0, block * 8, block);

        lbrush = CreatePatternBrush((HBITMAP)GetCurrentObject(hbdc,
                                                              OBJ_BITMAP));
        DeleteObject(GetCurrentObject(hbdc, OBJ_BITMAP));

        SelectObject(hbdc, CreateCompatibleBitmap(hdc, block * 8, block));

        vertex[1].x = block * 4;
        vertex[2].x = block * 8;

        GradientFill(hbdc, vertex, 3, gradient, 2, GRADIENT_FILL_RECT_H);

        lshade = CreatePatternBrush((HBITMAP)GetCurrentObject(hbdc,
                                                              OBJ_BITMAP));
        DeleteObject(GetCurrentObject(hbdc, OBJ_BITMAP));

        SelectObject(hbdc, CreateCompatibleBitmap(hdc, block * 16, block));

        SetDCPenColor(hbdc, foreground);
        SetDCBrushColor(hbdc, foreground);
        Rectangle(hbdc, 0, 0, block * 8, block);
        SetDCPenColor(hbdc, background);
        SetDCBrushColor(hbdc, background);
        Rectangle(hbdc, block * 8, 0, block * 16, block);

        ebrush = CreatePatternBrush((HBITMAP)GetCurrentObject(hbdc,
                                                              OBJ_BITMAP));
        DeleteObject(GetCurrentObject(hbdc, OBJ_BITMAP));

        DeleteDC(hbdc);
        strobe.changed = false;
    }

    if (true)
    {
        // Transform viewport
        SetViewportOrgEx(hdc, rect.left, rect.top, NULL);

        mc = ((19.0 * mc) + strobe.c) / 20.0;
        mx += mc * 50.0;

        if (mx > block * 16.0)
            mx = 0.0;

        if (mx < 0.0)
            mx = block * 16.0;

        int rx = round(mx - block * 16.0);
        SetBrushOrgEx(hdc, rx, 0, NULL);
        SelectObject(hdc, GetStockObject(NULL_PEN));

        if (fabs(mc) > 0.4)
        {
            SelectObject(hdc, GetStockObject(DC_BRUSH));
            SetDCBrushColor(hdc, background);
        }

        else if (fabs(mc) > 0.2)
            SelectObject(hdc, sshade);

        else
            SelectObject(hdc, sbrush);
        Rectangle(hdc, 0, 0, width, block);

        if (fabs(mc) > 0.3)
            SelectObject(hdc, mshade);

        else
            SelectObject(hdc, mbrush);
        Rectangle(hdc, 0, block, width, block * 2);

        if (fabs(mc) > 0.4)
            SelectObject(hdc, lshade);

        else
            SelectObject(hdc, lbrush);
        Rectangle(hdc, 0, block * 2, width, block * 3);

        SelectObject(hdc, ebrush);
        Rectangle(hdc, 0, block * 3, width, block * 4);
    }

    else
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

    return true;
}

// Draw staff
BOOL DrawStaff(HDC hdc, RECT rect)
{
    using Gdiplus::SmoothingModeAntiAlias;
    using Gdiplus::GraphicsPath;
    using Gdiplus::SolidBrush;
    using Gdiplus::Graphics;
    using Gdiplus::Matrix;
    using Gdiplus::PointF;
    using Gdiplus::Color;
    using Gdiplus::RectF;
    using Gdiplus::SizeF;
    using Gdiplus::Font;
    using Gdiplus::Pen;

    static HBITMAP bitmap;
    static SIZE size;
    static HDC hbdc;

    // Treble clef
    static const float tc[][2] =
        {
         {-6, 16}, {-8, 13},
         {-14, 19}, {-10, 35}, {2, 35},
         {8, 37}, {21, 30}, {21, 17},
         {21, 5}, {10, -1}, {0, -1},
         {-12, -1}, {-23, 5}, {-23, 22},
         {-23, 29}, {-22, 37}, {-7, 49},
         {10, 61}, {10, 68}, {10, 73},
         {10, 78}, {9, 82}, {7, 82},
         {2, 78}, {-2, 68}, {-2, 62},
         {-2, 25}, {10, 18}, {11, -8},
         {11, -18}, {5, -23}, {-4, -23},
         {-10, -23}, {-15, -18}, {-15, -13},
         {-15, -8}, {-12, -4}, {-7, -4},
         {3, -4}, {3, -20}, {-6, -17},
         {-5, -23}, {9, -20}, {9, -9},
         {7, 24}, {-5, 30}, {-5, 67},
         {-5, 78}, {-2, 87}, {7, 91},
         {13, 87}, {18, 80}, {17, 73},
         {17, 62}, {10, 54}, {1, 45},
         {-5, 38}, {-15, 33}, {-15, 19},
         {-15, 7}, {-8, 1}, {0, 1},
         {8, 1}, {15, 6}, {15, 14},
         {15, 23}, {7, 26}, {2, 26},
         {-5, 26}, {-9, 21}, {-6, 16}
        };

    // Bass clef
    static const float bc[][2] =
        {
         {-2.3,3},
         {6,7}, {10.5,12}, {10.5,16},
         {10.5,20.5}, {8.5,23.5}, {6.2,23.3},
         {5.2,23.5}, {2,23.5}, {0.5,19.5},
         {2,20}, {4,19.5}, {4,18},
         {4,17}, {3.5,16}, {2,16},
         {1,16}, {0,16.9}, {0,18.5},
         {0,21}, {2.1,24}, {6,24},
         {10,24}, {13.5,21.5}, {13.5,16.5},
         {13.5,11}, {7,5.5}, {-2.0,2.8},
         {14.9,21},
         {14.9,22.5}, {16.9,22.5}, {16.9,21},
         {16.9,19.5}, {14.9,19.5}, {14.9,21},
         {14.9,15},
         {14.9,16.5}, {16.9,16.5}, {16.9,15},
         {16.9,13.5}, {14.9,13.5}, {14.9,15}
        };

    // Note head
    static const float hd[][2] =
        {
         {8.0, 0.0},
         {8.0, 8.0}, {-8.0, 8.0}, {-8.0, 0.0},
         {-8.0, -8.0}, {8.0, -8.0}, {8.0, 0.0}
        };

    // Sharp symbol
    static const float sp[][2] =
        {
         {35, 35}, // 0
         {8, 22}, // 1
         {8, 46}, // 2
         {35, 59}, // 3
         {35, 101}, // 4
         {8, 88}, // 5
         {8, 111}, // 6
         {35, 125}, // 7
         {35, 160}, // 8
         {44, 160}, // 9
         {44, 129}, // 10
         {80, 147}, // 11
         {80, 183}, // 12
         {89, 183}, // 13
         {89, 151}, // 14
         {116, 165}, // 15
         {116, 141}, // 16
         {89, 127}, // 17
         {89, 86}, // 18
         {116, 100}, // 19
         {116, 75}, // 20
         {89, 62}, // 21
         {89, 19}, // 22
         {80, 19}, // 23
         {80, 57}, // 23
         {44, 39}, // 25
         {44, -1}, // 26
         {35, -1}, // 27
         {35, 35}, // 28
         {44, 64}, // 29
         {80, 81}, // 30
         {80, 123}, // 31
         {44, 105}, // 32
         {44, 64}, // 33
        };

    // Flat symbol
    static const float ft[][2] =
        {
         {20, 86}, // 0
         {28, 102.667}, {41.6667, 111}, {61, 111}, // 3
         {71.6667, 111}, {80.3333, 107.5}, {87, 100.5}, // 6
         {93.6667, 93.5}, {97, 83.6667}, {97, 71}, // 9
         {97, 53}, {89, 36.6667}, {73, 22}, // 12
         {57, 7.33333}, {35.3333, -1.33333}, {8, -4}, // 15
         {8, 195}, // 16
         {20, 195}, // 17
         {20, 86}, // 18
         {20, 7}, // 19
         {35.3333, 9}, {47.8333, 15.6667}, {57.5, 27}, // 22
         {67.1667, 38.3333}, {72, 51.6667}, {72, 67}, // 25
         {72, 75.6667}, {70.1667, 82.3333}, {66.5, 87}, // 28
         {62.8333, 91.6667}, {57.3333, 94}, {50, 94}, // 31
         {41.3333, 94}, {34.1667, 90.3333}, {28.5, 83}, // 34
         {22.8333, 75.6667}, {20, 64.6667}, {20, 50}, // 37
         {20, 7}, // 38
        };

    // Scale offsets
    static const int offset[] =
        {0, 0, 1, 2, 2, 3,
         3, 4, 5, 5, 6, 6};

    static const int sharps[] =
        {NATURAL, SHARP, NATURAL, FLAT, NATURAL, NATURAL,
         SHARP, NATURAL, FLAT, NATURAL, FLAT, NATURAL};

    // Draw nice etched edge
    DrawEdge(hdc, &rect , EDGE_SUNKEN, BF_ADJUST | BF_RECT);

    // Calculate dimensions
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    int lineHeight = height / 14.0;
    int lineWidth = width / 16.0;
    int margin = width / 32.0;

    // Create DC
    if (hbdc == NULL)
    {
        // Create DC
        hbdc = CreateCompatibleDC(hdc);
    }

    // Create new bitmap if resized
    if (width != size.cx || height != size.cy)
    {
        // Delete old bitmap
        if (bitmap != NULL)
            DeleteObject(bitmap);

        // Create new bitmap
        bitmap = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(hbdc, bitmap);

        size.cx = width;
        size.cy = height;
    }

    // Erase background
    SetViewportOrgEx(hbdc, 0, 0, NULL);
    RECT brct = {0, 0, width, height};
    FillRect(hbdc, &brct, (HBRUSH)GetStockObject(WHITE_BRUSH));

    // Move origin
    SetViewportOrgEx(hbdc, 0, height / 2, NULL);

    // Draw staff
    for (int i = 1; i < 6; i++)
    {
        MoveToEx(hbdc, margin, i * lineHeight, NULL);
        LineTo(hbdc, width - margin, i * lineHeight);
        MoveToEx(hbdc, margin, -i * lineHeight, NULL);
        LineTo(hbdc, width - margin, -i * lineHeight);
    }

    // Draw leger lines
    MoveToEx(hbdc, width / 2 - lineWidth / 2, 0, NULL);
    LineTo(hbdc, width / 2 + lineWidth / 2, 0);
    MoveToEx(hbdc, width / 2 + lineWidth * 5.5, -lineHeight * 6, NULL);
    LineTo(hbdc, width / 2 + lineWidth * 6.5, -lineHeight * 6);
    MoveToEx(hbdc, width / 2 - lineWidth * 5.5, lineHeight * 6, NULL);
    LineTo(hbdc, width / 2 - lineWidth * 6.5, lineHeight * 6);

    // Graphics
    Graphics graphics(hbdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    // Draw treble clef
    GraphicsPath tclef;
    tclef.AddLine(tc[0][0], tc[0][1], tc[1][0], tc[1][1]);
    for (unsigned int i = 1; i < Length(tc) - 1; i += 3)
        tclef.AddBezier(tc[i][0], tc[i][1], tc[i + 1][0], tc[i + 1][1],
                        tc[i + 2][0], tc[i + 2][1], tc[i + 3][0], tc[i + 3][1]);
    Matrix matrix;
    RectF bounds;
    SizeF sizeF;
    Pen pen(Color(0, 0, 0));
    SolidBrush brush(Color(0, 0, 0));

    tclef.GetBounds(&bounds, &matrix, &pen);
    matrix.Translate(-(bounds.GetLeft() + bounds.GetRight()) / 2,
                     -(bounds.GetTop() + bounds.GetBottom()) / 2);
    tclef.Transform(&matrix);
    bounds.GetSize(&sizeF);
    float scale = (height / 2) / sizeF.Height;
    matrix.Reset();
    matrix.Scale(scale, -scale);
    tclef.Transform(&matrix);
    matrix.Reset();
    matrix.Translate(margin + lineWidth / 2, -lineHeight * 3);
    tclef.Transform(&matrix);
    graphics.FillPath(&brush, &tclef);

    // Draw bass clef
    GraphicsPath bclef;
    for (int i = 0; i < 27; i += 3)
        bclef.AddBezier(bc[i][0], bc[i][1], bc[i + 1][0], bc[i + 1][1],
                        bc[i + 2][0], bc[i + 2][1], bc[i + 3][0], bc[i + 3][1]);
    bclef.StartFigure();
    for (int i = 28; i < 34; i += 3)
        bclef.AddBezier(bc[i][0], bc[i][1], bc[i + 1][0], bc[i + 1][1],
                        bc[i + 2][0], bc[i + 2][1], bc[i + 3][0], bc[i + 3][1]);
    bclef.StartFigure();
    for (unsigned int i = 35; i < Length(bc) - 1; i += 3)
        bclef.AddBezier(bc[i][0], bc[i][1], bc[i + 1][0], bc[i + 1][1],
                        bc[i + 2][0], bc[i + 2][1], bc[i + 3][0], bc[i + 3][1]);

    bclef.GetBounds(&bounds, &matrix, &pen);
    matrix.Translate(-(bounds.GetLeft() + bounds.GetRight()) / 2,
                     -(bounds.GetTop() + bounds.GetBottom()) / 2);
    bclef.Transform(&matrix);
    bounds.GetSize(&sizeF);
    scale = (lineHeight * 4.5) / sizeF.Height;
    matrix.Reset();
    matrix.Scale(scale, -scale);
    bclef.Transform(&matrix);
    matrix.Reset();
    matrix.Translate(margin + lineWidth / 2, lineHeight * 2.8);
    bclef.Transform(&matrix);
    graphics.FillPath(&brush, &bclef);

    // Note head
    GraphicsPath head;
    for (unsigned int i = 0; i < Length(hd) - 1; i += 3)
        head.AddBezier(hd[i][0], hd[i][1], hd[i + 1][0], hd[i + 1][1],
                       hd[i + 2][0], hd[i + 2][1], hd[i + 3][0], hd[i + 3][1]);
    head.GetBounds(&bounds, &matrix, &pen);
    bounds.GetSize(&sizeF);
    scale = (lineHeight * 2) / sizeF.Height;
    matrix.Reset();
    matrix.Scale(scale, -scale);
    head.Transform(&matrix);

    // Sharp
    GraphicsPath sharp;
    for (unsigned int i = 0; i < 28; i++)
        sharp.AddLine(sp[i][0], sp[i][1], sp[i + 1][0], sp[i + 1][1]);
    sharp.StartFigure();
    for (int i = 29; i < 33; i++)
        sharp.AddLine(sp[i][0], sp[i][1], sp[i + 1][0], sp[i + 1][1]);
    matrix.Reset();
    sharp.GetBounds(&bounds, &matrix, &pen);
    matrix.Translate(-(bounds.GetLeft() + bounds.GetRight()) / 2,
                     -(bounds.GetTop() + bounds.GetBottom()) / 2);
    sharp.Transform(&matrix);
    matrix.Reset();
    bounds.GetSize(&sizeF);
    scale = (lineHeight * 3) / sizeF.Height;
    matrix.Scale(scale, -scale);
    sharp.Transform(&matrix);

    // Flat
    GraphicsPath flat;
    for (int i = 0; i < 15; i += 3)
        flat.AddBezier(ft[i][0], ft[i][1], ft[i + 1][0], ft[i + 1][1],
                       ft[i + 2][0], ft[i + 2][1], ft[i + 3][0], ft[i + 3][1]);
    for (int i = 15; i < 19; i++)
        flat.AddLine(ft[i][0], ft[i][1], ft[i + 1][0], ft[i + 1][1]);
    flat.StartFigure();
    for (int i = 19; i < 36; i += 3)
        flat.AddBezier(ft[i][0], ft[i][1], ft[i + 1][0], ft[i + 1][1],
                       ft[i + 2][0], ft[i + 2][1], ft[i + 3][0], ft[i + 3][1]);
    flat.AddLine(ft[37][0], ft[37][1], ft[38][0], ft[38][1]);
    matrix.Reset();
    flat.GetBounds(&bounds, &matrix, &pen);
    matrix.Translate(-(bounds.GetLeft() + bounds.GetRight()) / 2,
                     -(bounds.GetTop() + bounds.GetBottom()) / 2);
    flat.Transform(&matrix);
    matrix.Reset();
    bounds.GetSize(&sizeF);
    scale = (lineHeight * 3) / sizeF.Height;
    matrix.Scale(scale, -scale);
    flat.Transform(&matrix);

    // Calculate transform for note
    int xBase = lineWidth * 14;
    int yBase = lineHeight * 14;
    int note = staff.n - staff.transpose;
    int octave = note / OCTAVE;
    int index = (note + OCTAVE) % OCTAVE;

    // Wrap top two octaves
    if (octave >= 6)
        octave -= 2;

    // Wrap C0
    else if (octave == 0 && index <= 1)
        octave += 4;

    // Wrap bottom two octaves
    else if (octave <= 1 || (octave == 2 && index <= 1))
        octave += 2;

    float dx = (octave * lineWidth * 3.5) +
        (offset[index] * lineWidth / 2);
    float dy = (octave * lineHeight * 3.5) +
        (offset[index] * lineHeight / 2);

    // Draw note
    matrix.Reset();
    matrix.Translate(width / 2 - xBase + dx, yBase - dy);
    head.Transform(&matrix);
    graphics.FillPath(&brush, &head);

    // Accidentals
    switch (sharps[index])
    {
        // Natural
    case NATURAL:
        // Do nothing
        break;

        // Draw sharp
    case SHARP:
        matrix.Reset();
        matrix.Translate(width / 2 - xBase + dx - lineWidth / 2, yBase - dy);
        sharp.Transform(&matrix);
        graphics.FillPath(&brush, &sharp);
        break;

        // Draw flat
    case FLAT:
        matrix.Reset();
        matrix.Translate(width / 2 - xBase + dx - lineWidth / 2,
                         yBase - dy - lineHeight / 2);
        flat.Transform(&matrix);
        graphics.FillPath(&brush, &flat);
        break;
    }

    // Move origin back
    SetViewportOrgEx(hbdc, 0, 0, NULL);

    // Copy the bitmap
    BitBlt(hdc, rect.left, rect.top, width, height,
           hbdc, 0, 0, SRCCOPY);

    return true;
}
