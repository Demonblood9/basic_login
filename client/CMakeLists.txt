cmake_minimum_required(VERSION 3.10)
project(SecureLoginClient)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    # Add Windows subsystem flag for GUI application
    set(CMAKE_WIN32_EXECUTABLE ON)

    # Add source files
    add_executable(SecureLoginClient WIN32 main.cpp)

    # Link required Windows libraries
    target_link_libraries(SecureLoginClient
        winhttp
        comctl32
        gdi32
        user32
    )

    # Set Unicode support
    target_compile_definitions(SecureLoginClient PRIVATE UNICODE _UNICODE)

    # Set compiler flags
    if(MSVC)
        target_compile_options(SecureLoginClient PRIVATE /W4)
    else()
        target_compile_options(SecureLoginClient PRIVATE -Wall -Wextra)
    endif()

else()
    message(FATAL_ERROR "This project is designed for Windows only")
endif()

# Installation
install(TARGETS SecureLoginClient DESTINATION bin)
