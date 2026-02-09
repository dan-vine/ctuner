////////////////////////////////////////////////////////////////////////////////
//
//  staff_view.h - Musical staff notation display with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef STAFF_VIEW_H
#define STAFF_VIEW_H

#include "../app_state.h"

namespace ctuner {

class StaffView {
public:
    StaffView();
    ~StaffView() = default;

    // Render the staff display
    void render(AppState& state);

    // Set note to display
    void setNote(int note) { m_note = note; }
    void setTranspose(int transpose) { m_transpose = transpose; }

private:
    void drawStaff(float width, float height);
    void drawClefs(float width, float height);
    void drawNote(float width, float height, int note);
    void drawAccidental(float x, float y, int type, float scale);

    int m_note = 0;
    int m_transpose = 0;

    // Scale offsets for staff position
    static const int s_scaleOffsets[12];
    static const int s_accidentals[12];

    enum Accidental { NATURAL = 0, SHARP = 1, FLAT = 2 };
};

} // namespace ctuner

#endif // STAFF_VIEW_H
