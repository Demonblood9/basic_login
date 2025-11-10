cmake_minimum_required(VERSION 3.15)
project(SecureLicenseClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(SecureLicenseClient WIN32 main.cpp resource.h)

# Link libraries
target_link_libraries(SecureLicenseClient
    winhttp
    advapi32
    iphlpapi
    comctl32
)

# Set subsystem to Windows
if(MSVC)
    set_target_properties(SecureLicenseClient PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup"
    )
endif()
