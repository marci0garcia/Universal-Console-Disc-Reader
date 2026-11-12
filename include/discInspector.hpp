#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>

struct DiscInfo {
    bool isPlayStation = false;
    std::string consoleType = "Unknown"; // "PS1" or "PS2"
    std::string serialId = "";           // e.g., "SCUS_973.28"
    std::string cleanSerial = "";        // e.g., "SCUS-97328"
    std::string gameTitle = "";          // e.g., "Gran Turismo 4"
    std::string region = "Unknown";      // "NTSC-U (USA)", "PAL (Europe)", "NTSC-J (Japan)"
    std::string version = "1.00";        // Extracted from VER = ...
    std::string mediaType = "CD-ROM";    // "CD-ROM (Max 24x)" or "DVD-ROM (Max 8x)"
};

class DiscInspector {
public:
    static DiscInfo inspectDrive(std::string driveLetter) {
        DiscInfo info;
        if (driveLetter.empty()) return info;

        if (driveLetter.back() != ':') driveLetter += ":";
        std::string cnfPath = driveLetter + "/SYSTEM.CNF";

        std::error_code ec;
        if (!std::filesystem::exists(cnfPath, ec)) {
            return info;
        }

        std::ifstream file(cnfPath);
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("BOOT2") != std::string::npos) {
                info.isPlayStation = true;
                info.consoleType = "PS2";
                info.mediaType = "DVD/CD (Max 8x/24x)";
            } else if (line.find("BOOT") != std::string::npos) {
                info.isPlayStation = true;
                info.consoleType = "PS1";
                info.mediaType = "CD-ROM (Max 24x)";
            }

            if (info.isPlayStation && info.serialId.empty()) {
                size_t slashPos = line.find_last_of("\\/");
                size_t semiPos = line.find(';', slashPos);
                
                if (slashPos != std::string::npos) {
                    info.serialId = line.substr(slashPos + 1, semiPos - (slashPos + 1));
                    info.serialId.erase(info.serialId.find_last_not_of(" \r\n\t") + 1);

                    info.cleanSerial = info.serialId;
                    std::replace(info.cleanSerial.begin(), info.cleanSerial.end(), '_', '-');
                    info.cleanSerial.erase(
                        std::remove(info.cleanSerial.begin(), info.cleanSerial.end(), '.'), 
                        info.cleanSerial.end()
                    );

                    // Decode Region from Serial Prefix (SLUS/SCUS = USA, SLES/SCES = Europe, SLPS/SCPS = Japan)
                    if (info.cleanSerial.size() >= 4) {
                        char regChar = std::toupper(info.cleanSerial[2]);
                        if (regChar == 'U') info.region = "NTSC-U (North America)";
                        else if (regChar == 'E') info.region = "PAL (Europe / Australia)";
                        else if (regChar == 'P' || regChar == 'J') info.region = "NTSC-J (Japan)";
                        else if (regChar == 'A') info.region = "NTSC-J (Asia)";
                    }
                }
            }

            if (line.find("VER") != std::string::npos) {
                size_t eqPos = line.find('=');
                if (eqPos != std::string::npos) {
                    std::string ver = line.substr(eqPos + 1);
                    size_t start = ver.find_first_not_of(" \t");
                    size_t end = ver.find_last_not_of(" \r\n\t");
                    if (start != std::string::npos && end != std::string::npos) {
                        info.version = ver.substr(start, end - start + 1);
                    }
                }
            }
        }
        return info;
    }
};