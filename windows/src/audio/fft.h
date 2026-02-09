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

#include <cmath>
#include <vector>

namespace ctuner {

// Complex number structure for FFT computation
struct Complex {
    double r = 0.0;  // Real part
    double i = 0.0;  // Imaginary part

    Complex() = default;
    Complex(double real, double imag = 0.0) : r(real), i(imag) {}

    double magnitude() const { return std::hypot(r, i); }
    double phase() const { return std::atan2(i, r); }
};

// Perform in-place radix-2 FFT on real input data
// Parameters:
//   a[] - Array of complex numbers (input: real values in .r, output: FFT result)
//   n   - Size of array (must be a power of 2)
void fftr(Complex* a, int n);

// Overload for vector
void fftr(std::vector<Complex>& a);

} // namespace ctuner

#endif // FFT_H
