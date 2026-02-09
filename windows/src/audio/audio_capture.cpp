////////////////////////////////////////////////////////////////////////////////
//
//  audio_capture.cpp - Windows audio input capture
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

#include "audio_capture.h"
#include <cstring>

namespace ctuner {

AudioCapture::AudioCapture()
{
    // Initialize buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        m_bufferData[i].resize(BUFFER_SIZE);
        std::memset(&m_headers[i], 0, sizeof(WAVEHDR));
    }
    m_scopeData.resize(BUFFER_SIZE);
}

AudioCapture::~AudioCapture()
{
    stop();
}

bool AudioCapture::start()
{
    if (m_running) {
        return true;  // Already running
    }

    m_stopRequested = false;
    m_running = true;

    // Start audio thread
    m_thread = std::thread(&AudioCapture::audioThreadFunc, this);

    return true;
}

void AudioCapture::stop()
{
    if (!m_running) {
        return;
    }

    m_stopRequested = true;

    // Post quit message to audio thread
    if (m_threadId != 0) {
        PostThreadMessage(m_threadId, WM_QUIT, 0, 0);
    }

    // Wait for thread to finish
    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_running = false;
}

void AudioCapture::audioThreadFunc()
{
    m_threadId = GetCurrentThreadId();

    // Create wave format structure
    WAVEFORMATEX wf = {};
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = CHANNELS;
    wf.nSamplesPerSec = SAMPLE_RATE;
    wf.nAvgBytesPerSec = SAMPLE_RATE * BLOCK_ALIGN;
    wf.nBlockAlign = BLOCK_ALIGN;
    wf.wBitsPerSample = BITS_PER_SAMPLE;
    wf.cbSize = 0;

    // Open wave input device
    MMRESULT mmr = waveInOpen(&m_hwi, WAVE_MAPPER | WAVE_FORMAT_DIRECT, &wf,
                               m_threadId, 0, CALLBACK_THREAD);

    if (mmr != MMSYSERR_NOERROR) {
        waveInGetErrorTextA(mmr, m_lastError, sizeof(m_lastError));
        m_running = false;
        return;
    }

    // Prepare and add buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        m_headers[i].lpData = reinterpret_cast<LPSTR>(m_bufferData[i].data());
        m_headers[i].dwBufferLength = BUFFER_SIZE * sizeof(short);
        m_headers[i].dwFlags = 0;

        mmr = waveInPrepareHeader(m_hwi, &m_headers[i], sizeof(WAVEHDR));
        if (mmr != MMSYSERR_NOERROR) {
            waveInGetErrorTextA(mmr, m_lastError, sizeof(m_lastError));
            waveInClose(m_hwi);
            m_hwi = nullptr;
            m_running = false;
            return;
        }

        mmr = waveInAddBuffer(m_hwi, &m_headers[i], sizeof(WAVEHDR));
        if (mmr != MMSYSERR_NOERROR) {
            waveInGetErrorTextA(mmr, m_lastError, sizeof(m_lastError));
            waveInClose(m_hwi);
            m_hwi = nullptr;
            m_running = false;
            return;
        }
    }

    // Start recording
    mmr = waveInStart(m_hwi);
    if (mmr != MMSYSERR_NOERROR) {
        waveInGetErrorTextA(mmr, m_lastError, sizeof(m_lastError));
        waveInClose(m_hwi);
        m_hwi = nullptr;
        m_running = false;
        return;
    }

    // Message loop
    MSG msg;
    while (!m_stopRequested) {
        BOOL result = GetMessage(&msg, reinterpret_cast<HWND>(-1), 0, 0);

        if (result == 0 || result == -1) {
            break;
        }

        switch (msg.message) {
            case MM_WIM_OPEN:
                // Wave input opened
                break;

            case MM_WIM_DATA:
                processWaveData(msg.wParam, msg.lParam);
                break;

            case MM_WIM_CLOSE:
                // Wave input closed
                break;
        }
    }

    // Clean up
    waveInStop(m_hwi);
    waveInReset(m_hwi);

    for (int i = 0; i < NUM_BUFFERS; i++) {
        waveInUnprepareHeader(m_hwi, &m_headers[i], sizeof(WAVEHDR));
    }

    waveInClose(m_hwi);
    m_hwi = nullptr;
    m_running = false;
}

void AudioCapture::processWaveData(WPARAM wParam, LPARAM lParam)
{
    WAVEHDR* header = reinterpret_cast<WAVEHDR*>(lParam);
    short* data = reinterpret_cast<short*>(header->lpData);
    int count = header->dwBytesRecorded / sizeof(short);

    // Copy to scope data for display
    if (count <= static_cast<int>(m_scopeData.size())) {
        std::memcpy(m_scopeData.data(), data, count * sizeof(short));
    }

    // Call callback if set
    if (m_callback) {
        m_callback(data, count);
    }

    // Return buffer for reuse
    if (!m_stopRequested && m_hwi != nullptr) {
        waveInAddBuffer(m_hwi, header, sizeof(WAVEHDR));
    }
}

} // namespace ctuner
