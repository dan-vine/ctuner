////////////////////////////////////////////////////////////////////////////////
//
//  log_viewer.h - Window to display logged notes during recording
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LOG_VIEWER_H
#define LOG_VIEWER_H

#include "../app_state.h"
#include "../logging/frequency_logger.h"

namespace ctuner {

class LogViewer {
public:
    LogViewer();
    ~LogViewer() = default;

    // Render the log viewer window
    void render(AppState& state, FrequencyLogger& logger);

    // Show/hide the window
    void show() { m_visible = true; }
    void hide() { m_visible = false; }
    bool isVisible() const { return m_visible; }
    void toggle() { m_visible = !m_visible; }

private:
    void renderTable(const std::vector<LogEntry>& entries);
    void renderStatistics(const FrequencyLogger::Statistics& stats, double duration);
    void renderPitchGraph(const std::vector<LogEntry>& entries);

    bool m_visible = false;
    bool m_autoScroll = true;
    bool m_showGraph = true;
    int m_displayMode = 0;  // 0=all, 1=unique notes only
};

} // namespace ctuner

#endif // LOG_VIEWER_H
