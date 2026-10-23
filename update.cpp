#include <iostream>
#include <cstdlib>
#include <filesystem>

int main() {
    std::string backendPath = "./redumper.exe"; 
    
    if (!std::filesystem::exists(backendPath)) {
        std::cout << "Redumper missing. Downloading...\n";
        

        std::string downloadCommand = "curl -L -o redumper.zip https://github.com/superg/redumper/releases/download/b767/redumper-b767-windows-x64.zip";
        
        int result = system(downloadCommand.c_str());
        
        if (result == 0) {
            std::cout << "Download complete. Extracting...\n";

            system("tar -xf redumper.zip");
        } else {
            std::cerr << "Failed to download backend.\n";
            return 1;
        }
    }

    std::cout << "Redumper is ready. Launching UI...\n";
    
    return 0;
}