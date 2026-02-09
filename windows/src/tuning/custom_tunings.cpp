////////////////////////////////////////////////////////////////////////////////
//
//  custom_tunings.cpp - Custom tuning load/save with JSON
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include "custom_tunings.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>

namespace ctuner {

CustomTunings::CustomTunings()
{
    // Default to %APPDATA%/CTuner/tunings/
    char appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath))) {
        m_directory = std::string(appDataPath) + "\\CTuner\\tunings";
    } else {
        m_directory = ".\\tunings";
    }
}

void CustomTunings::setDirectory(const std::string& path)
{
    m_directory = path;
}

bool CustomTunings::ensureDirectory()
{
    // Create parent directory first
    std::string parent = m_directory.substr(0, m_directory.rfind('\\'));
    CreateDirectoryA(parent.c_str(), nullptr);

    // Create tunings directory
    if (CreateDirectoryA(m_directory.c_str(), nullptr) ||
        GetLastError() == ERROR_ALREADY_EXISTS) {
        return true;
    }

    m_lastError = "Failed to create directory: " + m_directory;
    return false;
}

std::vector<Temperament> CustomTunings::loadAll()
{
    std::vector<Temperament> tunings;

    auto files = listFiles();
    for (const auto& file : files) {
        Temperament t;
        if (loadFile(file, t)) {
            tunings.push_back(t);
        }
    }

    return tunings;
}

std::vector<std::string> CustomTunings::listFiles()
{
    std::vector<std::string> files;

    std::string pattern = m_directory + "\\*.json";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                files.push_back(fd.cFileName);
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }

    return files;
}

bool CustomTunings::loadFile(const std::string& filename, Temperament& out)
{
    std::string fullPath = m_directory + "\\" + filename;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        m_lastError = "Failed to open file: " + fullPath;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    if (!parseJson(json, out)) {
        return false;
    }

    out.isCustom = true;
    return true;
}

bool CustomTunings::saveFile(const Temperament& temperament, const std::string& filename)
{
    if (!ensureDirectory()) {
        return false;
    }

    std::string fullPath = m_directory + "\\" + filename;

    std::ofstream file(fullPath);
    if (!file.is_open()) {
        m_lastError = "Failed to create file: " + fullPath;
        return false;
    }

    file << toJson(temperament);
    return true;
}

bool CustomTunings::deleteFile(const std::string& filename)
{
    std::string fullPath = m_directory + "\\" + filename;

    if (DeleteFileA(fullPath.c_str())) {
        return true;
    }

    m_lastError = "Failed to delete file: " + fullPath;
    return false;
}

std::string CustomTunings::generateFilename(const std::string& name)
{
    std::string filename;
    filename.reserve(name.size());

    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            filename += std::tolower(static_cast<unsigned char>(c));
        } else if (c == ' ' || c == '-' || c == '_') {
            if (!filename.empty() && filename.back() != '_') {
                filename += '_';
            }
        }
    }

    // Remove trailing underscore
    while (!filename.empty() && filename.back() == '_') {
        filename.pop_back();
    }

    if (filename.empty()) {
        filename = "custom_tuning";
    }

    return filename + ".json";
}

// Simple JSON parser (handles our specific format)
bool CustomTunings::parseJson(const std::string& json, Temperament& out)
{
    out = Temperament();

    // Find "name" field
    size_t namePos = json.find("\"name\"");
    if (namePos != std::string::npos) {
        size_t colonPos = json.find(':', namePos);
        size_t quoteStart = json.find('"', colonPos + 1);
        size_t quoteEnd = json.find('"', quoteStart + 1);
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            out.name = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
    }

    // Find "description" field
    size_t descPos = json.find("\"description\"");
    if (descPos != std::string::npos) {
        size_t colonPos = json.find(':', descPos);
        size_t quoteStart = json.find('"', colonPos + 1);
        size_t quoteEnd = json.find('"', quoteStart + 1);
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            out.description = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
    }

    // Find "ratios" array
    size_t ratiosPos = json.find("\"ratios\"");
    if (ratiosPos != std::string::npos) {
        size_t arrayStart = json.find('[', ratiosPos);
        size_t arrayEnd = json.find(']', arrayStart);

        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
            std::string arrayStr = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

            // Parse comma-separated values
            std::stringstream ss(arrayStr);
            std::string item;
            int index = 0;

            while (std::getline(ss, item, ',') && index < 12) {
                // Trim whitespace
                size_t start = item.find_first_not_of(" \t\n\r");
                size_t end = item.find_last_not_of(" \t\n\r");

                if (start != std::string::npos && end != std::string::npos) {
                    try {
                        out.ratios[index] = std::stod(item.substr(start, end - start + 1));
                    } catch (...) {
                        out.ratios[index] = 1.0;
                    }
                }
                index++;
            }

            if (index < 12) {
                m_lastError = "Ratios array has fewer than 12 elements";
                return false;
            }
        } else {
            m_lastError = "Invalid ratios array format";
            return false;
        }
    } else {
        m_lastError = "Missing ratios field";
        return false;
    }

    if (out.name.empty()) {
        m_lastError = "Missing name field";
        return false;
    }

    return true;
}

std::string CustomTunings::toJson(const Temperament& temperament)
{
    std::ostringstream ss;

    ss << "{\n";
    ss << "    \"name\": \"" << temperament.name << "\",\n";
    ss << "    \"description\": \"" << temperament.description << "\",\n";
    ss << "    \"ratios\": [\n        ";

    for (int i = 0; i < 12; i++) {
        ss.precision(9);
        ss << std::fixed << temperament.ratios[i];
        if (i < 11) {
            ss << ", ";
            if (i == 5) ss << "\n        ";  // Line break for readability
        }
    }

    ss << "\n    ]\n";
    ss << "}\n";

    return ss.str();
}

} // namespace ctuner
