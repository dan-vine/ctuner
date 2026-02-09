////////////////////////////////////////////////////////////////////////////////
//
//  app_state.h - Centralized application state
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef APP_STATE_H
#define APP_STATE_H

#include <vector>
#include <string>
#include <array>
#include <chrono>

namespace ctuner {

// Audio processing constants
constexpr int SAMPLE_RATE = 11025;
constexpr int BITS_PER_SAMPLE = 16;
constexpr int CHANNELS = 1;
constexpr int SAMPLES = 16384;
constexpr int OVERSAMPLE = 16;
constexpr int STEP = SAMPLES / OVERSAMPLE;
constexpr int RANGE = SAMPLES * 7 / 16;
constexpr int MAXIMA = 8;

// Reference values
constexpr double A5_REFERENCE = 440.0;
constexpr int C5_OFFSET = 57;
constexpr int A_OFFSET = 9;
constexpr int OCTAVE = 12;
constexpr int EQUAL_TEMPERAMENT = 8;

// Strobe colors
enum class StrobeColor {
    Blue,
    Olive,
    Magenta
};

// Note names
constexpr const char* NOTE_NAMES[] = {
    "C", "C#", "D", "Eb", "E", "F",
    "F#", "G", "Ab", "A", "Bb", "B"
};

// Display note names (with natural symbols)
constexpr const char* DISPLAY_NOTES[] = {
    "C", "C", "D", "E", "E", "F",
    "F", "G", "A", "A", "B", "B"
};

constexpr const char* DISPLAY_SHARPS[] = {
    "", "#", "", "b", "", "",
    "#", "", "b", "", "b", ""
};

// Peak/maximum information
struct Maximum {
    double frequency = 0.0;     // Detected frequency
    double refFrequency = 0.0;  // Reference frequency for note
    int note = 0;               // Note number (C0 = 0)
};

// Pitch detection result
struct PitchResult {
    double frequency = 0.0;     // Detected frequency in Hz
    double refFrequency = 0.0;  // Reference frequency for closest note
    double cents = 0.0;         // Cents deviation from reference
    int note = 0;               // Note number (0-based, C0 = 0)
    int octave = 0;             // Octave number
    double confidence = 0.0;    // Detection confidence (0-1)
    bool valid = false;         // Whether detection is valid

    int noteInOctave() const { return note % OCTAVE; }
    const char* noteName() const { return NOTE_NAMES[noteInOctave()]; }
};

// Log entry for frequency logging
struct LogEntry {
    double timestamp = 0.0;   // Seconds since session start
    double frequency = 0.0;   // Detected frequency
    int note = 0;             // Note number
    int octave = 0;           // Octave number
    double cents = 0.0;       // Cents deviation
};

// Filter settings for notes and octaves
struct FilterSettings {
    std::array<bool, 12> notes = {true, true, true, true, true, true,
                                   true, true, true, true, true, true};
    std::array<bool, 9> octaves = {true, true, true, true, true, true,
                                    true, true, true};
};

// Centralized application state
struct AppState {
    // Audio state
    PitchResult currentPitch;
    std::vector<double> spectrumData;
    std::vector<short> scopeData;
    std::vector<Maximum> maxima;
    int maximaCount = 0;
    bool audioRunning = false;

    // Spectrum view state
    float spectrumFreq = 0.0f;      // Current frequency bin
    float spectrumRef = 0.0f;       // Reference frequency bin
    float spectrumLow = 0.0f;       // Low frequency bound
    float spectrumHigh = 0.0f;      // High frequency bound
    int spectrumExpand = 1;         // Expansion factor (1, 2, 4, 8, 16)
    bool spectrumZoom = true;       // Zoom to detected note

    // Tuning settings
    double referenceFrequency = A5_REFERENCE;
    int currentTemperament = EQUAL_TEMPERAMENT;
    int key = 0;                    // Key for temperament (0 = C)
    int transpose = 0;              // Transpose display (-6 to +6)

    // Audio processing options
    bool audioFilter = false;       // Butterworth filter
    bool downsample = false;        // Harmonic product spectrum
    bool fundamentalFilter = false; // Fundamental frequency filter
    bool noteFilter = false;        // Note/octave filter
    FilterSettings filters;

    // Display options
    bool displayLock = false;       // Lock display values
    bool multipleNotes = false;     // Show multiple notes
    bool showSpectrum = true;
    bool showStrobe = false;
    bool showStaff = true;
    bool showMeter = true;

    // Strobe settings
    StrobeColor strobeColor = StrobeColor::Olive;

    // Logging
    bool loggingEnabled = false;
    std::vector<LogEntry> frequencyLog;
    std::chrono::steady_clock::time_point sessionStart;

    // Settings window
    bool showSettings = false;
    bool showTuningEditor = false;
    bool showFilters = false;

    // Helper functions
    void toggleStrobe() {
        showStrobe = !showStrobe;
        showStaff = !showStrobe;
    }

    void resetLog() {
        frequencyLog.clear();
        sessionStart = std::chrono::steady_clock::now();
    }

    void addLogEntry(const PitchResult& pitch) {
        if (!loggingEnabled || !pitch.valid) return;

        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - sessionStart).count();

        frequencyLog.push_back({
            elapsed,
            pitch.frequency,
            pitch.note,
            pitch.octave,
            pitch.cents
        });
    }
};

} // namespace ctuner

#endif // APP_STATE_H
