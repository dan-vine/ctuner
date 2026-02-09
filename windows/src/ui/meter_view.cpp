////////////////////////////////////////////////////////////////////////////////
//
//  meter_view.cpp - Tuning meter display with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "meter_view.h"
#include "imgui.h"
#include <cmath>

namespace ctuner {

MeterView::MeterView()
{
}

void MeterView::render(AppState& state)
{
    // Smooth the display value
    m_displayCents = (m_displayCents * 19.0 + state.currentPitch.cents * 100.0) / 20.0;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::BeginChild("Meter", ImVec2(-1, 80), true)) {
        ImVec2 size = ImGui::GetContentRegionAvail();

        drawScale(size.x, size.y);
        drawNeedle(size.x, size.y, static_cast<float>(m_displayCents));

        // Handle click to toggle lock
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            state.displayLock = !state.displayLock;
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleColor();
}

void MeterView::drawScale(float width, float height)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    float centerX = pos.x + width / 2.0f;
    float scaleY = pos.y + height / 3.0f;
    float tickTop = pos.y + height / 3.0f;
    float tickBottom = pos.y + height / 2.0f;
    float smallTickTop = pos.y + height * 3.0f / 8.0f;

    ImU32 black = IM_COL32(0, 0, 0, 255);

    // Draw scale markings (-50 to +50 cents)
    float pixelsPerCent = width / 110.0f;

    for (int i = 0; i <= 5; i++) {
        float offset = i * 10.0f * pixelsPerCent;

        // Major tick marks
        draw->AddLine(ImVec2(centerX + offset, tickTop),
                      ImVec2(centerX + offset, tickBottom), black);
        draw->AddLine(ImVec2(centerX - offset, tickTop),
                      ImVec2(centerX - offset, tickBottom), black);

        // Labels
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", i * 10);
        ImVec2 textSize = ImGui::CalcTextSize(buf);
        draw->AddText(ImVec2(centerX + offset - textSize.x / 2, pos.y),
                      black, buf);
        draw->AddText(ImVec2(centerX - offset - textSize.x / 2, pos.y),
                      black, buf);

        // Minor tick marks (every 2 cents)
        if (i < 5) {
            for (int j = 1; j < 5; j++) {
                float minorOffset = (i * 10.0f + j * 2.0f) * pixelsPerCent;
                draw->AddLine(ImVec2(centerX + minorOffset, smallTickTop),
                              ImVec2(centerX + minorOffset, tickBottom), black);
                draw->AddLine(ImVec2(centerX - minorOffset, smallTickTop),
                              ImVec2(centerX - minorOffset, tickBottom), black);
            }
        }
    }

    // Draw bar outline
    float barTop = pos.y + height * 3.0f / 4.0f - 2;
    float barBottom = pos.y + height * 3.0f / 4.0f + 2;
    float barLeft = centerX - 50.0f * pixelsPerCent;
    float barRight = centerX + 50.0f * pixelsPerCent;

    draw->AddRect(ImVec2(barLeft, barTop), ImVec2(barRight, barBottom),
                  IM_COL32(192, 192, 192, 255));
}

void MeterView::drawNeedle(float width, float height, float cents)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    float centerX = pos.x + width / 2.0f;
    float needleY = pos.y + height * 3.0f / 4.0f - 2;
    float pixelsPerCent = width / 110.0f;

    // Clamp cents to -50 to +50
    cents = std::max(-50.0f, std::min(50.0f, cents));

    float needleX = centerX + cents * pixelsPerCent;
    float needleWidth = height / 12.0f;
    float needleHeight = height / 6.0f;

    // Draw needle as a filled triangle pointing down
    ImVec2 points[4] = {
        ImVec2(needleX - needleWidth, needleY - needleHeight),
        ImVec2(needleX + needleWidth, needleY - needleHeight),
        ImVec2(needleX + 1, needleY + 2),
        ImVec2(needleX - 1, needleY + 2)
    };

    // Gradient effect for 3D look
    draw->AddQuadFilled(points[0], points[1], points[2], points[3],
                        IM_COL32(127, 127, 127, 255));
    draw->AddQuad(points[0], points[1], points[2], points[3],
                  IM_COL32(64, 64, 64, 255));
}

} // namespace ctuner
