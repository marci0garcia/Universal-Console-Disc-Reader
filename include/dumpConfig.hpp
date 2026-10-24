#pragma once
#include <string>

// redumper settings
struct DumpConfig {
    std::string drive = "";         
    int retries = 50;                // Sector retry count
    std::string imagePath = "dumps"; // Folder where BIN/CUE/ISO files go
    std::string imageName = "disc_dump";
    bool overwrite = false;
    bool forceSplit = false;
};