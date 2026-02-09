cmake_minimum_required(VERSION 3.16)
project(CTuner VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# ImGui source files
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

# Application source files
set(APP_SOURCES
    src/main.cpp
    src/app_state.h
    src/audio/audio_capture.cpp
    src/audio/audio_capture.h
    src/audio/pitch_detector.cpp
    src/audio/pitch_detector.h
    src/audio/fft.cpp
    src/audio/fft.h
    src/tuning/temperament.cpp
    src/tuning/temperament.h
    src/tuning/built_in_temperaments.h
    src/tuning/custom_tunings.cpp
    src/tuning/custom_tunings.h
    src/ui/main_window.cpp
    src/ui/main_window.h
    src/ui/spectrum_view.cpp
    src/ui/spectrum_view.h
    src/ui/meter_view.cpp
    src/ui/meter_view.h
    src/ui/strobe_view.cpp
    src/ui/strobe_view.h
    src/ui/staff_view.cpp
    src/ui/staff_view.h
    src/ui/settings_panel.cpp
    src/ui/settings_panel.h
    src/ui/tuning_editor.cpp
    src/ui/tuning_editor.h
    src/ui/log_viewer.cpp
    src/ui/log_viewer.h
    src/logging/frequency_logger.cpp
    src/logging/frequency_logger.h
)

# Create executable
add_executable(CTuner WIN32
    ${APP_SOURCES}
    ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(CTuner PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Link libraries
target_link_libraries(CTuner PRIVATE
    d3d11
    dxgi
    d3dcompiler
    winmm
    dwmapi
)

# Copy data files to build directory
add_custom_command(TARGET CTuner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:CTuner>/data
)

# Install rules
install(TARGETS CTuner DESTINATION bin)
install(DIRECTORY data/ DESTINATION bin/data)
