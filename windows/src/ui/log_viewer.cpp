////////////////////////////////////////////////////////////////////////////////
//
//  log_viewer.cpp - Window to display logged notes during recording
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "log_viewer.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>

namespace ctuner {

LogViewer::LogViewer()
{
}

void LogViewer::render(AppState& state, FrequencyLogger& logger)
{
    if (!m_visible) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Log Viewer", &m_visible, ImGuiWindowFlags_MenuBar)) {
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Auto-scroll", nullptr, &m_autoScroll);
                ImGui::MenuItem("Show Graph", nullptr, &m_showGraph);
                ImGui::Separator();
                if (ImGui::MenuItem("All Entries", nullptr, m_displayMode == 0)) {
                    m_displayMode = 0;
                }
                if (ImGui::MenuItem("Summary Only", nullptr, m_displayMode == 1)) {
                    m_displayMode = 1;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        const auto& entries = logger.getEntries();
        auto stats = logger.getStatistics();
        double duration = logger.getSessionDuration();

        // Status line
        if (logger.isLogging()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "RECORDING");
            ImGui::SameLine();
        }
        ImGui::Text("Entries: %d | Duration: %.1fs", static_cast<int>(entries.size()), duration);

        ImGui::Separator();

        // Statistics panel
        renderStatistics(stats, duration);

        ImGui::Separator();

        // Pitch graph
        if (m_showGraph && !entries.empty()) {
            renderPitchGraph(entries);
            ImGui::Separator();
        }

        // Entries table
        if (m_displayMode == 0) {
            renderTable(entries);
        }
    }
    ImGui::End();
}

void LogViewer::renderStatistics(const FrequencyLogger::Statistics& stats, double duration)
{
    ImGui::Text("Statistics:");

    ImGui::Columns(2, "stats_columns", false);

    ImGui::Text("Total notes:");
    ImGui::Text("Unique notes:");
    ImGui::Text("Notes/second:");

    ImGui::NextColumn();

    ImGui::Text("%d", stats.totalNotes);
    ImGui::Text("%d", stats.uniqueNotes);
    if (duration > 0) {
        ImGui::Text("%.1f", stats.totalNotes / duration);
    } else {
        ImGui::Text("--");
    }

    ImGui::Columns(1);

    ImGui::Columns(2, "stats_columns2", false);

    ImGui::Text("Avg frequency:");
    ImGui::Text("Avg cents deviation:");
    ImGui::Text("Max cents deviation:");

    ImGui::NextColumn();

    if (stats.totalNotes > 0) {
        ImGui::Text("%.2f Hz", stats.averageFrequency);
        ImGui::Text("%.1f cents", stats.averageCents);
        ImGui::Text("%.1f cents", stats.maxCentsDeviation);
    } else {
        ImGui::Text("--");
        ImGui::Text("--");
        ImGui::Text("--");
    }

    ImGui::Columns(1);
}

void LogViewer::renderPitchGraph(const std::vector<LogEntry>& entries)
{
    ImGui::Text("Cents Deviation Over Time:");

    ImVec2 graphSize(-1, 80);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.y = 80;

    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Background
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                        IM_COL32(40, 40, 40, 255));

    // Center line (0 cents)
    float centerY = pos.y + size.y / 2.0f;
    draw->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + size.x, centerY),
                  IM_COL32(100, 100, 100, 255));

    // +/- 50 cents lines
    float range50Y = size.y / 4.0f;
    draw->AddLine(ImVec2(pos.x, centerY - range50Y), ImVec2(pos.x + size.x, centerY - range50Y),
                  IM_COL32(60, 60, 60, 255));
    draw->AddLine(ImVec2(pos.x, centerY + range50Y), ImVec2(pos.x + size.x, centerY + range50Y),
                  IM_COL32(60, 60, 60, 255));

    // Labels
    draw->AddText(ImVec2(pos.x + 2, centerY - range50Y - 12), IM_COL32(150, 150, 150, 255), "+50");
    draw->AddText(ImVec2(pos.x + 2, centerY + range50Y + 2), IM_COL32(150, 150, 150, 255), "-50");

    if (!entries.empty()) {
        // Determine visible range (last N entries or time-based)
        int maxVisible = static_cast<int>(size.x / 2);  // 2 pixels per entry
        int startIdx = std::max(0, static_cast<int>(entries.size()) - maxVisible);

        // Draw pitch line
        float xStep = size.x / std::min(static_cast<int>(entries.size() - startIdx), maxVisible);

        ImVec2 prevPoint;
        bool firstPoint = true;

        for (size_t i = startIdx; i < entries.size(); i++) {
            float x = pos.x + (i - startIdx) * xStep;
            // Clamp cents to +/- 100 for display
            float cents = static_cast<float>(std::max(-100.0, std::min(100.0, entries[i].cents)));
            float y = centerY - (cents / 100.0f) * (size.y / 2.0f);

            ImVec2 point(x, y);

            // Color based on how in-tune (green = good, yellow = ok, red = bad)
            ImU32 color;
            float absCents = std::fabs(cents);
            if (absCents < 10) {
                color = IM_COL32(0, 255, 0, 255);  // Green
            } else if (absCents < 25) {
                color = IM_COL32(255, 255, 0, 255);  // Yellow
            } else {
                color = IM_COL32(255, 100, 100, 255);  // Red
            }

            if (!firstPoint) {
                draw->AddLine(prevPoint, point, color, 1.5f);
            }
            prevPoint = point;
            firstPoint = false;
        }
    }

    // Advance cursor
    ImGui::Dummy(graphSize);
}

void LogViewer::renderTable(const std::vector<LogEntry>& entries)
{
    ImGui::Text("Recorded Notes:");

    if (ImGui::BeginTable("log_table", 5,
                          ImGuiTableFlags_Borders |
                          ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_Resizable,
                          ImVec2(-1, -1))) {

        ImGui::TableSetupScrollFreeze(0, 1);  // Freeze header row
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Note", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Octave", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Frequency", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Cents", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        // Use clipper for performance with large logs
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(entries.size()));

        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                const auto& entry = entries[row];

                ImGui::TableNextRow();

                // Time
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%.2f", entry.timestamp);

                // Note
                ImGui::TableSetColumnIndex(1);
                int noteIdx = ((entry.note % OCTAVE) + OCTAVE) % OCTAVE;  // Handle negative
                ImGui::Text("%s", NOTE_NAMES[noteIdx]);

                // Octave
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", entry.octave);

                // Frequency
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.2f Hz", entry.frequency);

                // Cents with color
                ImGui::TableSetColumnIndex(4);
                float absCents = static_cast<float>(std::fabs(entry.cents));
                ImVec4 color;
                if (absCents < 10) {
                    color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green
                } else if (absCents < 25) {
                    color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
                } else {
                    color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);  // Red
                }
                ImGui::TextColored(color, "%+.1f", entry.cents);
            }
        }

        // Auto-scroll to bottom
        if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndTable();
    }
}

} // namespace ctuner
