////////////////////////////////////////////////////////////////////////////////
//
//  strobe_view.h - Strobe tuning display with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef STROBE_VIEW_H
#define STROBE_VIEW_H

#include "imgui.h"
#include "../app_state.h"

namespace ctuner {

class StrobeView {
public:
    StrobeView();
    ~StrobeView() = default;

    // Render the strobe display
    void render(AppState& state);

    // Update cents value
    void setCents(double cents) { m_targetCents = cents; }

private:
    void drawStrobeRow(float y, float height, float blockWidth, float offset, bool shaded);

    double m_targetCents = 0.0;
    double m_smoothedCents = 0.0;
    float m_phase = 0.0f;

    // Color schemes
    struct ColorScheme {
        ImU32 fgColor;
        ImU32 bgColor;
    };
    static const ColorScheme s_colorSchemes[3];
};

} // namespace ctuner

#endif // STROBE_VIEW_H
