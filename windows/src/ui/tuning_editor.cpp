////////////////////////////////////////////////////////////////////////////////
//
//  tuning_editor.cpp - Custom tuning editor with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "tuning_editor.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>

namespace ctuner {

const char* TuningEditor::s_noteNames[12] = {
    "C", "C#", "D", "Eb", "E", "F",
    "F#", "G", "Ab", "A", "Bb", "B"
};

TuningEditor::TuningEditor()
{
}

void TuningEditor::render(AppState& state, TemperamentManager& temperaments,
                          CustomTunings& customTunings)
{
    if (!m_visible) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Custom Tunings", &m_visible)) {
        // Tabs for list and editor
        if (ImGui::BeginTabBar("TuningTabs")) {
            if (ImGui::BeginTabItem("Custom Tunings")) {
                renderTuningList(temperaments, customTunings);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("New/Edit")) {
                renderNewTuning(temperaments, customTunings);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void TuningEditor::renderTuningList(TemperamentManager& temperaments,
                                     CustomTunings& customTunings)
{
    ImGui::Text("Custom tunings:");

    if (ImGui::BeginListBox("##customlist", ImVec2(-1, 200))) {
        int builtInCount = temperaments.getBuiltInCount();
        int totalCount = temperaments.getCount();

        for (int i = builtInCount; i < totalCount; i++) {
            const auto& t = temperaments.get(i);
            bool selected = (m_editIndex == i);

            if (ImGui::Selectable(t.name.c_str(), selected)) {
                m_editIndex = i;

                // Copy to edit buffers
                strncpy(m_nameBuffer, t.name.c_str(), sizeof(m_nameBuffer) - 1);
                strncpy(m_descBuffer, t.description.c_str(), sizeof(m_descBuffer) - 1);
                for (int j = 0; j < 12; j++) {
                    m_ratios[j] = t.ratios[j];
                }
            }
        }
        ImGui::EndListBox();
    }

    // Buttons
    if (ImGui::Button("New")) {
        m_editIndex = -1;
        m_nameBuffer[0] = '\0';
        m_descBuffer[0] = '\0';
        // Reset to equal temperament ratios
        const auto& equal = temperaments.get(temperaments.getEqualTemperamentIndex());
        for (int i = 0; i < 12; i++) {
            m_ratios[i] = equal.ratios[i];
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete") && m_editIndex >= temperaments.getBuiltInCount()) {
        // Delete file
        const auto& t = temperaments.get(m_editIndex);
        std::string filename = CustomTunings::generateFilename(t.name);
        customTunings.deleteFile(filename);

        // Remove from manager
        temperaments.removeCustom(m_editIndex);
        m_editIndex = -1;
    }

    ImGui::SameLine();
    if (ImGui::Button("Reload All")) {
        loadCustomTunings(temperaments, customTunings);
    }
}

void TuningEditor::renderNewTuning(TemperamentManager& temperaments,
                                    CustomTunings& customTunings)
{
    ImGui::Text("Name:");
    ImGui::InputText("##name", m_nameBuffer, sizeof(m_nameBuffer));

    ImGui::Text("Description:");
    ImGui::InputTextMultiline("##desc", m_descBuffer, sizeof(m_descBuffer),
                               ImVec2(-1, 60));

    ImGui::Separator();
    renderRatioEditor();
    ImGui::Separator();

    // Save button
    if (ImGui::Button("Save")) {
        if (strlen(m_nameBuffer) > 0) {
            std::array<double, 12> ratios;
            for (int i = 0; i < 12; i++) {
                ratios[i] = m_ratios[i];
            }

            Temperament t(m_nameBuffer, m_descBuffer, ratios, true);

            // Save to file
            std::string filename = CustomTunings::generateFilename(m_nameBuffer);
            if (customTunings.saveFile(t, filename)) {
                if (m_editIndex >= temperaments.getBuiltInCount()) {
                    // Update existing
                    temperaments.updateCustom(m_editIndex, t);
                } else {
                    // Add new
                    m_editIndex = temperaments.addCustom(t);
                }
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset to Equal")) {
        const auto& equal = temperaments.get(temperaments.getEqualTemperamentIndex());
        for (int i = 0; i < 12; i++) {
            m_ratios[i] = equal.ratios[i];
        }
    }
}

void TuningEditor::renderRatioEditor()
{
    ImGui::Text("Ratios (relative to C):");

    ImGui::Columns(2, nullptr, false);

    for (int i = 0; i < 12; i++) {
        char label[16];
        snprintf(label, sizeof(label), "%s##r%d", s_noteNames[i], i);

        ImGui::SetNextItemWidth(100);
        ImGui::InputDouble(label, &m_ratios[i], 0.001, 0.01, "%.9f");

        if ((i + 1) % 6 == 0) {
            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1);

    // Show cents from equal temperament
    ImGui::Text("Cents deviation from Equal:");
    const double equalRatios[12] = {
        1.0, 1.059463094, 1.122462048, 1.189207115,
        1.259921050, 1.334839854, 1.414213562, 1.498307077,
        1.587401052, 1.681792831, 1.781797436, 1.887748625
    };

    ImGui::Columns(4, nullptr, false);
    for (int i = 0; i < 12; i++) {
        double cents = 1200.0 * std::log2(m_ratios[i] / equalRatios[i]);
        ImGui::Text("%s: %+.1f", s_noteNames[i], cents);
        if ((i + 1) % 3 == 0) ImGui::NextColumn();
    }
    ImGui::Columns(1);
}

void TuningEditor::loadCustomTunings(TemperamentManager& temperaments,
                                      CustomTunings& customTunings)
{
    auto tunings = customTunings.loadAll();
    for (const auto& t : tunings) {
        // Check if already exists
        if (temperaments.findByName(t.name) < 0) {
            temperaments.addCustom(t);
        }
    }
}

} // namespace ctuner
