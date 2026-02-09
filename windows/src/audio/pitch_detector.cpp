////////////////////////////////////////////////////////////////////////////////
//
//  pitch_detector.cpp - Pitch detection algorithm
//
//  Copyright (C) 2009  Bill Farmer
//  Refactored 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "pitch_detector.h"
#include "../tuning/built_in_temperaments.h"
#include <cstring>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ctuner {

// Temperament data (imported from built_in_temperaments.h)
const double PitchDetector::temperaments[32][12] = BUILT_IN_TEMPERAMENTS;

PitchDetector::PitchDetector()
    : m_buffer(SAMPLES, 0.0)
    , m_fftData(SAMPLES)
    , m_amplitude(RANGE, 0.0)
    , m_phase(RANGE, 0.0)
    , m_prevPhase(RANGE, 0.0)
    , m_frequency(RANGE, 0.0)
    , m_derivative(RANGE, 0.0)
    , m_ds2(RANGE / 2, 0.0)
    , m_ds3(RANGE / 3, 0.0)
    , m_ds4(RANGE / 4, 0.0)
    , m_ds5(RANGE / 5, 0.0)
    , m_maxima(MAXIMA)
    , m_maximaValues(MAXIMA, 0.0)
{
}

void PitchDetector::processBuffer(const short* samples, int count)
{
    // Apply Butterworth filter if enabled
    applyButterworthFilter(samples, count);

    // Perform FFT analysis
    performFFT();

    // Find peaks in spectrum
    findMaxima();

    // Calculate pitch from peaks
    calculatePitch();

    // Notify callback if set
    if (m_callback) {
        m_callback(m_result, m_amplitude, m_maxima, m_maximaCount);
    }
}

void PitchDetector::applyButterworthFilter(const short* samples, int count)
{
    // Shift existing buffer data
    std::memmove(m_buffer.data(), m_buffer.data() + STEP,
                 (SAMPLES - STEP) * sizeof(double));

    // Butterworth filter coefficients, 3dB/octave
    constexpr double G = 3.023332184e+01;
    constexpr double K = 0.9338478249;

    // Process new samples
    for (int i = 0; i < count && i < STEP; i++) {
        if (m_filter) {
            m_xv[0] = m_xv[1];
            m_xv[1] = static_cast<double>(samples[i]) / G;

            m_yv[0] = m_yv[1];
            m_yv[1] = (m_xv[0] + m_xv[1]) + (K * m_yv[0]);

            m_buffer[(SAMPLES - STEP) + i] = m_yv[1];
        } else {
            m_buffer[(SAMPLES - STEP) + i] = static_cast<double>(samples[i]);
        }
    }
}

void PitchDetector::performFFT()
{
    // Track maximum for normalization
    if (m_dmax < 4096.0)
        m_dmax = 4096.0;

    double norm = m_dmax;
    m_dmax = 0.0;

    // Copy data to FFT input with window function
    for (int i = 0; i < SAMPLES; i++) {
        // Find maximum amplitude
        if (m_dmax < std::fabs(m_buffer[i]))
            m_dmax = std::fabs(m_buffer[i]);

        // Hann window
        double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / SAMPLES);

        // Normalize and window
        m_fftData[i].r = m_buffer[i] / norm * window;
        m_fftData[i].i = 0.0;
    }

    // Perform FFT
    fftr(m_fftData);

    // Process FFT output
    for (int i = 1; i < RANGE; i++) {
        double real = m_fftData[i].r;
        double imag = m_fftData[i].i;

        // Calculate amplitude
        m_amplitude[i] = std::hypot(real, imag);

        // Calculate phase and frequency using phase vocoder technique
        double p = std::atan2(imag, real);
        double dp = m_prevPhase[i] - p;
        m_prevPhase[i] = p;

        // Calculate phase difference
        dp -= static_cast<double>(i) * EXPECT;

        int qpd = static_cast<int>(dp / M_PI);
        if (qpd >= 0)
            qpd += qpd & 1;
        else
            qpd -= qpd & 1;

        dp -= M_PI * static_cast<double>(qpd);

        // Calculate frequency difference
        double df = OVERSAMPLE * dp / (2.0 * M_PI);

        // Calculate actual frequency
        m_frequency[i] = i * FPS + df * FPS;

        // Calculate derivative for peak detection
        m_derivative[i] = m_amplitude[i] - m_amplitude[i - 1];
    }

    // Apply harmonic product spectrum if downsampling enabled
    if (m_downsample) {
        // Downsample x2
        for (size_t i = 0; i < m_ds2.size(); i++) {
            m_ds2[i] = 0.0;
            for (int j = 0; j < 2; j++)
                m_ds2[i] += m_amplitude[(i * 2) + j];
        }

        // Downsample x3
        for (size_t i = 0; i < m_ds3.size(); i++) {
            m_ds3[i] = 0.0;
            for (int j = 0; j < 3; j++)
                m_ds3[i] += m_amplitude[(i * 3) + j];
        }

        // Downsample x4
        for (size_t i = 0; i < m_ds4.size(); i++) {
            m_ds4[i] = 0.0;
            for (int j = 0; j < 4; j++)
                m_ds4[i] += m_amplitude[(i * 4) + j];
        }

        // Downsample x5
        for (size_t i = 0; i < m_ds5.size(); i++) {
            m_ds5[i] = 0.0;
            for (int j = 0; j < 5; j++)
                m_ds5[i] += m_amplitude[(i * 5) + j];
        }

        // Multiply downsampled spectra
        for (size_t i = 1; i < m_amplitude.size(); i++) {
            m_amplitude[i] *= (i < m_ds2.size()) ? m_ds2[i] : 0.0;
            m_amplitude[i] *= (i < m_ds3.size()) ? m_ds3[i] : 0.0;
            m_amplitude[i] *= (i < m_ds4.size()) ? m_ds4[i] : 0.0;
            m_amplitude[i] *= (i < m_ds5.size()) ? m_ds5[i] : 0.0;

            // Recalculate derivative
            m_derivative[i] = m_amplitude[i] - m_amplitude[i - 1];
        }
    }
}

void PitchDetector::findMaxima()
{
    double maxAmp = 0.0;
    double maxFreq = 0.0;

    int count = 0;
    int limit = RANGE - 1;

    // Clear maxima
    for (int i = 0; i < MAXIMA; i++) {
        m_maxima[i] = Maximum();
        m_maximaValues[i] = 0.0;
    }

    // Find maximum value and list of peaks
    for (int i = 1; i < limit; i++) {
        // Calculate cents relative to reference
        double cf = -12.0 * std::log2(m_reference / m_frequency[i]);

        // Note number
        int note = static_cast<int>(std::round(cf)) + C5_OFFSET;

        // Skip negative notes
        if (note < 0)
            continue;

        // Apply fundamental filter
        if (m_fundamental && count > 0 &&
            (note % OCTAVE) != (m_maxima[0].note % OCTAVE))
            continue;

        // Apply note/octave filter
        if (m_noteFilter) {
            int n = note % OCTAVE;
            int o = note / OCTAVE;

            if (o >= static_cast<int>(m_filterSettings.octaves.size()))
                continue;

            if (!m_filterSettings.notes[n] || !m_filterSettings.octaves[o])
                continue;
        }

        // Track maximum
        if (m_amplitude[i] > maxAmp) {
            maxAmp = m_amplitude[i];
            maxFreq = m_frequency[i];
        }

        // Find peaks (local maxima above threshold)
        if (count < MAXIMA &&
            m_amplitude[i] > MIN_AMPLITUDE &&
            m_amplitude[i] > (maxAmp / 4.0) &&
            m_derivative[i] > 0.0 &&
            m_derivative[i + 1] < 0.0)
        {
            m_maxima[count].frequency = m_frequency[i];
            m_maxima[count].note = note;

            // Calculate temperament-adjusted reference
            int n = (note - m_key + OCTAVE) % OCTAVE;
            int a = (A_OFFSET - m_key + OCTAVE) % OCTAVE;

            double temperRatio = temperaments[m_temperament][n] /
                                temperaments[m_temperament][a];
            double equalRatio = temperaments[EQUAL_TEMPERAMENT][n] /
                               temperaments[EQUAL_TEMPERAMENT][a];
            double temperAdjust = temperRatio / equalRatio;

            m_maxima[count].refFrequency = m_reference *
                std::pow(2.0, std::round(cf) / 12.0) * temperAdjust;

            // Set search limit to one octave above if not downsampling
            if (!m_downsample && limit > i * 2)
                limit = i * 2 - 1;

            count++;
        }
    }

    m_maximaCount = count;
}

void PitchDetector::calculatePitch()
{
    m_result = PitchResult();

    if (m_maximaCount == 0) {
        m_freqBin = 0.0f;
        m_refBin = 0.0f;
        m_lowBin = 0.0f;
        m_highBin = 0.0f;
        return;
    }

    // Use first maximum (fundamental)
    double f = m_maxima[0].frequency;
    double fr = m_maxima[0].refFrequency;

    // Calculate cents relative to reference
    double cf = -12.0 * std::log2(m_reference / f);

    if (std::isnan(cf)) {
        return;
    }

    // Note number
    int note = static_cast<int>(std::round(cf)) + C5_OFFSET;
    if (note < 0) {
        return;
    }

    // Temperament adjustment
    int n = (note - m_key + OCTAVE) % OCTAVE;
    int a = (A_OFFSET - m_key + OCTAVE) % OCTAVE;

    double temperRatio = temperaments[m_temperament][n] /
                        temperaments[m_temperament][a];
    double equalRatio = temperaments[EQUAL_TEMPERAMENT][n] /
                       temperaments[EQUAL_TEMPERAMENT][a];
    double temperAdjust = temperRatio / equalRatio;

    // Reference frequency for this note
    fr = m_reference * std::pow(2.0, std::round(cf) / 12.0) * temperAdjust;

    // Lower and upper bounds
    double fl = m_reference * std::pow(2.0, (std::round(cf) - 0.55) / 12.0) * temperAdjust;
    double fh = m_reference * std::pow(2.0, (std::round(cf) + 0.55) / 12.0) * temperAdjust;

    // Find nearest maximum to reference
    double df = 1000.0;
    for (int i = 0; i < m_maximaCount; i++) {
        if (std::fabs(m_maxima[i].frequency - fr) < df) {
            df = std::fabs(m_maxima[i].frequency - fr);
            f = m_maxima[i].frequency;
        }
    }

    // Cents relative to reference note
    double c = -12.0 * std::log2(fr / f);

    if (!std::isfinite(c))
        c = 0.0;

    // Check if within 50 cents
    if (std::fabs(c) > 0.5) {
        return;
    }

    // Fill result
    m_result.frequency = f;
    m_result.refFrequency = fr;
    m_result.cents = c;
    m_result.note = note;
    m_result.octave = note / OCTAVE;
    m_result.confidence = 1.0;  // Could be improved with amplitude-based confidence
    m_result.valid = true;

    // Update spectrum display values
    m_freqBin = static_cast<float>(f / FPS);
    m_refBin = static_cast<float>(fr / FPS);
    m_lowBin = static_cast<float>(fl / FPS);
    m_highBin = static_cast<float>(fh / FPS);
}

double PitchDetector::getTemperamentRatio(int note) const
{
    int n = (note - m_key + OCTAVE) % OCTAVE;
    return temperaments[m_temperament][n];
}

} // namespace ctuner
