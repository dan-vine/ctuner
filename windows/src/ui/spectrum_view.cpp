////////////////////////////////////////////////////////////////////////////////
//
//  spectrum_view.cpp - Spectrum visualization with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "spectrum_view.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>

namespace ctuner {

SpectrumView::SpectrumView()
{
    m_spectrum.resize(RANGE, 0.0);
    m_maxima.resize(MAXIMA);
}

void SpectrumView::updateData(const std::vector<double>& spectrum,
                               float freq, float ref, float low, float high,
                               const std::vector<Maximum>& maxima, int count)
{
    if (!spectrum.empty()) {
        m_spectrum = spectrum;

        // Update max value for scaling
        for (const auto& v : m_spectrum) {
            if (v > m_maxValue) m_maxValue = static_cast<float>(v);
        }
        // Decay max value slowly
        m_maxValue *= 0.99f;
        if (m_maxValue < 1.0f) m_maxValue = 1.0f;
    }

    m_freq = freq;
    m_ref = ref;
    m_low = low;
    m_high = high;
    m_maxima = maxima;
    m_maximaCount = count;
}

void SpectrumView::render(AppState& state)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    if (ImGui::BeginChild("Spectrum", ImVec2(-1, 60), true)) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        // Draw graticule
        drawGraticule(size.x, size.y);

        if (state.spectrumZoom && m_ref > 0) {
            renderZoomedSpectrum(state);
        } else {
            renderFullSpectrum(state);
        }

        // Handle click to toggle zoom
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            state.spectrumZoom = !state.spectrumZoom;
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleColor();
}

void SpectrumView::renderZoomedSpectrum(AppState& state)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    float xscale = (size.x / (m_ref - m_low)) / 2.0f;
    float yscale = size.y / m_maxValue;

    // Draw spectrum
    ImVec2 lastPoint(pos.x, pos.y + size.y);

    int lowIdx = std::max(1, static_cast<int>(std::floor(m_low)));
    int highIdx = std::min(static_cast<int>(m_spectrum.size()) - 1,
                          static_cast<int>(std::ceil(m_high)));

    for (int i = lowIdx; i <= highIdx; i++) {
        float x = (i - m_low) * xscale;
        float y = size.y - static_cast<float>(m_spectrum[i]) * yscale;

        ImVec2 point(pos.x + x, pos.y + y);
        draw->AddLine(lastPoint, point, IM_COL32(0, 255, 0, 255));
        lastPoint = point;
    }

    // Draw reference line (center)
    float centerX = pos.x + size.x / 2.0f;
    draw->AddLine(ImVec2(centerX, pos.y), ImVec2(centerX, pos.y + size.y),
                  IM_COL32(0, 255, 0, 255));

    // Draw maxima markers
    ImU32 yellow = IM_COL32(255, 255, 0, 255);
    for (int i = 0; i < m_maximaCount; i++) {
        float freqBin = static_cast<float>(m_maxima[i].frequency) /
                        (static_cast<float>(SAMPLE_RATE) / static_cast<float>(SAMPLES));

        if (freqBin > m_low && freqBin < m_high) {
            float x = pos.x + (freqBin - m_low) * xscale;
            draw->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + size.y), yellow);

            // Show cents deviation
            double c = -12.0 * std::log2(m_maxima[i].refFrequency / m_maxima[i].frequency);
            if (std::isfinite(c)) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%+.0f", c * 100.0);
                draw->AddText(ImVec2(x - 10, pos.y + 2), yellow, buf);
            }
        }
    }
}

void SpectrumView::renderFullSpectrum(AppState& state)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    int displayRange = static_cast<int>(m_spectrum.size()) / state.spectrumExpand;
    float xscale = std::log(static_cast<float>(displayRange)) / size.x;
    float yscale = size.y / m_maxValue;

    // Draw spectrum with log frequency scale
    ImVec2 lastPoint(pos.x, pos.y + size.y);
    int lastIdx = 1;

    for (int x = 0; x < static_cast<int>(size.x); x++) {
        float value = 0.0f;
        int idx = static_cast<int>(std::round(std::exp(x * xscale)));

        for (int i = lastIdx; i <= idx && i < static_cast<int>(m_spectrum.size()); i++) {
            if (i > 0 && m_spectrum[i] > value) {
                value = static_cast<float>(m_spectrum[i]);
            }
        }
        lastIdx = idx;

        float y = size.y - value * yscale;
        ImVec2 point(pos.x + x, pos.y + y);
        draw->AddLine(lastPoint, point, IM_COL32(0, 255, 0, 255));
        lastPoint = point;
    }

    // Draw maxima markers
    ImU32 yellow = IM_COL32(255, 255, 0, 255);
    for (int i = 0; i < m_maximaCount; i++) {
        float freqBin = static_cast<float>(m_maxima[i].frequency) /
                        (static_cast<float>(SAMPLE_RATE) / static_cast<float>(SAMPLES));

        if (freqBin > 1 && freqBin < displayRange) {
            float x = pos.x + std::log(freqBin) / xscale;
            draw->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + size.y), yellow);
        }
    }

    // Show expand factor if > 1
    if (state.spectrumExpand > 1) {
        char buf[16];
        snprintf(buf, sizeof(buf), "x%d", state.spectrumExpand);
        draw->AddText(ImVec2(pos.x + 2, pos.y + 2), IM_COL32(255, 255, 0, 255), buf);
    }

    // Show D if downsampling
    if (state.downsample) {
        draw->AddText(ImVec2(pos.x + 2, pos.y + size.y - 12),
                      IM_COL32(255, 255, 0, 255), "D");
    }
}

void SpectrumView::drawGraticule(float width, float height)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImU32 color = IM_COL32(0, 64, 0, 255);

    // Vertical lines
    for (float x = 5; x < width; x += 5) {
        draw->AddLine(ImVec2(pos.x + x, pos.y),
                      ImVec2(pos.x + x, pos.y + height), color);
    }

    // Horizontal lines
    for (float y = 5; y < height; y += 5) {
        draw->AddLine(ImVec2(pos.x, pos.y + y),
                      ImVec2(pos.x + width, pos.y + y), color);
    }
}

} // namespace ctuner
