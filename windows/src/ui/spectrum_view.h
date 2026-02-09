////////////////////////////////////////////////////////////////////////////////
//
//  spectrum_view.h - Spectrum visualization with ImGui
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SPECTRUM_VIEW_H
#define SPECTRUM_VIEW_H

#include "../app_state.h"
#include <vector>

namespace ctuner {

class SpectrumView {
public:
    SpectrumView();
    ~SpectrumView() = default;

    // Render the spectrum view
    void render(AppState& state);

    // Update spectrum data
    void updateData(const std::vector<double>& spectrum,
                    float freq, float ref, float low, float high,
                    const std::vector<Maximum>& maxima, int count);

private:
    void renderZoomedSpectrum(AppState& state);
    void renderFullSpectrum(AppState& state);
    void drawGraticule(float width, float height);

    std::vector<double> m_spectrum;
    std::vector<Maximum> m_maxima;
    int m_maximaCount = 0;

    float m_freq = 0.0f;
    float m_ref = 0.0f;
    float m_low = 0.0f;
    float m_high = 0.0f;
    float m_maxValue = 1.0f;
};

} // namespace ctuner

#endif // SPECTRUM_VIEW_H
