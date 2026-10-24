#include "redumperWrapper.hpp"
#include "discInspector.hpp"
#include <iostream>
#include <filesystem>

int main() {
    std::string drive = "D:"; // Change this to optical drive letter

    std::cout << "Checking disc in drive " + drive + "...\n";
    DiscInfo disc = DiscInspector::inspectDrive(drive);

    if (!disc.isPlayStation) {
        std::cout << "No valid PS1 or PS2 disc detected in " << drive << "\n";
        return 1;
    }

    std::cout << "Detected Console : " << disc.consoleType << "\n";
    std::cout << "Detected Game ID : " << disc.serialId << " (" << disc.cleanSerial << ")\n";

    // Check if game already dumped
    // PS1 dump make .cue/.bin, PS2 DVD dumps create an .iso or .cue for CD based
    std::string dumpFolder = "./dumps/" + disc.cleanSerial;
    if (std::filesystem::exists(dumpFolder + "/" + disc.cleanSerial + ".iso") ||
        std::filesystem::exists(dumpFolder + "/" + disc.cleanSerial + ".cue")) {
        
        std::cout << "\n[SKIP] Game " << disc.cleanSerial << " is already dumped in " << dumpFolder << "\n";
        return 0;
    }

    // If not dumped yet, configure redumper using the game's Serial ID
    std::string redumperPath = std::filesystem::exists("./bin/redumper.exe") 
                               ? "./bin/redumper.exe" 
                               : "./build/bin/redumper.exe";

    RedumperWrapper dumper(redumperPath);

    DumpConfig config;
    config.drive = drive;
    config.retries = 50;
    config.imagePath = dumpFolder;
    config.imageName = disc.cleanSerial;
    config.overwrite = false;

    std::cout << "\nStarting disc dump for " << disc.cleanSerial << "...\n";

    int exitCode = dumper.dumpDisc(config, [](const std::string& line) {
        std::cout << "[REDUMPER] " << line;
    });

    std::cout << "\nDump finished with exit code: " << exitCode << "\n";
    return 0;
}