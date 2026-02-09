////////////////////////////////////////////////////////////////////////////////
//
//  temperament.h - Temperament handling
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TEMPERAMENT_H
#define TEMPERAMENT_H

#include <string>
#include <vector>
#include <array>

namespace ctuner {

// Temperament data structure
struct Temperament {
    std::string name;
    std::string description;
    std::array<double, 12> ratios;  // Ratios for C, C#, D, Eb, E, F, F#, G, Ab, A, Bb, B
    bool isCustom = false;

    Temperament() : ratios{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0} {}

    Temperament(const std::string& n, const std::string& desc,
                const std::array<double, 12>& r, bool custom = false)
        : name(n), description(desc), ratios(r), isCustom(custom) {}

    // Get ratio for a note (0 = C, 11 = B)
    double getRatio(int note) const {
        return ratios[note % 12];
    }

    // Get ratio adjusted for a key
    double getRatioInKey(int note, int key) const {
        int adjustedNote = (note - key + 12) % 12;
        return ratios[adjustedNote];
    }
};

// Temperament manager - handles built-in and custom temperaments
class TemperamentManager {
public:
    TemperamentManager();
    ~TemperamentManager() = default;

    // Get number of temperaments
    int getCount() const { return static_cast<int>(m_temperaments.size()); }

    // Get temperament by index
    const Temperament& get(int index) const;

    // Get temperament name
    const std::string& getName(int index) const;

    // Get all temperament names for UI
    std::vector<std::string> getNames() const;

    // Find temperament by name
    int findByName(const std::string& name) const;

    // Add custom temperament
    int addCustom(const Temperament& temperament);

    // Remove custom temperament
    bool removeCustom(int index);

    // Update custom temperament
    bool updateCustom(int index, const Temperament& temperament);

    // Get index of Equal Temperament
    int getEqualTemperamentIndex() const { return 8; }

    // Get number of built-in temperaments
    int getBuiltInCount() const { return m_builtInCount; }

private:
    void initializeBuiltIn();

    std::vector<Temperament> m_temperaments;
    int m_builtInCount = 0;
};

} // namespace ctuner

#endif // TEMPERAMENT_H
