////////////////////////////////////////////////////////////////////////////////
//
//  audio_capture.h - Windows audio input capture
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

#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <windows.h>
#include <mmsystem.h>
#include <functional>
#include <atomic>
#include <thread>
#include <vector>

namespace ctuner {

class AudioCapture {
public:
    // Callback for audio data
    using DataCallback = std::function<void(const short* samples, int count)>;

    // Audio parameters
    static constexpr int SAMPLE_RATE = 11025;
    static constexpr int BITS_PER_SAMPLE = 16;
    static constexpr int CHANNELS = 1;
    static constexpr int BLOCK_ALIGN = 2;
    static constexpr int BUFFER_SIZE = 1024;  // STEP from original
    static constexpr int NUM_BUFFERS = 4;

    AudioCapture();
    ~AudioCapture();

    // Start/stop audio capture
    bool start();
    void stop();

    // Check if running
    bool isRunning() const { return m_running; }

    // Set data callback
    void setCallback(DataCallback callback) { m_callback = callback; }

    // Get last error message
    const char* getLastError() const { return m_lastError; }

    // Get scope data (copy of last buffer for display)
    const std::vector<short>& getScopeData() const { return m_scopeData; }

private:
    // Audio thread function
    void audioThreadFunc();

    // Process incoming audio data
    void processWaveData(WPARAM wParam, LPARAM lParam);

    // Wave input handle
    HWAVEIN m_hwi = nullptr;

    // Thread
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
    DWORD m_threadId = 0;

    // Buffers
    std::vector<short> m_bufferData[NUM_BUFFERS];
    WAVEHDR m_headers[NUM_BUFFERS];

    // Scope data for display
    std::vector<short> m_scopeData;

    // Callback
    DataCallback m_callback;

    // Error message
    char m_lastError[256] = {0};
};

} // namespace ctuner

#endif // AUDIO_CAPTURE_H
