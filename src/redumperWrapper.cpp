#include "redumperWrapper.hpp"
#include <cstdio>
#include <iostream>
#include <filesystem>

// Cross-platform pipe macros
#ifdef _WIN32
    #define POPEN _popen
    #define PCLOSE _pclose
#else
    #define POPEN popen
    #define PCLOSE pclose
#endif

RedumperWrapper::RedumperWrapper(const std::string& binaryPath) 
    : m_binaryPath(binaryPath) {}

int RedumperWrapper::dumpDisc(const DumpConfig& config, std::function<void(const std::string&)> onOutputLine) {
    // Ensure the output directory exists before redumper tries to write to it
    if (!config.imagePath.empty()) {
        std::filesystem::create_directories(config.imagePath);
    }

    // Build the arguments based on Redumper documentation
    std::string args = "disc";
    
    if (!config.drive.empty()) {
        args += " --drive=" + config.drive;
    }
    args += " --retries=" + std::to_string(config.retries);
    args += " --image-path=\"" + config.imagePath + "\"";
    args += " --image-name=\"" + config.imageName + "\"";
    
    if (config.overwrite) {
        args += " --overwrite";
    }

    return executeCommand(args, onOutputLine);
}

int RedumperWrapper::listDrives(std::function<void(const std::string&)> onOutputLine) {
    return executeCommand("--list-all-drives", onOutputLine);
}

int RedumperWrapper::executeCommand(const std::string& args, std::function<void(const std::string&)> onOutputLine) {
    std::string exePath = std::filesystem::path(m_binaryPath).make_preferred().string();

    // Verify the binary exists before attempting to launch it
    if (!std::filesystem::exists(exePath)) {
        if (onOutputLine) {
            onOutputLine("[ERROR] Could not find redumper executable at: " + 
                         std::filesystem::absolute(exePath).string() + "\n");
        }
        return -1;
    }

    std::string innerCommand = "\"" + exePath + "\" " + args + " 2>&1";

#ifdef _WIN32
    std::string fullCommand = "\"" + innerCommand + "\"";
#else
    std::string fullCommand = innerCommand;
#endif

    FILE* pipe = POPEN(fullCommand.c_str(), "r");
    if (!pipe) {
        if (onOutputLine) onOutputLine("[ERROR] Failed to launch redumper process.\n");
        return -1;
    }

    // Read output live as redumper prints it
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        if (onOutputLine) {
            onOutputLine(std::string(buffer));
        }
    }

    return PCLOSE(pipe);
}