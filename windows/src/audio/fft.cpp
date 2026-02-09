////////////////////////////////////////////////////////////////////////////////
//
//  fft.cpp - Fast Fourier Transform
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

#include "fft.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ctuner {

// Perform in-place radix-2 FFT on real input data
// Uses Cooley-Tukey decimation-in-time algorithm with bit-reversal permutation
void fftr(Complex* a, int n)
{
    double norm = std::sqrt(1.0 / n);

    // Bit-reversal permutation
    for (int i = 0, j = 0; i < n; i++)
    {
        if (j >= i)
        {
            double tr = a[j].r * norm;

            a[j].r = a[i].r * norm;
            a[j].i = 0.0;

            a[i].r = tr;
            a[i].i = 0.0;
        }

        int m = n / 2;
        while (m >= 1 && j >= m)
        {
            j -= m;
            m /= 2;
        }
        j += m;
    }

    // Danielson-Lanczos butterfly operations
    for (int mmax = 1, istep = 2 * mmax; mmax < n;
         mmax = istep, istep = 2 * mmax)
    {
        double delta = (M_PI / mmax);
        for (int m = 0; m < mmax; m++)
        {
            double w = m * delta;
            double wr = std::cos(w);
            double wi = std::sin(w);

            for (int i = m; i < n; i += istep)
            {
                int j = i + mmax;
                double tr = wr * a[j].r - wi * a[j].i;
                double ti = wr * a[j].i + wi * a[j].r;
                a[j].r = a[i].r - tr;
                a[j].i = a[i].i - ti;
                a[i].r += tr;
                a[i].i += ti;
            }
        }
    }
}

void fftr(std::vector<Complex>& a)
{
    fftr(a.data(), static_cast<int>(a.size()));
}

} // namespace ctuner
