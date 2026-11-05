#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

class GameDatabase {
public:
    void loadFromDirectory(const std::string& resourceDir) {
        loadYamlDatabase(resourceDir + "/GameIndex.yaml"); // PCSX2 (PS2)
        loadYamlDatabase(resourceDir + "/gamedb.yaml");    // DuckStation (PS1)
    }

    // Returns the human-readable game title, or the serial ID if not found
    std::string getGameTitle(std::string cleanSerial) const {
        std::transform(cleanSerial.begin(), cleanSerial.end(), cleanSerial.begin(), ::toupper);
        auto it = m_titles.find(cleanSerial);
        if (it != m_titles.end()) {
            return it->second;
        }
        return cleanSerial; // Fallback if unknown disc
    }

    // Strips illegal Windows filename characters (: * ? " < > | / \) for folder naming
    static std::string sanitizeForFilename(std::string title) {
        const std::string illegalChars = "\\/:*?\"<>|";
        for (char& c : title) {
            if (illegalChars.find(c) != std::string::npos) {
                c = '_';
            }
        }
        return title;
    }

    size_t count() const { return m_titles.size(); }

private:
    std::unordered_map<std::string, std::string> m_titles;

    void loadYamlDatabase(const std::string& filePath) {
        if (!std::filesystem::exists(filePath)) return;

        std::ifstream file(filePath);
        std::string line;
        std::string currentSerial;

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            // Top-level YAML key (no leading spaces) = Game Serial ID (e.g. "SCUS-97328:")
            if (!std::isspace(static_cast<unsigned char>(line[0]))) {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    currentSerial = line.substr(0, colonPos);
                    // Trim quotes or whitespace if present
                    currentSerial.erase(std::remove(currentSerial.begin(), currentSerial.end(), '\"'), currentSerial.end());
                    currentSerial.erase(currentSerial.find_last_not_of(" \r\n\t") + 1);
                    std::transform(currentSerial.begin(), currentSerial.end(), currentSerial.begin(), ::toupper);
                }
            } 
            // Indented line: check for "  name:" (PCSX2) or "  title:" (DuckStation)
            else if (!currentSerial.empty()) {
                size_t firstChar = line.find_first_not_of(" \t");
                if (firstChar == std::string::npos) continue;

                // Only match direct child properties (2 spaces indentation)
                if (firstChar <= 4 && (line.compare(firstChar, 5, "name:") == 0 || 
                                       line.compare(firstChar, 6, "title:") == 0)) {
                    size_t colonPos = line.find(':', firstChar);
                    std::string title = line.substr(colonPos + 1);

                    // Trim leading/trailing spaces and quotes
                    size_t start = title.find_first_not_of(" \t\"");
                    size_t end = title.find_last_not_of(" \r\n\t\"");
                    if (start != std::string::npos && end != std::string::npos && end >= start) {
                        m_titles[currentSerial] = title.substr(start, end - start + 1);
                    }
                    currentSerial.clear(); // Move on to next serial
                }
            }
        }
    }
};