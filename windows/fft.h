////////////////////////////////////////////////////////////////////////////////
//
//  fft.h - Fast Fourier Transform
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

#ifndef FFT_H
#define FFT_H

#include <math.h>

// Complex number structure for FFT computation
typedef struct
{
    double r;  // Real part
    double i;  // Imaginary part
} complex;

// Perform in-place radix-2 FFT on real input data
// Parameters:
//   a[] - Array of complex numbers (input: real values in .r, output: FFT result)
//   n   - Size of array (must be a power of 2)
void fftr(complex a[], int n);

#endif // FFT_H
