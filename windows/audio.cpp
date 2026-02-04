////////////////////////////////////////////////////////////////////////////////
//
//  audio.cpp - Audio input and signal processing
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

// Audio thread - initializes waveform audio input and runs message loop
DWORD WINAPI AudioThread(LPVOID lpParameter)
{
    // Create wave format structure
    static WAVEFORMATEX wf =
        {WAVE_FORMAT_PCM, CHANNELS,
         SAMPLE_RATE, SAMPLE_RATE * BLOCK_ALIGN,
         BLOCK_ALIGN, BITS_PER_SAMPLE, 0};

    MMRESULT mmr;

    // Open a waveform audio input device
    mmr = waveInOpen(&audio.hwi, WAVE_MAPPER | WAVE_FORMAT_DIRECT, &wf,
                     (DWORD_PTR)audio.id,  (DWORD_PTR)NULL, CALLBACK_THREAD);

    if (mmr != MMSYSERR_NOERROR)
    {
        static TCHAR s[64];

        waveInGetErrorText(mmr, s, sizeof(s));
        MessageBox(window.hwnd, s, "WaveInOpen", MB_OK | MB_ICONERROR);
        return mmr;
    }

    // Create the waveform audio input buffers and structures
    static short data[4][STEP];
    static WAVEHDR hdrs[4] =
        {{(LPSTR)data[0], sizeof(data[0]), 0, 0, 0, 0},
         {(LPSTR)data[1], sizeof(data[1]), 0, 0, 0, 0},
         {(LPSTR)data[2], sizeof(data[2]), 0, 0, 0, 0},
         {(LPSTR)data[3], sizeof(data[3]), 0, 0, 0, 0}};

    for (int i = 0; i < Length(hdrs); i++)
    {
        // Prepare a waveform audio input header
        mmr = waveInPrepareHeader(audio.hwi, &hdrs[i], sizeof(WAVEHDR));
        if (mmr != MMSYSERR_NOERROR)
        {
            static TCHAR s[64];

            waveInGetErrorText(mmr, s, sizeof(s));
            MessageBox(window.hwnd, s, "WaveInPrepareHeader",
                       MB_OK | MB_ICONERROR);
            return mmr;
        }

        // Add a waveform audio input buffer
        mmr = waveInAddBuffer(audio.hwi, &hdrs[i], sizeof(WAVEHDR));
        if (mmr != MMSYSERR_NOERROR)
        {
            static TCHAR s[64];

            waveInGetErrorText(mmr, s, sizeof(s));
            MessageBox(window.hwnd, s, "WaveInAddBuffer",
                       MB_OK | MB_ICONERROR);
            return mmr;
        }
    }

    // Start the waveform audio input
    mmr = waveInStart(audio.hwi);
    if (mmr != MMSYSERR_NOERROR)
    {
        static TCHAR s[64];

        waveInGetErrorText(mmr, s, sizeof(s));
        MessageBox(window.hwnd, s, "WaveInStart", MB_OK | MB_ICONERROR);
        return mmr;
    }

    // Set up reference value
    if (audio.reference == 0)
        audio.reference = A5_REFNCE;

    // Create a message loop for processing thread messages
    MSG msg;
    BOOL flag;

    while ((flag = GetMessage(&msg, (HWND)-1, 0, 0)) != 0)
    {
        if (flag == -1)
            break;

        // Process messages
        switch (msg.message)
        {
            // Audio input opened
        case MM_WIM_OPEN:
            // Not used
            break;

            // Audio input data
        case MM_WIM_DATA:
            WaveInData(msg.wParam, msg.lParam);
            break;

            // Audio input closed
        case MM_WIM_CLOSE:
            // Not used
            break;
        }
    }

    return msg.wParam;
}

// Process incoming audio data - performs FFT and pitch detection
VOID WaveInData(WPARAM wParam, LPARAM lParam)
{
    enum
    {TIMER_COUNT = 16};

    // Create buffers for processing the audio data
    static double buffer[SAMPLES];
    static complex x[SAMPLES];

    static double xa[RANGE];
    static double xp[RANGE];
    static double xf[RANGE];

    static double x2[RANGE / 2];
    static double x3[RANGE / 3];
    static double x4[RANGE / 4];
    static double x5[RANGE / 5];

    static double dx[RANGE];

    static maximum maxima[MAXIMA];
    static double  values[MAXIMA];

    static double fps = (double)SAMPLE_RATE / (double)SAMPLES;
    static double expect = 2.0 * M_PI * (double)STEP / (double)SAMPLES;

    // initialise data structs
    if (scope.data == NULL)
    {
        scope.length = STEP;

        spectrum.data = xa;
        spectrum.length = RANGE;

        spectrum.values = values;
        display.maxima = maxima;
    }

    // Copy the input data
    memmove(buffer, buffer + STEP, (SAMPLES - STEP) * sizeof(double));

    short *data = (short *)((WAVEHDR *)lParam)->lpData;

    // Butterworth filter, 3dB/octave
    for (int i = 0; i < STEP; i++)
    {
        static double G = 3.023332184e+01;
        static double K = 0.9338478249;

        static double xv[2];
        static double yv[2];

        xv[0] = xv[1];
        xv[1] = (double)data[i] / G;

        yv[0] = yv[1];
        yv[1] = (xv[0] + xv[1]) + (K * yv[0]);

        // Choose filtered/unfiltered data
        buffer[(SAMPLES - STEP) + i] =
            audio.filter? yv[1]: (double)data[i];
    }

    // Give the buffer back
    waveInAddBuffer(audio.hwi, (WAVEHDR *)lParam, sizeof(WAVEHDR));

    // Maximum data value
    static double dmax;

    if (dmax < 4096.0)
        dmax = 4096.0;

    // Calculate normalising value
    double norm = dmax;
    dmax = 0.0;

    // Copy data to FFT input arrays for tuner
    for (int i = 0; i < SAMPLES; i++)
    {
        // Find the magnitude
        if (dmax < fabs(buffer[i]))
            dmax = fabs(buffer[i]);

        // Calculate the window
        double window =
            0.5 - 0.5 * cos(2.0 * M_PI *
                            i / SAMPLES);

        // Normalise and window the input data
        x[i].r = (double)buffer[i] / norm * window;
    }

    // do FFT for tuner
    fftr(x, SAMPLES);

    // Process FFT output for tuner
    for (int i = 1; i < RANGE; i++)
    {
        double real = x[i].r;
        double imag = x[i].i;

        xa[i] = hypot(real, imag);

#ifdef NOISE

        // Do noise cancellation
        xm[i] = (xa[i] + (xm[i] * 19.0)) / 20.0;

        if (xm[i] > xa[i])
            xm[i] = xa[i];

        xd[i] = xa[i] - xm[i];

#endif

        // Do frequency calculation
        double p = atan2(imag, real);
        double dp = xp[i] - p;
        xp[i] = p;

        // Calculate phase difference
        dp -= (double)i * expect;

        int qpd = dp / M_PI;

        if (qpd >= 0)
            qpd += qpd & 1;

        else
            qpd -= qpd & 1;

        dp -=  M_PI * (double)qpd;

        // Calculate frequency difference
        double df = OVERSAMPLE * dp / (2.0 * M_PI);

        // Calculate actual frequency from slot frequency plus
        // frequency difference and correction value
        xf[i] = (i * fps + df * fps);

        // Calculate differences for finding maxima
        dx[i] = xa[i] - xa[i - 1];
    }

    // Downsample
    if (audio.down)
    {
        // x2 = xa << 2
        for (unsigned int i = 0; i < Length(x2); i++)
        {
            x2[i] = 0.0;

            for (int j = 0; j < 2; j++)
                x2[i] += xa[(i * 2) + j];
        }

        // x3 = xa << 3
        for (unsigned int i = 0; i < Length(x3); i++)
        {
            x3[i] = 0.0;

            for (int j = 0; j < 3; j++)
                x3[i] += xa[(i * 3) + j];
        }

        // x4 = xa << 4
        for (unsigned int i = 0; i < Length(x4); i++)
        {
            x4[i] = 0.0;

            for (int j = 0; j < 4; j++)
                x4[i] += xa[(i * 4) + j];
        }

        // x5 = xa << 5
        for (unsigned int i = 0; i < Length(x5); i++)
        {
            x5[i] = 0.0;

            for (int j = 0; j < 5; j++)
                x5[i] += xa[(i * 5) + j];
        }

        // Add downsamples
        for (unsigned int i = 1; i < Length(xa); i++)
        {
            xa[i] *= (i < Length(x2))? x2[i]: 0.0;
            xa[i] *= (i < Length(x3))? x3[i]: 0.0;
            xa[i] *= (i < Length(x4))? x4[i]: 0.0;
            xa[i] *= (i < Length(x5))? x5[i]: 0.0;

            // Recalculate differences
            dx[i] = xa[i] - xa[i - 1];
        }
    }

    // Maximum FFT output
    double max = 0.0;
    double f = 0.0;

    int count = 0;
    int limit = RANGE - 1;

    // Find maximum value, and list of maxima
    for (int i = 1; i < limit; i++)
    {
        // Clear maxima
        maxima[count].f  = 0.0;
        maxima[count].fr = 0.0;
        maxima[count].n  = 0;

        values[count] = 0.0;

        // Cents relative to reference
        double cf = -12.0 * log2(audio.reference / xf[i]);

        // Note number
        int note = round(cf) + C5_OFFSET;

        // Don't use if negative
        if (note < 0)
            continue;

        // Fundamental filter
        if (audio.fund && (count > 0) &&
            ((note % OCTAVE) != (maxima[0].n % OCTAVE)))
            continue;

        // Note filter
        if (audio.note)
        {
            // Get note and octave
            int n = note % OCTAVE;
            int o = note / OCTAVE;

            // Ignore too high
            if (o >= Length(filter.octave))
                continue;

            // Ignore if filtered
            if (!filter.note[n] ||
                !filter.octave[o])
                continue;
        }

        // Find maximum value
        if (xa[i] > max)
        {
            max = xa[i];
            f = xf[i];
        }

        // If display not locked, find maxima and add to list
        if (!display.lock && count < Length(maxima) &&
            xa[i] > MIN && xa[i] > (max / 4.0) &&
            dx[i] > 0.0 && dx[i + 1] < 0.0)
        {
            // Frequency
            maxima[count].f = xf[i];

            // Note number
            maxima[count].n = note;

            // Octave note number
            int n = (note - audio.key + OCTAVE) % OCTAVE;
            // A note number
            int a = (A_OFFSET - audio.key + OCTAVE) % OCTAVE;

            // Temperament ratio
            double temperRatio = temperaments[audio.temperament][n] /
              temperaments[audio.temperament][a];
            // Equal ratio
            double equalRatio = temperaments[EQUAL][n] /
              temperaments[EQUAL][a];

            // Temperament adjustment
            double temperAdjust = temperRatio / equalRatio;

            // Reference note
            maxima[count].fr = audio.reference *
                pow(2.0, round(cf) / 12.0) * temperAdjust;

            // Set limit to octave above
            if (!audio.down && (limit > i * 2))
                limit = i * 2 - 1;

            count++;
        }
    }

    // Reference note frequency and lower and upper limits
    double fr = 0.0;
    double fl = 0.0;
    double fh = 0.0;

    // Note number
    int note = 0;

    // Found flag and cents value
    BOOL found = false;
    double c = 0.0;

    // Do the note and cents calculations
    if (max > MIN)
    {
        found = true;

        // Frequency
        if (!audio.down)
            f = maxima[0].f;

        // Cents relative to reference
        double cf =
            -12.0 * log2(audio.reference / f);

        // Don't count silly values
        if (isnan(cf))
        {
            cf = 0.0;
            found = false;
        }

        // Note number
        note = round(cf) + C5_OFFSET;

        if (note < 0)
            found = false;

        // Octave note number
        int n = (note - audio.key + OCTAVE) % OCTAVE;
        // A note number
        int a = (A_OFFSET - audio.key + OCTAVE) % OCTAVE;

        // Temperament ratio
        double temperRatio = temperaments[audio.temperament][n] /
            temperaments[audio.temperament][a];
        // Equal ratio
        double equalRatio = temperaments[EQUAL][n] /
            temperaments[EQUAL][a];

        // Temperament adjustment
        double temperAdjust = temperRatio / equalRatio;

        // Reference note
        fr = audio.reference * pow(2.0, round(cf) / 12.0) * temperAdjust;

        // Lower and upper freq
        fl = audio.reference * pow(2.0, (round(cf) - 0.55) /
                                   12.0) * temperAdjust;
        fh = audio.reference * pow(2.0, (round(cf) + 0.55) /
                                   12.0) * temperAdjust;

        // Find nearest maximum to reference note
        double df = 1000.0;

        for (int i = 0; i < count; i++)
        {
            if (fabs(maxima[i].f - fr) < df)
            {
                df = fabs(maxima[i].f - fr);
                f = maxima[i].f;
            }
        }

        // Cents relative to reference note
        c = -12.0 * log2(fr / f);

        // Ignore silly values
        if (!isfinite(c))
            c = 0.0;

        // Ignore if not within 50 cents of reference note
        if (fabs(c) > 0.5)
            found = false;
    }

    // If display not locked
    if (!display.lock)
    {
        // Update scope window
        scope.data = data;
        InvalidateRgn(scope.hwnd, NULL, true);

        // Update spectrum window
        for (int i = 0; i < count; i++)
            values[i] = maxima[i].f / fps;

        spectrum.count = count;

        if (found)
        {
            spectrum.f = f  / fps;
            spectrum.r = fr / fps;
            spectrum.l = fl / fps;
            spectrum.h = fh / fps;
        }

        InvalidateRgn(spectrum.hwnd, NULL, true);
    }

    static long timer;

    if (found)
    {
        // If display not locked
        if (!display.lock)
        {
            // Update the display struct
            display.f = f;
            display.fr = fr;
            display.c = c;
            display.n = note;
            display.count = count;

            // Update strobe
            strobe.c = c;

            // Update staff
            staff.n = note;

            // Update meter
            meter.c = c;
        }

        // Update display and staff
        InvalidateRgn(display.hwnd, NULL, true);
        InvalidateRgn(staff.hwnd, NULL, true);

        // Reset count;
        timer = 0;
    }

    else
    {
        // If display not locked
        if (!display.lock)
        {

            if (timer > TIMER_COUNT)
            {
                display.f = 0.0;
                display.fr = 0.0;
                display.c = 0.0;
                display.n = 0;
                display.count = 0;

                // Update strobe
                strobe.c = 0.0;

                // Update staff
                staff.n = 0.0;

                // Update meter
                meter.c = 0.0;

                // Update spectrum
                spectrum.f = 0.0;
                spectrum.r = 0.0;
                spectrum.l = 0.0;
                spectrum.h = 0.0;
            }

            // Update display and staff
            InvalidateRgn(display.hwnd, NULL, true);
            InvalidateRgn(staff.hwnd, NULL, true);
        }
    }

    timer++;
}
