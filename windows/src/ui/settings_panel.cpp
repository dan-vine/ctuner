////////////////////////////////////////////////////////////////////////////////
//
//  settings_panel.cpp - Options/settings panel with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "settings_panel.h"
#include "imgui.h"
#include <cstdio>

namespace ctuner {

SettingsPanel::SettingsPanel()
{
    snprintf(m_refBuffer, sizeof(m_refBuffer), "%.2f", A5_REFERENCE);
}

void SettingsPanel::render(AppState& state, TemperamentManager& temperaments)
{
    if (!m_visible) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(340, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Tuner Options", &m_visible)) {
        renderCheckboxes(state);
        ImGui::Separator();
        renderDropdowns(state, temperaments);
        ImGui::Separator();
        renderReference(state);

        if (m_showFilters) {
            ImGui::Separator();
            renderFilters(state);
        }
    }
    ImGui::End();
}

void SettingsPanel::renderCheckboxes(AppState& state)
{
    ImGui::Columns(2, nullptr, false);

    // Left column
    if (ImGui::Checkbox("Zoom spectrum", &state.spectrumZoom)) {
        // Save preference
    }
    if (ImGui::Checkbox("Audio filter", &state.audioFilter)) {
        // Save preference
    }
    if (ImGui::Checkbox("Multiple notes", &state.multipleNotes)) {
    }
    if (ImGui::Checkbox("Fundamental", &state.fundamentalFilter)) {
    }

    ImGui::NextColumn();

    // Right column
    if (ImGui::Checkbox("Display strobe", &state.showStrobe)) {
        state.showStaff = !state.showStrobe;
    }
    if (ImGui::Checkbox("Downsample", &state.downsample)) {
    }
    if (ImGui::Checkbox("Lock display", &state.displayLock)) {
    }
    if (ImGui::Checkbox("Note filter", &state.noteFilter)) {
    }

    ImGui::Columns(1);
}

void SettingsPanel::renderDropdowns(AppState& state, TemperamentManager& temperaments)
{
    // Spectrum expand
    const char* expandItems[] = { "x1", "x2", "x4", "x8", "x16" };
    int expandIndex = 0;
    for (int i = 0; i < 5; i++) {
        if (state.spectrumExpand == (1 << i)) {
            expandIndex = i;
            break;
        }
    }
    if (ImGui::Combo("Spectrum expand", &expandIndex, expandItems, 5)) {
        state.spectrumExpand = 1 << expandIndex;
    }

    // Strobe colours
    const char* colorItems[] = { "Blue/Cyan", "Olive/Aqua", "Magenta/Yellow" };
    int colorIndex = static_cast<int>(state.strobeColor);
    if (ImGui::Combo("Strobe colours", &colorIndex, colorItems, 3)) {
        state.strobeColor = static_cast<StrobeColor>(colorIndex);
    }

    // Transpose
    const char* transposeItems[] = {
        "+6 [Key: F#]", "+5 [Key: F]", "+4 [Key: E]",
        "+3 [Key: Eb]", "+2 [Key: D]", "+1 [Key: C#]",
        "+0 [Key: C]", "-1 [Key: B]", "-2 [Key: Bb]",
        "-3 [Key: A]", "-4 [Key: Ab]", "-5 [Key: G]", "-6 [Key: F#]"
    };
    int transposeIndex = 6 - state.transpose;
    if (ImGui::Combo("Transpose", &transposeIndex, transposeItems, 13)) {
        state.transpose = 6 - transposeIndex;
    }

    // Temperament
    auto tempNames = temperaments.getNames();
    if (ImGui::BeginCombo("Temperament", tempNames[state.currentTemperament].c_str())) {
        for (int i = 0; i < static_cast<int>(tempNames.size()); i++) {
            bool selected = (state.currentTemperament == i);
            if (ImGui::Selectable(tempNames[i].c_str(), selected)) {
                state.currentTemperament = i;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Key
    const char* keyItems[] = {
        "C", "C#", "D", "Eb", "E", "F",
        "F#", "G", "Ab", "A", "Bb", "B"
    };
    if (ImGui::Combo("Key", &state.key, keyItems, 12)) {
    }

    // Filters button
    if (state.noteFilter) {
        if (ImGui::Button("Filters...")) {
            m_showFilters = !m_showFilters;
        }
    }
}

void SettingsPanel::renderReference(AppState& state)
{
    ImGui::Text("Reference:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputText("##ref", m_refBuffer, sizeof(m_refBuffer),
                         ImGuiInputTextFlags_CharsDecimal)) {
        double value = atof(m_refBuffer);
        if (value >= 420.0 && value <= 480.0) {
            state.referenceFrequency = value;
        }
    }
    ImGui::SameLine();
    ImGui::Text("Hz");

    // Up/down buttons
    ImGui::SameLine();
    if (ImGui::Button("-")) {
        state.referenceFrequency -= 0.1;
        if (state.referenceFrequency < 420.0) state.referenceFrequency = 420.0;
        snprintf(m_refBuffer, sizeof(m_refBuffer), "%.2f", state.referenceFrequency);
    }
    ImGui::SameLine();
    if (ImGui::Button("+")) {
        state.referenceFrequency += 0.1;
        if (state.referenceFrequency > 480.0) state.referenceFrequency = 480.0;
        snprintf(m_refBuffer, sizeof(m_refBuffer), "%.2f", state.referenceFrequency);
    }
}

void SettingsPanel::renderFilters(AppState& state)
{
    ImGui::Text("Note Filters:");
    ImGui::Columns(6, nullptr, false);

    const char* noteNames[] = {
        "C", "C#", "D", "Eb", "E", "F",
        "F#", "G", "Ab", "A", "Bb", "B"
    };

    for (int i = 0; i < 12; i++) {
        char label[8];
        snprintf(label, sizeof(label), "%s##n%d", noteNames[i], i);
        ImGui::Checkbox(label, &state.filters.notes[i]);
        if ((i + 1) % 2 == 0) ImGui::NextColumn();
    }

    ImGui::Columns(1);

    ImGui::Text("Octave Filters:");
    ImGui::Columns(5, nullptr, false);

    for (int i = 0; i < 9; i++) {
        char label[16];
        snprintf(label, sizeof(label), "Oct %d##o%d", i, i);
        ImGui::Checkbox(label, &state.filters.octaves[i]);
        if ((i + 1) % 2 == 0) ImGui::NextColumn();
    }

    ImGui::Columns(1);
}

} // namespace ctuner
