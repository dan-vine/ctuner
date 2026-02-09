////////////////////////////////////////////////////////////////////////////////
//
//  main_window.cpp - Main tuner UI window with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "main_window.h"
#include "imgui.h"
#include <cstdio>
#include <cmath>

namespace ctuner {

MainWindow::MainWindow()
{
}

void MainWindow::initialize()
{
    // Load custom tunings
    m_tuningEditor.loadCustomTunings(m_temperaments, m_customTunings);
}

void MainWindow::render(AppState& state)
{
    // Main window flags - fullscreen docked window
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_MenuBar;

    // Get the viewport and set window position/size
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("CTuner", nullptr, windowFlags);

    renderMenuBar(state);

    // Scope view (waveform)
    renderScopeView(state);

    // Spectrum view
    m_spectrumView.render(state);

    // Main display area (note, frequency, cents)
    renderDisplayArea(state);

    // Strobe or Staff view
    m_strobeView.render(state);
    m_staffView.render(state);

    // Meter view
    m_meterView.render(state);

    // Status bar
    renderStatusBar(state);

    ImGui::End();

    // Render child windows
    m_settingsPanel.render(state, m_temperaments);
    m_tuningEditor.render(state, m_temperaments, m_customTunings);
    m_logViewer.render(state, m_logger);

    // Demo window for debugging
    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }
}

void MainWindow::renderMenuBar(AppState& state)
{
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Settings...", "O")) {
                m_settingsPanel.show();
            }
            if (ImGui::MenuItem("Custom Tunings...")) {
                m_tuningEditor.show();
            }
            ImGui::Separator();
            ImGui::MenuItem("Zoom Spectrum", "Z", &state.spectrumZoom);
            ImGui::MenuItem("Audio Filter", "F", &state.audioFilter);
            ImGui::MenuItem("Downsample", "D", &state.downsample);
            ImGui::MenuItem("Lock Display", "L", &state.displayLock);
            ImGui::MenuItem("Multiple Notes", "M", &state.multipleNotes);
            ImGui::Separator();
            bool strobeEnabled = state.showStrobe;
            if (ImGui::MenuItem("Display Strobe", "S", &strobeEnabled)) {
                state.showStrobe = strobeEnabled;
                state.showStaff = !strobeEnabled;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Logging")) {
            if (!m_logger.isLogging()) {
                if (ImGui::MenuItem("Start Logging")) {
                    m_logger.startSession();
                    state.loggingEnabled = true;
                    m_logViewer.show();  // Auto-show log viewer
                }
            } else {
                if (ImGui::MenuItem("Stop Logging")) {
                    m_logger.stopSession();
                    state.loggingEnabled = false;
                }
            }
            ImGui::Separator();
            bool viewerVisible = m_logViewer.isVisible();
            if (ImGui::MenuItem("Show Log Viewer", "V", &viewerVisible)) {
                if (viewerVisible) m_logViewer.show();
                else m_logViewer.hide();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export to CSV...", nullptr, false,
                               m_logger.getEntryCount() > 0)) {
                std::string filename = m_logger.exportCSVAuto();
                // Could show a message with the filename
            }
            if (ImGui::MenuItem("Clear Log", nullptr, false,
                               m_logger.getEntryCount() > 0)) {
                m_logger.clear();
            }
            ImGui::Separator();
            ImGui::Text("Entries: %d", m_logger.getEntryCount());
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("ImGui Demo")) {
                m_showDemo = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("About CTuner")) {
                // Could show about dialog
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void MainWindow::renderScopeView(AppState& state)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    if (ImGui::BeginChild("Scope", ImVec2(-1, 50), true)) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        // Draw graticule
        ImU32 gridColor = IM_COL32(0, 64, 0, 255);
        for (float x = 5; x < size.x; x += 5) {
            draw->AddLine(ImVec2(pos.x + x, pos.y),
                          ImVec2(pos.x + x, pos.y + size.y), gridColor);
        }
        for (float y = 5; y < size.y; y += 5) {
            draw->AddLine(ImVec2(pos.x, pos.y + y),
                          ImVec2(pos.x + size.x, pos.y + y), gridColor);
        }

        // Draw scope trace
        if (!state.scopeData.empty()) {
            float maxVal = 4096.0f;
            for (const auto& v : state.scopeData) {
                if (std::abs(v) > maxVal) maxVal = static_cast<float>(std::abs(v));
            }

            float yscale = maxVal / (size.y / 2.0f);
            float centerY = pos.y + size.y / 2.0f;

            // Find sync point (zero crossing with positive slope)
            int syncOffset = 0;
            int maxDx = 0;
            for (size_t i = 1; i < state.scopeData.size() && i < static_cast<size_t>(size.x); i++) {
                int dx = state.scopeData[i] - state.scopeData[i - 1];
                if (dx > maxDx) {
                    maxDx = dx;
                    syncOffset = static_cast<int>(i);
                }
                if (maxDx > 0 && dx < 0) break;
            }

            ImVec2 lastPoint(pos.x, centerY);
            for (int x = 0; x < static_cast<int>(size.x) &&
                          (x + syncOffset) < static_cast<int>(state.scopeData.size()); x++) {
                float y = centerY - state.scopeData[x + syncOffset] / yscale;
                ImVec2 point(pos.x + x, y);
                draw->AddLine(lastPoint, point, IM_COL32(0, 255, 0, 255));
                lastPoint = point;
            }
        }

        // Show F if filtered
        if (state.audioFilter) {
            draw->AddText(ImVec2(pos.x + 2, pos.y + size.y - 12),
                          IM_COL32(255, 255, 0, 255), "F");
        }

        // Handle click to toggle filter
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            state.audioFilter = !state.audioFilter;
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleColor();
}

void MainWindow::renderDisplayArea(AppState& state)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    if (ImGui::BeginChild("Display", ImVec2(-1, 120), true)) {
        const PitchResult& pitch = state.currentPitch;

        if (state.multipleNotes && state.maximaCount > 0) {
            // Multiple notes mode
            ImGui::Text("Multiple notes detected:");
            for (int i = 0; i < state.maximaCount && i < static_cast<int>(state.maxima.size()); i++) {
                const Maximum& m = state.maxima[i];
                if (m.frequency > 0) {
                    double c = -12.0 * std::log2(m.refFrequency / m.frequency);
                    if (std::isfinite(c)) {
                        int note = (m.note - state.transpose + OCTAVE) % OCTAVE;
                        ImGui::Text("%s%d  %.2f Hz  %+.2f cents",
                                   NOTE_NAMES[note], m.note / OCTAVE,
                                   m.frequency, c * 100.0);
                    }
                }
            }
        } else {
            // Single note display
            if (pitch.valid) {
                int displayNote = (pitch.note - state.transpose + OCTAVE) % OCTAVE;
                int displayOctave = pitch.note / OCTAVE;

                // Large note display
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);  // Would use larger font
                ImGui::SetWindowFontScale(3.0f);
                ImGui::Text("%s%d", DISPLAY_NOTES[displayNote], displayOctave);
                ImGui::SameLine();
                ImGui::SetWindowFontScale(1.5f);
                ImGui::Text("%s", DISPLAY_SHARPS[displayNote]);
                ImGui::SameLine();
                ImGui::SetWindowFontScale(2.0f);
                ImGui::Text("  %+.2fc", pitch.cents * 100.0);
                ImGui::SetWindowFontScale(1.0f);
                ImGui::PopFont();

                // Frequency details
                ImGui::Text("Reference: %.2f Hz", pitch.refFrequency);
                ImGui::SameLine(200);
                ImGui::Text("Detected: %.2f Hz", pitch.frequency);

                ImGui::Text("Tuning ref: %.2f Hz", state.referenceFrequency);
                ImGui::SameLine(200);
                ImGui::Text("Difference: %+.2f Hz", pitch.frequency - pitch.refFrequency);
            } else {
                ImGui::SetWindowFontScale(2.0f);
                ImGui::Text("--");
                ImGui::SetWindowFontScale(1.0f);
                ImGui::Text("No pitch detected");
            }
        }

        // Show lock icon if locked
        if (state.displayLock) {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddText(ImVec2(pos.x + 2, pos.y - 14),
                          IM_COL32(0, 0, 0, 255), "[LOCKED]");
        }

        // Handle click to toggle lock
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            state.displayLock = !state.displayLock;
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleColor(2);
}

void MainWindow::renderStatusBar(AppState& state)
{
    ImGui::Separator();
    ImGui::Text("Temperament: %s", m_temperaments.getName(state.currentTemperament).c_str());
    ImGui::SameLine(200);
    ImGui::Text("Ref: %.2f Hz", state.referenceFrequency);
    ImGui::SameLine(350);
    if (state.loggingEnabled) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "LOGGING");
    }
}

} // namespace ctuner
