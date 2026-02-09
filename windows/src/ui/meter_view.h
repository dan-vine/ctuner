////////////////////////////////////////////////////////////////////////////////
//
//  meter_view.h - Tuning meter display with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef METER_VIEW_H
#define METER_VIEW_H

#include "../app_state.h"

namespace ctuner {

class MeterView {
public:
    MeterView();
    ~MeterView() = default;

    // Render the meter
    void render(AppState& state);

    // Update cents value
    void setCents(double cents) { m_targetCents = cents; }

private:
    void drawScale(float width, float height);
    void drawNeedle(float width, float height, float cents);

    double m_targetCents = 0.0;
    double m_displayCents = 0.0;  // Smoothed display value
};

} // namespace ctuner

#endif // METER_VIEW_H
