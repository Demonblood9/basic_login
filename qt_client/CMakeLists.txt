cmake_minimum_required(VERSION 3.16)
project(SecureLicenseClient VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt
find_package(Qt6 COMPONENTS Core Widgets Network QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets Network)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Source files
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# Create executable
if (Qt6_FOUND)
    qt_add_executable(SecureLicenseClient
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(SecureLicenseClient ${PROJECT_SOURCES})
endif()

# Link Qt libraries
if (Qt6_FOUND)
    target_link_libraries(SecureLicenseClient PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
    )
else()
    target_link_libraries(SecureLicenseClient PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::Network
    )
endif()

# Windows-specific settings
if(WIN32)
    set_target_properties(SecureLicenseClient PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # Add manifest file for admin privileges
    if(MSVC)
        # Create a resource file to embed the manifest
        set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
        file(WRITE ${RC_FILE} "1 RT_MANIFEST \"${CMAKE_CURRENT_SOURCE_DIR}/app.manifest\"\n")
        target_sources(SecureLicenseClient PRIVATE ${RC_FILE})
    endif()
endif()

# macOS-specific settings
if(APPLE)
    set_target_properties(SecureLicenseClient PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()

# Finalize Qt executable
if (Qt6_FOUND)
    qt_finalize_executable(SecureLicenseClient)
endif()

# Installation
install(TARGETS SecureLicenseClient
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
