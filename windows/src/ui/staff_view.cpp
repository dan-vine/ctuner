////////////////////////////////////////////////////////////////////////////////
//
//  staff_view.cpp - Musical staff notation display with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "staff_view.h"
#include "imgui.h"
#include <cmath>

namespace ctuner {

// Scale offsets: how many lines/spaces from C
const int StaffView::s_scaleOffsets[12] = {
    0, 0, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6
};

// Accidentals: 0=natural, 1=sharp, 2=flat
const int StaffView::s_accidentals[12] = {
    NATURAL, SHARP, NATURAL, FLAT, NATURAL, NATURAL,
    SHARP, NATURAL, FLAT, NATURAL, FLAT, NATURAL
};

StaffView::StaffView()
{
}

void StaffView::render(AppState& state)
{
    if (!state.showStaff) {
        return;
    }

    m_note = state.currentPitch.note;
    m_transpose = state.transpose;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::BeginChild("Staff", ImVec2(-1, 68), true)) {
        ImVec2 size = ImGui::GetContentRegionAvail();

        drawStaff(size.x, size.y);
        drawClefs(size.x, size.y);

        if (state.currentPitch.valid) {
            drawNote(size.x, size.y, m_note - m_transpose);
        }

        // Handle click to toggle strobe/staff
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            state.toggleStrobe();
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleColor();
}

void StaffView::drawStaff(float width, float height)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    float lineHeight = height / 14.0f;
    float margin = width / 32.0f;
    ImU32 black = IM_COL32(0, 0, 0, 255);

    // Draw treble staff (upper 5 lines)
    float centerY = pos.y + height / 2.0f;
    for (int i = 1; i <= 5; i++) {
        float y = centerY - i * lineHeight;
        draw->AddLine(ImVec2(pos.x + margin, y),
                      ImVec2(pos.x + width - margin, y), black);
    }

    // Draw bass staff (lower 5 lines)
    for (int i = 1; i <= 5; i++) {
        float y = centerY + i * lineHeight;
        draw->AddLine(ImVec2(pos.x + margin, y),
                      ImVec2(pos.x + width - margin, y), black);
    }

    // Draw middle C leger line (short, centered)
    float lineWidth = width / 16.0f;
    draw->AddLine(ImVec2(pos.x + width / 2 - lineWidth / 2, centerY),
                  ImVec2(pos.x + width / 2 + lineWidth / 2, centerY), black);
}

void StaffView::drawClefs(float width, float height)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    float lineHeight = height / 14.0f;
    float margin = width / 32.0f;
    float centerY = pos.y + height / 2.0f;
    ImU32 black = IM_COL32(0, 0, 0, 255);

    // Simplified treble clef (G clef) - just draw "G" for now
    // In a real implementation, this would be a bezier curve
    float trebleX = pos.x + margin + lineHeight / 2;
    float trebleY = centerY - lineHeight * 3;
    draw->AddText(ImVec2(trebleX - 4, trebleY - 8), black, "G");

    // Simplified bass clef (F clef) - just draw "F" for now
    float bassX = pos.x + margin + lineHeight / 2;
    float bassY = centerY + lineHeight * 3;
    draw->AddText(ImVec2(bassX - 4, bassY - 8), black, "F");
}

void StaffView::drawNote(float width, float height, int note)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    float lineHeight = height / 14.0f;
    float lineWidth = width / 16.0f;
    float centerY = pos.y + height / 2.0f;
    ImU32 black = IM_COL32(0, 0, 0, 255);

    // Adjust note to positive
    int adjustedNote = (note + OCTAVE * 10) % (OCTAVE * 10);
    int octave = adjustedNote / OCTAVE;
    int noteInOctave = adjustedNote % OCTAVE;

    // Wrap octaves for display (show octaves 2-7)
    if (octave >= 6) octave -= 2;
    else if (octave == 0 && noteInOctave <= 1) octave += 4;
    else if (octave <= 1 || (octave == 2 && noteInOctave <= 1)) octave += 2;

    // Calculate Y position
    // Middle C (C4, note 48) is at centerY
    // Each step is half a lineHeight
    int c4Note = 48;
    int noteOffset = adjustedNote - c4Note;
    int scaleOffset = s_scaleOffsets[noteInOctave];
    int octaveOffset = (octave - 4) * 7;  // 7 steps per octave
    int totalOffset = octaveOffset + scaleOffset;

    float noteY = centerY - totalOffset * lineHeight / 2.0f;
    float noteX = pos.x + width / 2.0f;

    // Draw note head (filled ellipse)
    float noteWidth = lineHeight * 0.8f;
    float noteHeight = lineHeight * 0.6f;

    draw->AddEllipseFilled(ImVec2(noteX, noteY), noteWidth, noteHeight, black);

    // Draw leger lines if needed
    if (noteY < centerY - lineHeight * 5) {
        // Above treble staff
        for (float ly = centerY - lineHeight * 6; ly >= noteY; ly -= lineHeight) {
            draw->AddLine(ImVec2(noteX - lineWidth / 2, ly),
                          ImVec2(noteX + lineWidth / 2, ly), black);
        }
    } else if (noteY > centerY + lineHeight * 5) {
        // Below bass staff
        for (float ly = centerY + lineHeight * 6; ly <= noteY; ly += lineHeight) {
            draw->AddLine(ImVec2(noteX - lineWidth / 2, ly),
                          ImVec2(noteX + lineWidth / 2, ly), black);
        }
    } else if (std::fabs(noteY - centerY) < lineHeight / 2) {
        // Middle C
        draw->AddLine(ImVec2(noteX - lineWidth / 2, centerY),
                      ImVec2(noteX + lineWidth / 2, centerY), black);
    }

    // Draw accidental if needed
    int accidental = s_accidentals[noteInOctave];
    if (accidental != NATURAL) {
        drawAccidental(noteX - lineWidth, noteY, accidental, lineHeight / 10.0f);
    }
}

void StaffView::drawAccidental(float x, float y, int type, float scale)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImU32 black = IM_COL32(0, 0, 0, 255);

    if (type == SHARP) {
        // Draw sharp symbol (#)
        draw->AddText(ImVec2(x - 8, y - 8), black, "#");
    } else if (type == FLAT) {
        // Draw flat symbol (b)
        draw->AddText(ImVec2(x - 8, y - 8), black, "b");
    }
}

} // namespace ctuner
