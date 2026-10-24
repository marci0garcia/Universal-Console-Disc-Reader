#pragma once
#include "dumpConfig.hpp"
#include <functional>
#include <string>

class RedumperWrapper {
public:
    explicit RedumperWrapper(const std::string& binaryPath = "./redumper.exe");

    // Runs 'redumper disc' and streams each line of output to onOutputLine
    int dumpDisc(const DumpConfig& config, std::function<void(const std::string&)> onOutputLine);

    // Runs 'redumper --list-all-drives'
    int listDrives(std::function<void(const std::string&)> onOutputLine);

private:
    std::string m_binaryPath;
    int executeCommand(const std::string& args, std::function<void(const std::string&)> onOutputLine);
};