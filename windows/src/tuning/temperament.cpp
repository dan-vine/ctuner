////////////////////////////////////////////////////////////////////////////////
//
//  temperament.cpp - Temperament handling
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "temperament.h"
#include "built_in_temperaments.h"
#include <algorithm>

namespace ctuner {

// Static temperament data
static const double builtInTemperaments[32][12] = BUILT_IN_TEMPERAMENTS;

TemperamentManager::TemperamentManager()
{
    initializeBuiltIn();
}

void TemperamentManager::initializeBuiltIn()
{
    m_temperaments.clear();
    m_temperaments.reserve(NUM_BUILT_IN_TEMPERAMENTS + 16);  // Room for custom

    for (int i = 0; i < NUM_BUILT_IN_TEMPERAMENTS; i++) {
        std::array<double, 12> ratios;
        for (int j = 0; j < 12; j++) {
            ratios[j] = builtInTemperaments[i][j];
        }

        m_temperaments.emplace_back(
            TEMPERAMENT_NAMES[i],
            "",  // No description for built-in
            ratios,
            false
        );
    }

    m_builtInCount = NUM_BUILT_IN_TEMPERAMENTS;
}

const Temperament& TemperamentManager::get(int index) const
{
    static Temperament defaultTemp;
    if (index < 0 || index >= static_cast<int>(m_temperaments.size())) {
        return defaultTemp;
    }
    return m_temperaments[index];
}

const std::string& TemperamentManager::getName(int index) const
{
    static std::string empty;
    if (index < 0 || index >= static_cast<int>(m_temperaments.size())) {
        return empty;
    }
    return m_temperaments[index].name;
}

std::vector<std::string> TemperamentManager::getNames() const
{
    std::vector<std::string> names;
    names.reserve(m_temperaments.size());
    for (const auto& t : m_temperaments) {
        names.push_back(t.name);
    }
    return names;
}

int TemperamentManager::findByName(const std::string& name) const
{
    auto it = std::find_if(m_temperaments.begin(), m_temperaments.end(),
        [&name](const Temperament& t) { return t.name == name; });

    if (it != m_temperaments.end()) {
        return static_cast<int>(std::distance(m_temperaments.begin(), it));
    }
    return -1;
}

int TemperamentManager::addCustom(const Temperament& temperament)
{
    Temperament custom = temperament;
    custom.isCustom = true;
    m_temperaments.push_back(custom);
    return static_cast<int>(m_temperaments.size()) - 1;
}

bool TemperamentManager::removeCustom(int index)
{
    if (index < m_builtInCount || index >= static_cast<int>(m_temperaments.size())) {
        return false;  // Can't remove built-in
    }

    m_temperaments.erase(m_temperaments.begin() + index);
    return true;
}

bool TemperamentManager::updateCustom(int index, const Temperament& temperament)
{
    if (index < m_builtInCount || index >= static_cast<int>(m_temperaments.size())) {
        return false;  // Can't modify built-in
    }

    m_temperaments[index] = temperament;
    m_temperaments[index].isCustom = true;
    return true;
}

} // namespace ctuner
