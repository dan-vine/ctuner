////////////////////////////////////////////////////////////////////////////////
//
//  custom_tunings.h - Custom tuning load/save with JSON
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CUSTOM_TUNINGS_H
#define CUSTOM_TUNINGS_H

#include "temperament.h"
#include <string>
#include <vector>

namespace ctuner {

// Custom tuning file operations
class CustomTunings {
public:
    CustomTunings();
    ~CustomTunings() = default;

    // Set the directory for custom tunings
    void setDirectory(const std::string& path);

    // Get the directory path
    const std::string& getDirectory() const { return m_directory; }

    // Load all custom tunings from directory
    std::vector<Temperament> loadAll();

    // Load a single tuning from file
    bool loadFile(const std::string& filename, Temperament& out);

    // Save a tuning to file
    bool saveFile(const Temperament& temperament, const std::string& filename);

    // Delete a tuning file
    bool deleteFile(const std::string& filename);

    // Get list of tuning files in directory
    std::vector<std::string> listFiles();

    // Generate filename from tuning name
    static std::string generateFilename(const std::string& name);

    // Get last error message
    const std::string& getLastError() const { return m_lastError; }

private:
    // Create directory if it doesn't exist
    bool ensureDirectory();

    // Simple JSON parsing (avoiding external dependency)
    bool parseJson(const std::string& json, Temperament& out);
    std::string toJson(const Temperament& temperament);

    std::string m_directory;
    std::string m_lastError;
};

} // namespace ctuner

#endif // CUSTOM_TUNINGS_H
