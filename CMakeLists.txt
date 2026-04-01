cmake_minimum_required(VERSION 3.20)
project(wispr-flow VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia Network DBus)

qt_add_executable(wispr-flow
    src/main.cpp
    src/Application.h src/Application.cpp
    src/Settings.h src/Settings.cpp
    src/AudioCapture.h src/AudioCapture.cpp
    src/AudioEncoder.h src/AudioEncoder.cpp
    src/TranscriptionService.h
    src/WhisperApiService.h src/WhisperApiService.cpp
    src/GroqApiService.h src/GroqApiService.cpp
    src/GlobalHotkey.h src/GlobalHotkey.cpp
    src/TextInjector.h src/TextInjector.cpp
    src/TrayIcon.h src/TrayIcon.cpp
    src/OverlayWidget.h src/OverlayWidget.cpp
    src/SettingsDialog.h src/SettingsDialog.cpp
    resources/resources.qrc
)

target_link_libraries(wispr-flow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
    Qt6::DBus
)
