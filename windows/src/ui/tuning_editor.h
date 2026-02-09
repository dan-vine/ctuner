////////////////////////////////////////////////////////////////////////////////
//
//  tuning_editor.h - Custom tuning editor with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TUNING_EDITOR_H
#define TUNING_EDITOR_H

#include "../app_state.h"
#include "../tuning/temperament.h"
#include "../tuning/custom_tunings.h"

namespace ctuner {

class TuningEditor {
public:
    TuningEditor();
    ~TuningEditor() = default;

    // Render the tuning editor window
    void render(AppState& state, TemperamentManager& temperaments,
                CustomTunings& customTunings);

    // Show/hide editor
    void show() { m_visible = true; }
    void hide() { m_visible = false; }
    bool isVisible() const { return m_visible; }

    // Load custom tunings into temperament manager
    void loadCustomTunings(TemperamentManager& temperaments,
                           CustomTunings& customTunings);

private:
    void renderNewTuning(TemperamentManager& temperaments,
                         CustomTunings& customTunings);
    void renderTuningList(TemperamentManager& temperaments,
                          CustomTunings& customTunings);
    void renderRatioEditor();

    bool m_visible = false;

    // Editor state
    bool m_editing = false;
    int m_editIndex = -1;

    // New/edit tuning data
    char m_nameBuffer[64] = "";
    char m_descBuffer[256] = "";
    double m_ratios[12] = {1.0, 1.059463094, 1.122462048, 1.189207115,
                           1.259921050, 1.334839854, 1.414213562, 1.498307077,
                           1.587401052, 1.681792831, 1.781797436, 1.887748625};

    static const char* s_noteNames[12];
};

} // namespace ctuner

#endif // TUNING_EDITOR_H
