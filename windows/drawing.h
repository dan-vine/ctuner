////////////////////////////////////////////////////////////////////////////////
//
//  drawing.h - Drawing functions for Tuner UI components
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

#ifndef DRAWING_H
#define DRAWING_H

#include <windows.h>

// Draw item dispatcher - called from WM_DRAWITEM handler
BOOL DrawItem(WPARAM wParam, LPARAM lParam);

// Individual component drawing functions
BOOL DrawScope(HDC hdc, RECT rect);
BOOL DrawSpectrum(HDC hdc, RECT rect);
BOOL DrawDisplay(HDC hdc, RECT rect);
BOOL DrawLock(HDC hdc, int x, int y);
BOOL DrawMeter(HDC hdc, RECT rect);
BOOL DrawStrobe(HDC hdc, RECT rect);
BOOL DrawStaff(HDC hdc, RECT rect);

#endif // DRAWING_H
