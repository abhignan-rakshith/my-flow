cmake_minimum_required(VERSION 3.20)
project(wispr-flow VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS Core Gui Widgets Multimedia Network)
if(LINUX)
    list(APPEND QT_COMPONENTS DBus)
endif()

find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})

set(MACOSX_BUNDLE_BUNDLE_NAME "My Flow")
set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.myflow.app")

if(APPLE)
    set(APP_ICON_MACOS "${CMAKE_SOURCE_DIR}/resources/icons/AppIcon.icns")
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set(MACOSX_BUNDLE_ICON_FILE "AppIcon")
endif()

qt_add_executable(wispr-flow
    ${APP_ICON_MACOS}
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
)

if(LINUX)
    target_link_libraries(wispr-flow PRIVATE Qt6::DBus)
endif()

if(APPLE)
    set_target_properties(wispr-flow PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist.in"
    )
    target_link_libraries(wispr-flow PRIVATE
        "-framework Carbon"
        "-framework ApplicationServices"
    )

    # Sign with stable identity so Accessibility permission survives rebuilds
    add_custom_command(TARGET wispr-flow POST_BUILD
        COMMAND codesign --force --sign F9D8560F07810652449F775F1408D25B341248CF --deep
                "$<TARGET_BUNDLE_DIR:wispr-flow>"
        COMMENT "Signing wispr-flow.app with Apple Development identity"
    )
endif()
