////////////////////////////////////////////////////////////////////////////////
//
//  strobe_view.cpp - Strobe tuning display with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "strobe_view.h"
#include "imgui.h"
#include <cmath>

namespace ctuner {

// Helper function to lerp between ImVec4 colors
static ImVec4 LerpColor(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    );
}

// Color schemes: fgColor, bgColor
const StrobeView::ColorScheme StrobeView::s_colorSchemes[3] = {
    { IM_COL32(63, 63, 255, 255), IM_COL32(63, 255, 255, 255) },   // Blue/Cyan
    { IM_COL32(111, 111, 0, 255), IM_COL32(191, 255, 191, 255) },  // Olive/Aqua
    { IM_COL32(255, 63, 255, 255), IM_COL32(255, 255, 63, 255) }   // Magenta/Yellow
};

StrobeView::StrobeView()
{
}

void StrobeView::render(AppState& state)
{
    if (!state.showStrobe) {
        return;
    }

    // Smooth the cents value
    m_smoothedCents = (m_smoothedCents * 19.0 + state.currentPitch.cents) / 20.0;

    // Update phase based on cents deviation
    m_phase += static_cast<float>(m_smoothedCents) * 50.0f;

    // Wrap phase
    float blockWidth = 64.0f;  // Will be adjusted based on row
    if (m_phase > blockWidth * 16.0f) m_phase -= blockWidth * 16.0f;
    if (m_phase < 0.0f) m_phase += blockWidth * 16.0f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::BeginChild("Strobe", ImVec2(-1, 68), true)) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        int colorIdx = static_cast<int>(state.strobeColor);
        ImU32 fg = s_colorSchemes[colorIdx].fgColor;
        ImU32 bg = s_colorSchemes[colorIdx].bgColor;

        float rowHeight = size.y / 4.0f;
        float absSmooth = std::fabs(m_smoothedCents);

        // Row 0: smallest blocks
        float block0 = rowHeight * 2.0f;
        bool shaded0 = absSmooth > 0.2;
        if (absSmooth > 0.4) {
            // Just show background
            draw->AddRectFilled(ImVec2(pos.x, pos.y),
                               ImVec2(pos.x + size.x, pos.y + rowHeight), bg);
        } else {
            drawStrobeRow(pos.y, rowHeight, block0, m_phase * block0 / (rowHeight * 8), shaded0);
        }

        // Row 1: medium blocks
        float block1 = rowHeight * 4.0f;
        bool shaded1 = absSmooth > 0.3;
        drawStrobeRow(pos.y + rowHeight, rowHeight, block1,
                      m_phase * block1 / (rowHeight * 8), shaded1);

        // Row 2: large blocks
        float block2 = rowHeight * 8.0f;
        bool shaded2 = absSmooth > 0.4;
        drawStrobeRow(pos.y + rowHeight * 2, rowHeight, block2,
                      m_phase * block2 / (rowHeight * 8), shaded2);

        // Row 3: extra large blocks
        float block3 = rowHeight * 16.0f;
        drawStrobeRow(pos.y + rowHeight * 3, rowHeight, block3,
                      m_phase * block3 / (rowHeight * 8), false);

        // Handle click to toggle strobe/staff
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            state.toggleStrobe();
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleColor();
}

void StrobeView::drawStrobeRow(float y, float height, float blockWidth, float offset, bool shaded)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Wrap offset
    float totalWidth = blockWidth * 2.0f;
    offset = std::fmod(offset, totalWidth);
    if (offset < 0) offset += totalWidth;

    // Get colors from parent (using default olive for now)
    ImU32 fg = IM_COL32(111, 111, 0, 255);
    ImU32 bg = IM_COL32(191, 255, 191, 255);

    // Draw alternating blocks
    float x = pos.x - offset;
    while (x < pos.x + size.x) {
        // Foreground block
        float x1 = std::max(x, pos.x);
        float x2 = std::min(x + blockWidth, pos.x + size.x);
        if (x2 > x1) {
            if (shaded) {
                // Draw with gradient for shaded effect
                for (float gx = x1; gx < x2; gx += 2.0f) {
                    float t = (gx - x) / blockWidth;
                    ImU32 color = ImGui::ColorConvertFloat4ToU32(
                        LerpColor(ImGui::ColorConvertU32ToFloat4(fg),
                                  ImGui::ColorConvertU32ToFloat4(bg), t));
                    draw->AddRectFilled(ImVec2(gx, y), ImVec2(gx + 2, y + height), color);
                }
            } else {
                draw->AddRectFilled(ImVec2(x1, y), ImVec2(x2, y + height), fg);
            }
        }

        // Background block
        x += blockWidth;
        x1 = std::max(x, pos.x);
        x2 = std::min(x + blockWidth, pos.x + size.x);
        if (x2 > x1) {
            if (shaded) {
                for (float gx = x1; gx < x2; gx += 2.0f) {
                    float t = (gx - x) / blockWidth;
                    ImU32 color = ImGui::ColorConvertFloat4ToU32(
                        LerpColor(ImGui::ColorConvertU32ToFloat4(bg),
                                  ImGui::ColorConvertU32ToFloat4(fg), t));
                    draw->AddRectFilled(ImVec2(gx, y), ImVec2(gx + 2, y + height), color);
                }
            } else {
                draw->AddRectFilled(ImVec2(x1, y), ImVec2(x2, y + height), bg);
            }
        }

        x += blockWidth;
    }
}

} // namespace ctuner
