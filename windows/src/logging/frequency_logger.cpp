////////////////////////////////////////////////////////////////////////////////
//
//  frequency_logger.cpp - CSV export and session logging
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "frequency_logger.h"
#include <windows.h>
#include <shlobj.h>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <set>
#include <cmath>

namespace ctuner {

FrequencyLogger::FrequencyLogger()
{
    m_entries.reserve(10000);  // Pre-allocate for performance
}

FrequencyLogger::~FrequencyLogger()
{
    stopSession();
}

void FrequencyLogger::startSession()
{
    clear();
    m_sessionStart = std::chrono::steady_clock::now();
    m_logging = true;
}

void FrequencyLogger::stopSession()
{
    m_logging = false;
}

void FrequencyLogger::addEntry(const PitchResult& pitch)
{
    if (!m_logging || !pitch.valid) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - m_sessionStart).count();

    LogEntry entry;
    entry.timestamp = elapsed;
    entry.frequency = pitch.frequency;
    entry.note = pitch.note;
    entry.octave = pitch.octave;
    entry.cents = pitch.cents * 100.0;  // Convert to cents

    m_entries.push_back(entry);
}

void FrequencyLogger::clear()
{
    m_entries.clear();
    m_sessionStart = std::chrono::steady_clock::now();
}

bool FrequencyLogger::exportCSV(const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        m_lastError = "Failed to create file: " + filename;
        return false;
    }

    // Write header
    file << "timestamp,frequency,note,octave,cents\n";

    // Write entries
    for (const auto& entry : m_entries) {
        file << std::fixed << std::setprecision(3) << entry.timestamp << ",";
        file << std::fixed << std::setprecision(2) << entry.frequency << ",";
        file << NOTE_NAMES[entry.note % OCTAVE] << ",";
        file << entry.octave << ",";
        file << std::showpos << std::fixed << std::setprecision(1) << entry.cents;
        file << std::noshowpos << "\n";
    }

    return true;
}

std::string FrequencyLogger::exportCSVAuto(const std::string& directory)
{
    std::string dir = directory;

    // Use Documents folder if not specified
    if (dir.empty()) {
        char documentsPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, 0, documentsPath))) {
            dir = documentsPath;
        } else {
            dir = ".";
        }
    }

    // Generate filename with timestamp
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::ostringstream ss;
    ss << dir << "\\ctuner_log_";
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    ss << ".csv";

    std::string filename = ss.str();

    if (exportCSV(filename)) {
        return filename;
    }

    return "";
}

double FrequencyLogger::getSessionDuration() const
{
    if (m_entries.empty()) {
        return 0.0;
    }

    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - m_sessionStart).count();
}

FrequencyLogger::Statistics FrequencyLogger::getStatistics() const
{
    Statistics stats;

    if (m_entries.empty()) {
        return stats;
    }

    std::set<int> uniqueNotes;
    double sumFreq = 0.0;
    double sumCents = 0.0;
    double maxCents = 0.0;

    for (const auto& entry : m_entries) {
        sumFreq += entry.frequency;
        sumCents += std::fabs(entry.cents);
        uniqueNotes.insert(entry.note);

        if (std::fabs(entry.cents) > maxCents) {
            maxCents = std::fabs(entry.cents);
        }
    }

    stats.totalNotes = static_cast<int>(m_entries.size());
    stats.uniqueNotes = static_cast<int>(uniqueNotes.size());
    stats.averageFrequency = sumFreq / m_entries.size();
    stats.averageCents = sumCents / m_entries.size();
    stats.maxCentsDeviation = maxCents;

    return stats;
}

} // namespace ctuner
