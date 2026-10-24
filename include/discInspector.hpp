#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>

struct DiscInfo {
    bool isPlayStation = false;
    std::string consoleType = "Unknown"; 
    std::string serialId = "";           
    std::string cleanSerial = "";        
};

class DiscInspector {
public:
    static DiscInfo inspectDrive(std::string driveLetter) {
        DiscInfo info;
        if (driveLetter.empty()) return info;

        if (driveLetter.back() != ':') driveLetter += ":";
        std::string cnfPath = driveLetter + "/SYSTEM.CNF";

        // Check if SYSTEM.CNF exists on the disc
        if (!std::filesystem::exists(cnfPath)) {
            return info;
        }

        std::ifstream file(cnfPath);
        std::string line;
        while (std::getline(file, line)) {
            // Determine PS1 or PS2
            if (line.find("BOOT2") != std::string::npos) {
                info.isPlayStation = true;
                info.consoleType = "PS2";
            } else if (line.find("BOOT") != std::string::npos) {
                info.isPlayStation = true;
                info.consoleType = "PS1";
            }

            // Extract the executable code
            if (info.isPlayStation) {
                size_t slashPos = line.find_last_of("\\/");
                size_t semiPos = line.find(';', slashPos);
                
                if (slashPos != std::string::npos) {
                    info.serialId = line.substr(slashPos + 1, semiPos - (slashPos + 1));
                    
                    // Trim
                    info.serialId.erase(info.serialId.find_last_not_of(" \r\n\t") + 1);

                    // Create clean Serial
                    info.cleanSerial = info.serialId;
                    std::replace(info.cleanSerial.begin(), info.cleanSerial.end(), '_', '-');
                    info.cleanSerial.erase(
                        std::remove(info.cleanSerial.begin(), info.cleanSerial.end(), '.'), 
                        info.cleanSerial.end()
                    );
                    break;
                }
            }
        }
        return info;
    }
};