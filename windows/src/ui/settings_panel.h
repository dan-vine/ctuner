////////////////////////////////////////////////////////////////////////////////
//
//  settings_panel.h - Options/settings panel with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SETTINGS_PANEL_H
#define SETTINGS_PANEL_H

#include "../app_state.h"
#include "../tuning/temperament.h"

namespace ctuner {

class SettingsPanel {
public:
    SettingsPanel();
    ~SettingsPanel() = default;

    // Render the settings panel
    void render(AppState& state, TemperamentManager& temperaments);

    // Show/hide panel
    void show() { m_visible = true; }
    void hide() { m_visible = false; }
    bool isVisible() const { return m_visible; }

private:
    void renderCheckboxes(AppState& state);
    void renderDropdowns(AppState& state, TemperamentManager& temperaments);
    void renderReference(AppState& state);
    void renderFilters(AppState& state);

    bool m_visible = false;
    bool m_showFilters = false;

    // Reference frequency input
    char m_refBuffer[16] = "440.00";
};

} // namespace ctuner

#endif // SETTINGS_PANEL_H
