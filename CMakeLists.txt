cmake_minimum_required(VERSION 3.18)
project(UniversalConsole)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# the redumper backend URL
set(REDUMPER_URL "https://github.com/superg/redumper/releases/download/b767/redumper-b767-windows-x64.zip")
set(DOWNLOAD_DEST "${CMAKE_CURRENT_BINARY_DIR}/redumper.zip")

# only download and extract if bin/redumper.exe isn't already in the build folder
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/bin/redumper.exe")
    message(STATUS "Downloading redumper backend...")
    file(DOWNLOAD ${REDUMPER_URL} ${DOWNLOAD_DEST} SHOW_PROGRESS)

    message(STATUS "Extracting redumper...")
    execute_process(
        COMMAND tar -xf ${DOWNLOAD_DEST} --strip-components=1
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    file(REMOVE ${DOWNLOAD_DEST})
endif()

include_directories(include)

add_executable(UniversalConsole 
    src/main.cpp
    src/redumperWrapper.cpp
)