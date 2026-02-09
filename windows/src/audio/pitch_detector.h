////////////////////////////////////////////////////////////////////////////////
//
//  pitch_detector.h - Pitch detection algorithm
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

#ifndef PITCH_DETECTOR_H
#define PITCH_DETECTOR_H

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "../app_state.h"
#include "fft.h"
#include <vector>
#include <functional>

namespace ctuner {

class PitchDetector {
public:
    // Callback for pitch detection results
    using ResultCallback = std::function<void(const PitchResult&,
                                               const std::vector<double>&,
                                               const std::vector<Maximum>&,
                                               int)>;

    PitchDetector();
    ~PitchDetector() = default;

    // Process a buffer of audio samples
    void processBuffer(const short* samples, int count);

    // Get the latest result
    const PitchResult& getResult() const { return m_result; }

    // Get spectrum data
    const std::vector<double>& getSpectrumData() const { return m_amplitude; }

    // Get detected maxima
    const std::vector<Maximum>& getMaxima() const { return m_maxima; }
    int getMaximaCount() const { return m_maximaCount; }

    // Get spectrum display values
    float getFrequencyBin() const { return m_freqBin; }
    float getReferenceBin() const { return m_refBin; }
    float getLowBin() const { return m_lowBin; }
    float getHighBin() const { return m_highBin; }

    // Settings
    void setReference(double freq) { m_reference = freq; }
    double getReference() const { return m_reference; }

    void setTemperament(int index) { m_temperament = index; }
    int getTemperament() const { return m_temperament; }

    void setKey(int key) { m_key = key; }
    int getKey() const { return m_key; }

    void setFilter(bool enable) { m_filter = enable; }
    bool getFilter() const { return m_filter; }

    void setDownsample(bool enable) { m_downsample = enable; }
    bool getDownsample() const { return m_downsample; }

    void setFundamental(bool enable) { m_fundamental = enable; }
    bool getFundamental() const { return m_fundamental; }

    void setNoteFilter(bool enable) { m_noteFilter = enable; }
    bool getNoteFilter() const { return m_noteFilter; }

    void setFilterSettings(const FilterSettings& settings) { m_filterSettings = settings; }
    const FilterSettings& getFilterSettings() const { return m_filterSettings; }

    // Set callback for results
    void setCallback(ResultCallback callback) { m_callback = callback; }

    // Access to temperament data
    static const double temperaments[32][12];

private:
    void applyButterworthFilter(const short* samples, int count);
    void performFFT();
    void findMaxima();
    void calculatePitch();
    double getTemperamentRatio(int note) const;

    // Audio buffer
    std::vector<double> m_buffer;

    // FFT data
    std::vector<Complex> m_fftData;
    std::vector<double> m_amplitude;
    std::vector<double> m_phase;
    std::vector<double> m_prevPhase;
    std::vector<double> m_frequency;
    std::vector<double> m_derivative;

    // Downsampling buffers
    std::vector<double> m_ds2;
    std::vector<double> m_ds3;
    std::vector<double> m_ds4;
    std::vector<double> m_ds5;

    // Detected peaks
    std::vector<Maximum> m_maxima;
    std::vector<double> m_maximaValues;
    int m_maximaCount = 0;

    // Spectrum display values
    float m_freqBin = 0.0f;
    float m_refBin = 0.0f;
    float m_lowBin = 0.0f;
    float m_highBin = 0.0f;

    // Result
    PitchResult m_result;

    // Settings
    double m_reference = A5_REFERENCE;
    int m_temperament = EQUAL_TEMPERAMENT;
    int m_key = 0;
    bool m_filter = false;
    bool m_downsample = false;
    bool m_fundamental = false;
    bool m_noteFilter = false;
    FilterSettings m_filterSettings;

    // Butterworth filter state
    double m_xv[2] = {0.0, 0.0};
    double m_yv[2] = {0.0, 0.0};

    // Maximum amplitude for normalization
    double m_dmax = 4096.0;

    // Callback
    ResultCallback m_callback;

    // Constants
    static constexpr double MIN_AMPLITUDE = 0.5;
    static constexpr double FPS = static_cast<double>(SAMPLE_RATE) / static_cast<double>(SAMPLES);
    static constexpr double EXPECT = 2.0 * M_PI * static_cast<double>(STEP) / static_cast<double>(SAMPLES);
};

} // namespace ctuner

#endif // PITCH_DETECTOR_H
