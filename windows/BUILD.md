# CTuner Build Instructions

## Prerequisites

- CMake 3.16 or later
- Visual Studio 2019 or later (with C++ workload)
- Windows SDK (comes with Visual Studio)

## Getting Dear ImGui

Before building, you need to download Dear ImGui:

1. Clone or download ImGui from https://github.com/ocornut/imgui
2. Copy the following files to the `imgui/` directory:
   - `imgui.cpp`
   - `imgui.h`
   - `imgui_demo.cpp`
   - `imgui_draw.cpp`
   - `imgui_internal.h`
   - `imgui_tables.cpp`
   - `imgui_widgets.cpp`
   - `imconfig.h`
   - `imstb_rectpack.h`
   - `imstb_textedit.h`
   - `imstb_truetype.h`
3. Copy the backend files to `imgui/backends/`:
   - `backends/imgui_impl_win32.cpp`
   - `backends/imgui_impl_win32.h`
   - `backends/imgui_impl_dx11.cpp`
   - `backends/imgui_impl_dx11.h`

Or run the provided script:
```
setup_imgui.bat
```

## Building with CMake

### Using Command Line

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release
```

### Using Visual Studio

1. Open Visual Studio
2. Select "Open a local folder"
3. Navigate to the `windows` directory
4. Visual Studio will detect CMakeLists.txt and configure automatically
5. Select the Release configuration
6. Build -> Build All

### Using CMake GUI

1. Open CMake GUI
2. Set source directory to the `windows` folder
3. Set build directory to `windows/build`
4. Click Configure, then Generate
5. Click Open Project to open in Visual Studio
6. Build the solution

## Running

The built executable will be in:
- `build/Release/CTuner.exe` (Release build)
- `build/Debug/CTuner.exe` (Debug build)

## Project Structure

```
windows/
├── CMakeLists.txt          # Build configuration
├── BUILD.md                # This file
├── imgui/                  # Dear ImGui library (download required)
│   ├── *.cpp, *.h
│   └── backends/
├── src/
│   ├── main.cpp           # Application entry point
│   ├── app_state.h        # Centralized state
│   ├── audio/
│   │   ├── audio_capture.*  # Windows audio input
│   │   ├── pitch_detector.* # Pitch detection
│   │   └── fft.*           # FFT algorithm
│   ├── tuning/
│   │   ├── temperament.*    # Temperament handling
│   │   ├── built_in_temperaments.h
│   │   └── custom_tunings.* # JSON custom tunings
│   ├── ui/
│   │   ├── main_window.*    # Main UI
│   │   ├── spectrum_view.*  # Spectrum display
│   │   ├── meter_view.*     # Cent meter
│   │   ├── strobe_view.*    # Strobe display
│   │   ├── staff_view.*     # Staff notation
│   │   ├── settings_panel.* # Options
│   │   └── tuning_editor.*  # Custom tuning editor
│   └── logging/
│       └── frequency_logger.* # CSV export
└── data/
    └── custom_tunings/     # User tuning files
```

## Features

- Real-time pitch detection with FFT
- 32 built-in temperaments
- Custom tuning support (JSON format)
- Spectrum, strobe, and staff displays
- Frequency logging with CSV export
- Keyboard shortcuts (Z, F, D, L, M, S, +, -)

## Keyboard Shortcuts

- `Z` - Toggle spectrum zoom
- `F` - Toggle audio filter
- `D` - Toggle downsample
- `L` - Lock display
- `M` - Multiple notes mode
- `S` - Toggle strobe/staff
- `+` - Expand spectrum
- `-` - Contract spectrum
- `O` - Open options (via menu)
