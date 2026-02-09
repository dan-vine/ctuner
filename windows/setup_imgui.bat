@echo off
REM Setup script to download Dear ImGui for CTuner

echo Downloading Dear ImGui...

REM Create imgui directory
if not exist imgui mkdir imgui
if not exist imgui\backends mkdir imgui\backends

REM Set version
set IMGUI_VERSION=v1.90.1

REM Download core files
echo Downloading core ImGui files...
curl -L -o imgui\imgui.cpp "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imgui.cpp"
curl -L -o imgui\imgui.h "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imgui.h"
curl -L -o imgui\imgui_demo.cpp "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imgui_demo.cpp"
curl -L -o imgui\imgui_draw.cpp "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imgui_draw.cpp"
curl -L -o imgui\imgui_internal.h "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imgui_internal.h"
curl -L -o imgui\imgui_tables.cpp "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imgui_tables.cpp"
curl -L -o imgui\imgui_widgets.cpp "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imgui_widgets.cpp"
curl -L -o imgui\imconfig.h "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imconfig.h"
curl -L -o imgui\imstb_rectpack.h "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imstb_rectpack.h"
curl -L -o imgui\imstb_textedit.h "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imstb_textedit.h"
curl -L -o imgui\imstb_truetype.h "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/imstb_truetype.h"

REM Download backend files
echo Downloading backend files...
curl -L -o imgui\backends\imgui_impl_win32.cpp "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/backends/imgui_impl_win32.cpp"
curl -L -o imgui\backends\imgui_impl_win32.h "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/backends/imgui_impl_win32.h"
curl -L -o imgui\backends\imgui_impl_dx11.cpp "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/backends/imgui_impl_dx11.cpp"
curl -L -o imgui\backends\imgui_impl_dx11.h "https://raw.githubusercontent.com/ocornut/imgui/%IMGUI_VERSION%/backends/imgui_impl_dx11.h"

echo.
echo Dear ImGui setup complete!
echo.
echo Next steps:
echo   1. mkdir build
echo   2. cd build
echo   3. cmake ..
echo   4. cmake --build . --config Release
echo.
pause
