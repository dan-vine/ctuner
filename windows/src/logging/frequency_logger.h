////////////////////////////////////////////////////////////////////////////////
//
//  frequency_logger.h - CSV export and session logging
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FREQUENCY_LOGGER_H
#define FREQUENCY_LOGGER_H

#include "../app_state.h"
#include <string>
#include <vector>
#include <chrono>
#include <fstream>

namespace ctuner {

class FrequencyLogger {
public:
    FrequencyLogger();
    ~FrequencyLogger();

    // Start/stop logging
    void startSession();
    void stopSession();
    bool isLogging() const { return m_logging; }

    // Add entry
    void addEntry(const PitchResult& pitch);

    // Get entries
    const std::vector<LogEntry>& getEntries() const { return m_entries; }
    int getEntryCount() const { return static_cast<int>(m_entries.size()); }

    // Clear log
    void clear();

    // Export to CSV file
    bool exportCSV(const std::string& filename);

    // Export with auto-generated filename
    std::string exportCSVAuto(const std::string& directory = "");

    // Get session duration
    double getSessionDuration() const;

    // Get statistics
    struct Statistics {
        double averageFrequency = 0.0;
        double averageCents = 0.0;
        double maxCentsDeviation = 0.0;
        int totalNotes = 0;
        int uniqueNotes = 0;
    };
    Statistics getStatistics() const;

    // Get last error
    const std::string& getLastError() const { return m_lastError; }

private:
    bool m_logging = false;
    std::chrono::steady_clock::time_point m_sessionStart;
    std::vector<LogEntry> m_entries;
    std::string m_lastError;
};

} // namespace ctuner

#endif // FREQUENCY_LOGGER_H
