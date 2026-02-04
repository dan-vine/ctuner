////////////////////////////////////////////////////////////////////////////////
//
//  audio.h - Audio input and signal processing
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

#ifndef AUDIO_H
#define AUDIO_H

#include <windows.h>

// Audio thread - initializes audio input and processes messages
DWORD WINAPI AudioThread(LPVOID lpParameter);

// Process incoming audio data from waveform input
VOID WaveInData(WPARAM wParam, LPARAM lParam);

#endif // AUDIO_H
