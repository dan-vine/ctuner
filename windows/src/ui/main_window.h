////////////////////////////////////////////////////////////////////////////////
//
//  main_window.h - Main tuner UI window with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "../app_state.h"
#include "../tuning/temperament.h"
#include "../tuning/custom_tunings.h"
#include "../logging/frequency_logger.h"
#include "spectrum_view.h"
#include "meter_view.h"
#include "strobe_view.h"
#include "staff_view.h"
#include "settings_panel.h"
#include "tuning_editor.h"
#include "log_viewer.h"

namespace ctuner {

class MainWindow {
public:
    MainWindow();
    ~MainWindow() = default;

    // Initialize components
    void initialize();

    // Render the main window
    void render(AppState& state);

    // Access to components for external updates
    SpectrumView& getSpectrumView() { return m_spectrumView; }
    TemperamentManager& getTemperaments() { return m_temperaments; }
    FrequencyLogger& getLogger() { return m_logger; }
    LogViewer& getLogViewer() { return m_logViewer; }

private:
    void renderMenuBar(AppState& state);
    void renderScopeView(AppState& state);
    void renderDisplayArea(AppState& state);
    void renderStatusBar(AppState& state);

    // UI Components
    SpectrumView m_spectrumView;
    MeterView m_meterView;
    StrobeView m_strobeView;
    StaffView m_staffView;
    SettingsPanel m_settingsPanel;
    TuningEditor m_tuningEditor;
    LogViewer m_logViewer;

    // Data managers
    TemperamentManager m_temperaments;
    CustomTunings m_customTunings;
    FrequencyLogger m_logger;

    // State
    bool m_showDemo = false;
};

} // namespace ctuner

#endif // MAIN_WINDOW_H
