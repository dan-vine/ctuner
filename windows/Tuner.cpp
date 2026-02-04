////////////////////////////////////////////////////////////////////////////////
//
//  Tuner - A Tuner written in C++.
//
//  Copyright (C) 2009  Bill Farmer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//  Bill Farmer  william j farmer [at] yahoo [dot] co [dot] uk.
//
///////////////////////////////////////////////////////////////////////////////

#include "Tuner.h"

// Global variable definitions
HINSTANCE hInst;
ULONG_PTR token;
Gdiplus::GdiplusStartupInput input;

WINDOW window;
WINDOW options;
WINDOW filters;

TOOL toolbar;
TOOLTIP tooltip;
SCOPE scope;
SPECTRUM spectrum;
DISPLAY display;
STROBE strobe;
STAFF staff;
METER meter;

BUTTON button;

TOOL key;
TOOL zoom;
TOOL text;
TOOL lock;
TOOL down;
TOOL mult;
TOOL fund;
TOOL note;
TOOL filt;
TOOL group;
TOOL enable;
TOOL expand;
TOOL updown;
TOOL colours;
TOOL transpose;
TOOL reference;
TOOL temperament;

BOXES boxes;
AUDIO audio;
FILTER filter;

// Temperaments data
double temperaments[32][12] =
#include "Temperaments.h"

// Application entry point.
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine,
                   int nCmdShow)
{
    // Check for a previous instance of this app
    if (!hPrevInstance)
        if (!RegisterMainClass(hInstance))
            return false;

    // Save the application-instance handle.
    hInst = hInstance;

    // Initialize common controls to get the new style controls, also
    // dependent on manifest file
    InitCommonControls();

    // Start Gdiplus
    GdiplusStartup(&token, &input, NULL);

    // Get saved status
    GetSavedStatus();

    // Create the main window.
    window.hwnd =
        CreateWindow(WCLASS, "Tuner",
                     WS_OVERLAPPED | WS_MINIMIZEBOX |
                     WS_SIZEBOX | WS_SYSMENU,
                     CW_USEDEFAULT, CW_USEDEFAULT,
                     CW_USEDEFAULT, CW_USEDEFAULT,
                     NULL, 0, hInst, NULL);

    // If the main window cannot be created, terminate
    // the application.
    if (!window.hwnd)
        return false;

    // Show the window and send a WM_PAINT message to the window
    // procedure.
    ShowWindow(window.hwnd, nCmdShow);
    UpdateWindow(window.hwnd);

    // Process messages
    MSG msg;
    BOOL flag;

    while ((flag = GetMessage(&msg, (HWND)NULL, 0, 0)) != 0)
    {
        if (flag == -1)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

// Register class
BOOL RegisterMainClass(HINSTANCE hInst)
{
    // Fill in the window class structure with parameters
    // that describe the main window.
    WNDCLASS wc = 
        {CS_HREDRAW | CS_VREDRAW,
         MainWndProc, 0, 0, hInst,
         LoadIcon(hInst, "Tuner"),
         LoadCursor(NULL, IDC_ARROW),
         GetSysColorBrush(COLOR_WINDOW),
         NULL, WCLASS};

    // Register the window class.
    return RegisterClass(&wc);
}

VOID GetSavedStatus()
{
    HKEY hkey;
    LONG error;
    DWORD value;
    int size = sizeof(value);

    // Initial values
    audio.filter = false;
    audio.reference = A5_REFNCE;
    audio.temperament = 8;
    display.transpose = 0;
    spectrum.expand = 1;
    spectrum.zoom = true;
    staff.enable = true;
    strobe.colours = 1;
    strobe.enable = false;

    // Open user key
    error = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\CTuner", 0,
                         KEY_READ, &hkey);

    if (error == ERROR_SUCCESS)
    {
        // Spectrum zoom
        error = RegQueryValueEx(hkey, "Zoom", NULL, NULL,
                                (LPBYTE)&value, (LPDWORD)&size);
        // Update value
        if (error == ERROR_SUCCESS)
            spectrum.zoom = value;

        // Strobe enable
        error = RegQueryValueEx(hkey, "Strobe", NULL, NULL,
                                (LPBYTE)&value,(LPDWORD)&size);
        // Update value
        if (error == ERROR_SUCCESS)
        {
            strobe.enable = value;
            staff.enable = !value;
        }

        // Strobe colours
        error = RegQueryValueEx(hkey, "Colours", NULL, NULL,
                                (LPBYTE)&value,(LPDWORD)&size);
        // Update value
        if (error == ERROR_SUCCESS)
            strobe.colours = value;

        // Filter
        error = RegQueryValueEx(hkey, "Filter", NULL, NULL,
                                (LPBYTE)&value,(LPDWORD)&size);
        // Update value
        if (error == ERROR_SUCCESS)
            audio.filter = value;

        // Reference
        error = RegQueryValueEx(hkey, "Reference", NULL, NULL,
                                (LPBYTE)&value,(LPDWORD)&size);
        // Update value
        if (error == ERROR_SUCCESS)
            audio.reference = value / 10.0;

        // Close key
        RegCloseKey(hkey);
    }
}

// Main window procedure
LRESULT CALLBACK MainWndProc(HWND hWnd,
                             UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        {
            // Create toolbar
            toolbar.hwnd =
                CreateWindow(TOOLBARCLASSNAME, NULL,
                             WS_VISIBLE | WS_CHILD | TBSTYLE_TOOLTIPS,
                             0, 0, 0, 0,
                             hWnd, (HMENU)TOOLBAR_ID, hInst, NULL);

            SendMessage(toolbar.hwnd, TB_BUTTONSTRUCTSIZE, 
                        (WPARAM)sizeof(TBBUTTON), 0);

            SendMessage(toolbar.hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(24, 24));
            SendMessage(toolbar.hwnd, TB_SETMAXTEXTROWS, 0, 0);

            // Add bitmap
            AddToolbarBitmap(toolbar.hwnd, "Toolbar");

            // Add buttons
            AddToolbarButtons(toolbar.hwnd);

            // Resize toolbar
            SendMessage(toolbar.hwnd, TB_AUTOSIZE, 0, 0); 
            GetWindowRect(toolbar.hwnd, &toolbar.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&toolbar.rect, 2);

            // Get the window and client dimensions
            GetWindowRect(hWnd, &window.wind);
            GetClientRect(hWnd, &window.rect);

            // Calculate desired window width and height
            int border = (window.wind.right - window.wind.left) -
                window.rect.right;
            int header = (window.wind.bottom - window.wind.top) -
                window.rect.bottom;
            int width  = WIDTH + border;
            int height = HEIGHT + toolbar.rect.bottom + header;

            // Set new dimensions
            SetWindowPos(hWnd, NULL, 0, 0,
                         width, height,
                         SWP_NOMOVE | SWP_NOZORDER);

            // Get client dimensions
            GetWindowRect(hWnd, &window.wind);
            GetClientRect(hWnd, &window.rect);

            width = window.rect.right;
            height = window.rect.bottom;

            // Create tooltip
            tooltip.hwnd =
                CreateWindow(TOOLTIPS_CLASS, NULL,
                             WS_POPUP | TTS_ALWAYSTIP,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             hWnd, NULL, hInst, NULL);

            SetWindowPos(tooltip.hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            tooltip.info.cbSize = sizeof(tooltip.info);
            tooltip.info.hwnd = hWnd;
            tooltip.info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;

            // Create scope display
            scope.hwnd =
                CreateWindow(WC_STATIC, NULL,
                             WS_VISIBLE | WS_CHILD |
                             SS_NOTIFY | SS_OWNERDRAW,
                             MARGIN, MARGIN, width - MARGIN * 2,
                             SCOPE_HEIGHT, hWnd,
                             (HMENU)SCOPE_ID, hInst, NULL);
            GetWindowRect(scope.hwnd, &scope.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&scope.rect, 2);

            // Add scope to tooltip
            tooltip.info.uId = (UINT_PTR)scope.hwnd;
            tooltip.info.lpszText = (LPSTR)"Scope, click to filter audio";

            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create spectrum display
            spectrum.hwnd =
                CreateWindow(WC_STATIC, NULL,
                             WS_VISIBLE | WS_CHILD |
                             SS_NOTIFY | SS_OWNERDRAW,
                             MARGIN, scope.rect.bottom + SPACING,
                             width - MARGIN * 2,
                             SPECTRUM_HEIGHT, hWnd,
                             (HMENU)SPECTRUM_ID, hInst, NULL);
            GetWindowRect(spectrum.hwnd, &spectrum.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&spectrum.rect, 2);

            // Add spectrum to tooltip
            tooltip.info.uId = (UINT_PTR)spectrum.hwnd;
            tooltip.info.lpszText = (LPSTR)"Spectrum, click to zoom";

            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create display
            display.hwnd =
                CreateWindow(WC_STATIC, NULL,
                             WS_VISIBLE | WS_CHILD |
                             SS_NOTIFY | SS_OWNERDRAW,
                             MARGIN, spectrum.rect.bottom + SPACING,
                             width - MARGIN * 2, DISPLAY_HEIGHT, hWnd,
                             (HMENU)DISPLAY_ID, hInst, NULL);
            GetWindowRect(display.hwnd, &display.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&display.rect, 2);

            // Add display to tooltip
            tooltip.info.uId = (UINT_PTR)display.hwnd;
            tooltip.info.lpszText = (LPSTR)"Display, click to lock";

            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create strobe
            strobe.hwnd =
                CreateWindow(WC_STATIC, NULL,
                             WS_VISIBLE | WS_CHILD |
                             SS_NOTIFY | SS_OWNERDRAW,
                             MARGIN, display.rect.bottom + SPACING,
                             width - MARGIN * 2, STROBE_HEIGHT, hWnd,
                             (HMENU)STROBE_ID, hInst, NULL);
            GetWindowRect(strobe.hwnd, &strobe.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&strobe.rect, 2);

            // Create tooltip for strobe
            tooltip.info.uId = (UINT_PTR)strobe.hwnd;
            tooltip.info.lpszText = (LPSTR)"Strobe, click to disable/enable";

            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Show window
            ShowWindow(strobe.hwnd, strobe.enable? SW_SHOW: SW_HIDE);

            // Create staff
            staff.hwnd =
                CreateWindow(WC_STATIC, NULL,
                             WS_VISIBLE | WS_CHILD |
                             SS_NOTIFY | SS_OWNERDRAW,
                             MARGIN, display.rect.bottom + SPACING,
                             width - MARGIN * 2, STAFF_HEIGHT, hWnd,
                             (HMENU)STAFF_ID, hInst, NULL);
            GetWindowRect(staff.hwnd, &staff.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&staff.rect, 2);

            // Create tooltip for staff
            tooltip.info.uId = (UINT_PTR)staff.hwnd;
            tooltip.info.lpszText = (LPSTR)"Staff, click to disable/enable";

            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Show window
            ShowWindow(staff.hwnd, staff.enable? SW_SHOW: SW_HIDE);

            // Create meter
            meter.hwnd =
                CreateWindow(WC_STATIC, NULL,
                             WS_VISIBLE | WS_CHILD |
                             SS_NOTIFY | SS_OWNERDRAW,
                             MARGIN, strobe.rect.bottom + SPACING,
                             width - MARGIN * 2, METER_HEIGHT, hWnd,
                             (HMENU)METER_ID, hInst, NULL);
            GetWindowRect(meter.hwnd, &meter.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&meter.rect, 2);

            // Add meter to tooltip
            tooltip.info.uId = (UINT_PTR)meter.hwnd;
            tooltip.info.lpszText = (LPSTR)"Cents, click to lock";

            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Find font
            HRSRC hRes = FindResource(hInst, "Musica", RT_FONT);

            if (hRes) 
            {
                // Load font
                HGLOBAL mem = LoadResource(hInst, hRes);
                void *data = LockResource(mem);
                size_t size = SizeofResource(hInst, hRes);

                DWORD n;
                // Add font
                AddFontMemResourceEx(data, size, 0, &n);
            }

            // Start audio thread
            audio.thread = CreateThread(NULL, 0, AudioThread, hWnd,
                                        0, &audio.id);

            // Start meter timer
            CreateTimerQueueTimer(&meter.timer, NULL,
                                  (WAITORTIMERCALLBACK)MeterCallback,
                                  &meter.hwnd, METER_DELAY, METER_DELAY,
                                  WT_EXECUTEDEFAULT);

            // Start strobe timer
            CreateTimerQueueTimer(&strobe.timer, NULL,
                                  (WAITORTIMERCALLBACK)StrobeCallback,
                                  &strobe.hwnd, STROBE_DELAY, STROBE_DELAY,
                                  WT_EXECUTEDEFAULT);
        }
        break;

        // Colour static text
    case WM_CTLCOLORSTATIC:
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        break;

        // Draw item
    case WM_DRAWITEM:
        return DrawItem(wParam, lParam);
        break;

        // Disable menus by capturing this message
    case WM_INITMENU:
        break;

        // Capture system character key to stop pop up menus and other
        // nonsense
    case WM_SYSCHAR:
        break;

        // Set the focus back to the window by clicking
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
        SetFocus(hWnd);
        break;

        // Context menu
    case WM_RBUTTONDOWN:
        DisplayContextMenu(hWnd, MAKEPOINTS(lParam));
        break;

        // Buttons
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            // Scope
        case SCOPE_ID:
            ScopeClicked(wParam, lParam);
            break;

            // Display
        case DISPLAY_ID:
            DisplayClicked(wParam, lParam);
            break;

            // Spectrum
        case SPECTRUM_ID:
            SpectrumClicked(wParam, lParam);
            break;

            // Strobe
        case STROBE_ID:
            StrobeClicked(wParam, lParam);
            break;

            // Staff
        case STAFF_ID:
            StaffClicked(wParam, lParam);
            break;

            // Meter
        case METER_ID:
            MeterClicked(wParam, lParam);
            break;

            // Zoom control
        case ZOOM_ID:
            ZoomClicked(wParam, lParam);
            break;

            // Enable control
        case ENABLE_ID:
            EnableClicked(wParam, lParam);
            break;

            // Filter control
        case FILTER_ID:
            FilterClicked(wParam, lParam);
            break;

            // Downsample control
        case DOWN_ID:
            DownClicked(wParam, lParam);
            break;

            // Lock control
        case LOCK_ID:
            LockClicked(wParam, lParam);
            break;

            // Multiple control
        case MULT_ID:
            MultipleClicked(wParam, lParam);
            break;

            // Options
        case OPTIONS_ID:
            DisplayOptions(wParam, lParam);
            break;

            // Quit
        case QUIT_ID:
            Gdiplus::GdiplusShutdown(token);
            waveInStop(audio.hwi);
            waveInClose(audio.hwi);
            PostQuitMessage(0);
            break;
        }

        // Set the focus back to the window
        SetFocus(hWnd);
        break;

        // Char pressed
    case WM_CHAR:
        CharPressed(wParam, lParam);
        break;

        // Audio input data
    case MM_WIM_DATA:
        WaveInData(wParam, lParam);
        break;

        // Size
    case WM_SIZE:
        WindowResize(hWnd, wParam, lParam);
        break;

        // Sizing
    case WM_SIZING:
        return WindowResizing(hWnd, wParam, lParam);
        break;

        // Process other messages.
    case WM_DESTROY:
        Gdiplus::GdiplusShutdown(token);
        waveInStop(audio.hwi);
        waveInClose(audio.hwi);
        PostQuitMessage(0);
        break;

        // Everything else
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

// WindowResize
BOOL WindowResize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    int width = LOWORD(lParam);
    int height = HIWORD(lParam) - toolbar.rect.bottom;

    // Get the window and client dimensions
    GetWindowRect(hWnd, &window.wind);
    GetClientRect(hWnd, &window.rect);

    if (width < (height * WIDTH) / HEIGHT)
    {
        // Calculate desired window width and height
        int border = (window.wind.right - window.wind.left) -
            window.rect.right;
        int header = (window.wind.bottom - window.wind.top) -
            window.rect.bottom;
        width = ((height * WIDTH) / HEIGHT) + border;
        height = height + toolbar.rect.bottom + header;

        // Set new dimensions
        SetWindowPos(hWnd, NULL, 0, 0,
                     width, height,
                     SWP_NOMOVE | SWP_NOZORDER);
        return true;
    }

    EnumChildWindows(hWnd, EnumChildProc, lParam);
    return true;
}

// Enum child proc for window resizing
BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam)
{
    int width = LOWORD(lParam);
    int height = HIWORD(lParam) - toolbar.rect.bottom;

    // Switch by id to resize tool windows.
    switch ((DWORD)GetWindowLongPtr(hWnd, GWLP_ID))
    {
        // Toolbar, let it resize itself
    case TOOLBAR_ID:
        SendMessage(hWnd, WM_SIZE, 0, lParam);
        GetWindowRect(hWnd, &toolbar.rect);
        MapWindowPoints(NULL, window.hwnd, (POINT *)&toolbar.rect, 2);
        break;

        // Scope, resize it
    case SCOPE_ID:
        MoveWindow(hWnd, MARGIN, toolbar.rect.bottom + MARGIN,
                   width - MARGIN * 2,
                   (height - TOTAL) * SCOPE_HEIGHT / TOTAL_HEIGHT,
                   false);
        InvalidateRgn(hWnd, NULL, true);
        GetWindowRect(hWnd, &scope.rect);
        MapWindowPoints(NULL, window.hwnd, (POINT *)&scope.rect, 2);
        break;

        // Spectrum, resize it
    case SPECTRUM_ID:
        MoveWindow(hWnd, MARGIN, scope.rect.bottom + SPACING,
                   width - MARGIN * 2,
                   (height - TOTAL) * SPECTRUM_HEIGHT / TOTAL_HEIGHT,
                   false);
        InvalidateRgn(hWnd, NULL, true);
        GetWindowRect(hWnd, &spectrum.rect);
        MapWindowPoints(NULL, window.hwnd, (POINT *)&spectrum.rect, 2);
        break;

        // Display, resize it
    case DISPLAY_ID:
        MoveWindow(hWnd, MARGIN, spectrum.rect.bottom + SPACING,
                   width - MARGIN * 2,
                   (height - TOTAL) * DISPLAY_HEIGHT / TOTAL_HEIGHT,
                   false);
        InvalidateRgn(hWnd, NULL, true);
        GetWindowRect(hWnd, &display.rect);
        MapWindowPoints(NULL, window.hwnd, (POINT *)&display.rect, 2);
        break;

        // Strobe, resize it
    case STROBE_ID:
        MoveWindow(hWnd, MARGIN, display.rect.bottom + SPACING,
                   width - MARGIN * 2,
                   (height - TOTAL) * STROBE_HEIGHT / TOTAL_HEIGHT,
                   false);
        InvalidateRgn(hWnd, NULL, true);
        GetWindowRect(hWnd, &strobe.rect);
        MapWindowPoints(NULL, window.hwnd, (POINT *)&strobe.rect, 2);
        break;

        // Staff, resize it
    case STAFF_ID:
        MoveWindow(hWnd, MARGIN, display.rect.bottom + SPACING,
                   width - MARGIN * 2,
                   (height - TOTAL) * STAFF_HEIGHT / TOTAL_HEIGHT,
                   false);
        InvalidateRgn(hWnd, NULL, true);
        GetWindowRect(hWnd, &staff.rect);
        MapWindowPoints(NULL, window.hwnd, (POINT *)&staff.rect, 2);
        break;

        // Meter, resize it
    case METER_ID:
        MoveWindow(hWnd, MARGIN, strobe.rect.bottom + SPACING,
                   width - MARGIN * 2,
                   (height - TOTAL) * METER_HEIGHT / TOTAL_HEIGHT,
                   false);
        InvalidateRgn(hWnd, NULL, true);
        GetWindowRect(hWnd, &meter.rect);
        MapWindowPoints(NULL, window.hwnd, (POINT *)&meter.rect, 2);
        break;
    }

    return true;
}

// Window resizing
BOOL WindowResizing(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    PRECT rectp = (PRECT)lParam;

    // Get the window and client dimensions
    GetWindowRect(hWnd, &window.wind);
    GetClientRect(hWnd, &window.rect);

    // Edges
    int border = (window.wind.right - window.wind.left) -
        window.rect.right;
    int header = (window.wind.bottom - window.wind.top) -
        window.rect.bottom;

    // Window minimum width and height
    int width  = WIDTH + border;
    int height = HEIGHT + toolbar.rect.bottom + header;

    // Minimum size
    if (rectp->right - rectp->left < width)
        rectp->right = rectp->left + width;

    if (rectp->bottom - rectp->top < height)
        rectp->bottom = rectp->top + height;

    // Maximum width
    if (rectp->right - rectp->left > STEP + border)
        rectp->right = rectp->left + STEP + border;

    // Offered width and height
    width = rectp->right - rectp->left;
    height = rectp->bottom - rectp->top;

    switch (wParam)
    {
    case WMSZ_LEFT:
    case WMSZ_RIGHT:
        height = (((width - border) * HEIGHT) / WIDTH) +
            toolbar.rect.bottom + header;
        rectp->bottom = rectp->top + height;
        break;

    case WMSZ_TOP:
    case WMSZ_BOTTOM:
        width = ((((height - toolbar.rect.bottom) - header) *
                  WIDTH) / HEIGHT) + border;
        rectp->right = rectp->left + width;
        break;

    default:
        width = ((((height - toolbar.rect.bottom) - header) *
                  WIDTH) / HEIGHT) + border;
        rectp->right = rectp->left + width;
        break;
    }

    return true;
}

// Add toolbar bitmap
BOOL AddToolbarBitmap(HWND control, LPCTSTR name)
{
    // Load bitmap
    HBITMAP hbm = (HBITMAP)
        LoadImage(hInst, name, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

    // Create DC
    HDC hdc = CreateCompatibleDC(NULL);

    // Select the bitmap
    SelectObject(hdc, hbm);

    // Select a brush
    SelectObject(hdc, GetSysColorBrush(COLOR_BTNFACE));

    // Get the colour of the first pixel
    COLORREF colour = GetPixel(hdc, 0, 0);

    // Flood fill the bitmap
    ExtFloodFill(hdc, 0, 0, colour, FLOODFILLSURFACE);

    // And the centre of the icon
    if (GetPixel(hdc, 15, 15) == colour)
            ExtFloodFill(hdc, 15, 15, colour, FLOODFILLSURFACE);

    // Delete the DC
    DeleteObject(hdc);

    // Add bitmap
    TBADDBITMAP bitmap =
        {NULL, (UINT_PTR)hbm};

    SendMessage(control, TB_ADDBITMAP, 1, (LPARAM)&bitmap);

    return true;
}

BOOL AddToolbarButtons(HWND control)
{
    // Define the buttons
    TBBUTTON buttons[] =
        {{0, 0, 0, BTNS_SEP},
         {OPTIONS_BM, OPTIONS_ID, TBSTATE_ENABLED, BTNS_BUTTON,
          {0}, 0, (INT_PTR)"Options"},
         {0, 0, 0, BTNS_SEP}};

    // Add to toolbar
    SendMessage(control, TB_ADDBUTTONS,
                Length(buttons), (LPARAM)&buttons);

    return true;
}

// Display context menu
BOOL DisplayContextMenu(HWND hWnd, POINTS points)
{
    HMENU menu;
    POINT point;

    // Convert coordinates
    POINTSTOPOINT(point, points);
    ClientToScreen(hWnd, &point);

    // Create menu
    menu = CreatePopupMenu();

    // Add menu items
    AppendMenu(menu, spectrum.zoom? MF_STRING | MF_CHECKED:
               MF_STRING, ZOOM_ID, "Zoom spectrum");
    AppendMenu(menu, strobe.enable? MF_STRING | MF_CHECKED:
               MF_STRING, ENABLE_ID, "Display strobe");
    AppendMenu(menu, audio.filter? MF_STRING | MF_CHECKED:
               MF_STRING, FILTER_ID, "Audio filter");
    AppendMenu(menu, audio.down? MF_STRING | MF_CHECKED:
               MF_STRING, DOWN_ID, "Downsample");
    AppendMenu(menu, display.lock? MF_STRING | MF_CHECKED:
               MF_STRING, LOCK_ID, "Lock display");
    AppendMenu(menu, display.mult? MF_STRING | MF_CHECKED:
               MF_STRING, MULT_ID, "Multiple notes");
    AppendMenu(menu, MF_SEPARATOR, 0, 0);
    AppendMenu(menu, MF_STRING, OPTIONS_ID, "Options...");
    AppendMenu(menu, MF_SEPARATOR, 0, 0);
    AppendMenu(menu, MF_STRING, QUIT_ID, "Quit");

    // Pop up the menu
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                   point.x, point.y,
                   0, hWnd, NULL);
    return true;
}

// Display options
BOOL DisplayOptions(WPARAM wParam, LPARAM lParam)
{
    WNDCLASS wc = 
        {CS_HREDRAW | CS_VREDRAW, OptionWProc,
         0, 0, hInst,
         LoadIcon(hInst, "Tuner"),
         LoadCursor(NULL, IDC_ARROW),
         GetSysColorBrush(COLOR_WINDOW),
         NULL, PCLASS};

    // Register the window class.
    RegisterClass(&wc);

    // Get the main window rect
    GetWindowRect(window.hwnd, &window.wind);

    // Create the window, offset right
    options.hwnd =
        CreateWindow(PCLASS, "Tuner Options",
                     WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION,
                     window.wind.left + OFFSET,
                     window.wind.top + OFFSET,
                     OPTIONS_WIDTH, OPTIONS_HEIGHT,
                     window.hwnd, (HMENU)NULL, hInst, NULL);
    return true;
}

// Options Procedure
LRESULT CALLBACK OptionWProc(HWND hWnd,
                             UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam)
{
    // Switch on message
    switch (uMsg)
    {
    case WM_CREATE:
        {
            static TCHAR s[64];

            // Get the window and client dimensions
            GetWindowRect(hWnd, &options.wind);
            GetClientRect(hWnd, &options.rect);

            // Calculate desired window width and height
            int border = (options.wind.right - options.wind.left) -
                options.rect.right;
            int header = (options.wind.bottom - options.wind.top) -
                options.rect.bottom;
            int width  = OPTIONS_WIDTH + border;
            int height = OPTIONS_HEIGHT + header;

            // Set new dimensions
            SetWindowPos(hWnd, NULL, 0, 0,
                         width, height,
                         SWP_NOMOVE | SWP_NOZORDER);

            // Get client dimensions
            GetWindowRect(hWnd, &options.wind);
            GetClientRect(hWnd, &options.rect);

            width = options.rect.right;
            height = options.rect.bottom;

            // Create group box
            group.hwnd =
                CreateWindow(WC_BUTTON, NULL,
                             WS_VISIBLE | WS_CHILD |
                             BS_GROUPBOX,
                             MARGIN, MARGIN, width - MARGIN * 2, GROUP_HEIGHT,
                             hWnd, NULL, hInst, NULL);
            GetWindowRect(group.hwnd, &group.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&group.rect, 2);

            // Create zoom tickbox
            zoom.hwnd =
                CreateWindow(WC_BUTTON, "Zoom spectrum:",
                             WS_VISIBLE | WS_CHILD | BS_LEFTTEXT |
                             BS_CHECKBOX,
                             group.rect.left + MARGIN, group.rect.top + MARGIN,
                             CHECK_WIDTH, CHECK_HEIGHT,
                             hWnd, (HMENU)ZOOM_ID, hInst, NULL);
            GetWindowRect(zoom.hwnd, &zoom.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&zoom.rect, 2);

            Button_SetCheck(zoom.hwnd,
                            spectrum.zoom? BST_CHECKED: BST_UNCHECKED);

            // Add tickbox to tooltip
            tooltip.info.uId = (UINT_PTR)zoom.hwnd;
            tooltip.info.lpszText = (LPSTR)"Zoom spectrum";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create strobe enable tickbox
            enable.hwnd =
                CreateWindow(WC_BUTTON, "Display strobe:",
                             WS_VISIBLE | WS_CHILD | BS_LEFTTEXT |
                             BS_CHECKBOX,
                             width / 2 + MARGIN, group.rect.top + MARGIN,
                             CHECK_WIDTH, CHECK_HEIGHT,
                             hWnd, (HMENU)ENABLE_ID, hInst, NULL);
            GetWindowRect(enable.hwnd, &enable.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&enable.rect, 2);

            Button_SetCheck(enable.hwnd,
                            strobe.enable? BST_CHECKED: BST_UNCHECKED);

            // Add tickbox to tooltip
            tooltip.info.uId = (UINT_PTR)enable.hwnd;
            tooltip.info.lpszText = (LPSTR)"Display strobe";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create filter tickbox
            filt.hwnd =
                CreateWindow(WC_BUTTON, "Audio filter:",
                             WS_VISIBLE | WS_CHILD | BS_LEFTTEXT |
                             BS_CHECKBOX,
                             group.rect.left + MARGIN,
                             zoom.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT,
                             hWnd, (HMENU)FILTER_ID, hInst, NULL);
            GetWindowRect(filt.hwnd, &filt.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&filt.rect, 2);

            Button_SetCheck(filt.hwnd,
                            audio.filter? BST_CHECKED: BST_UNCHECKED);

            // Add tickbox to tooltip
            tooltip.info.uId = (UINT_PTR)filt.hwnd;
            tooltip.info.lpszText = (LPSTR)"Audio filter";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create downsample tickbox
            down.hwnd =
                CreateWindow(WC_BUTTON, "Downsample:",
                             WS_VISIBLE | WS_CHILD | BS_LEFTTEXT |
                             BS_CHECKBOX,
                             width / 2 + MARGIN, enable.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT,
                             hWnd, (HMENU)DOWN_ID, hInst, NULL);
            GetWindowRect(down.hwnd, &down.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&down.rect, 2);

            Button_SetCheck(down.hwnd,
                            audio.down? BST_CHECKED: BST_UNCHECKED);

            // Add tickbox to tooltip
            tooltip.info.uId = (UINT_PTR)lock.hwnd;
            tooltip.info.lpszText = (LPSTR)"Downsample";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create multiple tickbox
            mult.hwnd =
                CreateWindow(WC_BUTTON, "Multiple notes:",
                             WS_VISIBLE | WS_CHILD | BS_LEFTTEXT |
                             BS_CHECKBOX,
                             group.rect.left + MARGIN,
                             filt.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT,
                             hWnd, (HMENU)MULT_ID, hInst, NULL);
            GetWindowRect(mult.hwnd, &mult.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&mult.rect, 2);

            Button_SetCheck(mult.hwnd,
                            display.mult? BST_CHECKED: BST_UNCHECKED);

            // Add tickbox to tooltip
            tooltip.info.uId = (UINT_PTR)mult.hwnd;
            tooltip.info.lpszText = (LPSTR)"Display multiple notes";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create lock tickbox
            lock.hwnd =
                CreateWindow(WC_BUTTON, "Lock display:",
                             WS_VISIBLE | WS_CHILD | BS_LEFTTEXT |
                             BS_CHECKBOX,
                             width / 2 + MARGIN,
                             down.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT,
                             hWnd, (HMENU)LOCK_ID, hInst, NULL);
            GetWindowRect(lock.hwnd, &lock.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&lock.rect, 2);

            Button_SetCheck(lock.hwnd,
                            display.lock? BST_CHECKED: BST_UNCHECKED);

            // Add tickbox to tooltip
            tooltip.info.uId = (UINT_PTR)lock.hwnd;
            tooltip.info.lpszText = (LPSTR)"Lock display";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create fundamental tickbox
            fund.hwnd =
                CreateWindow(WC_BUTTON, "Fundamental:",
                             WS_VISIBLE | WS_CHILD | BS_LEFTTEXT |
                             BS_CHECKBOX,
                             group.rect.left + MARGIN,
                             mult.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT,
                             hWnd, (HMENU)FUND_ID, hInst, NULL);
            GetWindowRect(fund.hwnd, &fund.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&fund.rect, 2);

            Button_SetCheck(fund.hwnd,
                            audio.fund? BST_CHECKED: BST_UNCHECKED);

            // Add tickbox to tooltip
            tooltip.info.uId = (UINT_PTR)fund.hwnd;
            tooltip.info.lpszText = (LPSTR)"Fundamental filter";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create note filter tickbox
            note.hwnd =
                CreateWindow(WC_BUTTON, "Note filter:",
                             WS_VISIBLE | WS_CHILD | BS_LEFTTEXT |
                             BS_CHECKBOX,
                             width / 2 + MARGIN,
                             lock.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT,
                             hWnd, (HMENU)NOTE_ID, hInst, NULL);
            GetWindowRect(note.hwnd, &note.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&note.rect, 2);

            Button_SetCheck(note.hwnd,
                            audio.note? BST_CHECKED: BST_UNCHECKED);

            // Add tickbox to tooltip
            tooltip.info.uId = (UINT_PTR)note.hwnd;
            tooltip.info.lpszText = (LPSTR)"Note filter";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create group box
            group.hwnd =
                CreateWindow(WC_BUTTON, NULL,
                             WS_VISIBLE | WS_CHILD |
                             BS_GROUPBOX,
                             MARGIN, group.rect.bottom + SPACING,
                             width - MARGIN * 2, EXPAND_HEIGHT,
                             hWnd, NULL, hInst, NULL);
            GetWindowRect(group.hwnd, &group.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&group.rect, 2);

            // Create text
            text.hwnd =
                CreateWindow(WC_STATIC, "Spectrum expand:",
                             WS_VISIBLE | WS_CHILD | SS_LEFT,
                             group.rect.left + MARGIN,
                             group.rect.top + MARGIN,
                             CHECK_WIDTH, CHECK_HEIGHT, hWnd,
                             (HMENU)TEXT_ID, hInst, NULL);
            GetWindowRect(text.hwnd, &text.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&text.rect, 2);

            // Create combo box
            expand.hwnd =
                CreateWindow(WC_COMBOBOX, "",
                             WS_VISIBLE | WS_CHILD |
                             CBS_DROPDOWNLIST,
                             width / 2 + MARGIN, text.rect.top,
                             CHECK_WIDTH, CHECK_HEIGHT, hWnd,
                             (HMENU)EXPAND_ID, hInst, NULL);
            GetWindowRect(expand.hwnd, &expand.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&expand.rect, 2);

            const char *sizes[] =
                {" x 1", " x 2", " x 4", " x 8", " x 16"};
            for (unsigned int i = 0; i < Length(sizes); i++)
                ComboBox_AddString(expand.hwnd, sizes[i]);

            // Select x 1
            ComboBox_SelectString(expand.hwnd, -1, " x 1");

            // Add edit to tooltip
            tooltip.info.uId = (UINT_PTR)expand.hwnd;
            tooltip.info.lpszText = (LPSTR)"Spectrum expand";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create text
            text.hwnd =
                CreateWindow(WC_STATIC, "Strobe colours:",
                             WS_VISIBLE | WS_CHILD | SS_LEFT,
                             group.rect.left + MARGIN,
                             text.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT, hWnd,
                             (HMENU)TEXT_ID, hInst, NULL);
            GetWindowRect(text.hwnd, &text.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&text.rect, 2);

            // Create combo box
            colours.hwnd =
                CreateWindow(WC_COMBOBOX, "",
                             WS_VISIBLE | WS_CHILD |
                             CBS_DROPDOWNLIST,
                             width / 2, text.rect.top,
                             CHECK_WIDTH + MARGIN, CHECK_HEIGHT, hWnd,
                             (HMENU)COLOURS_ID, hInst, NULL);
            GetWindowRect(colours.hwnd, &colours.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&colours.rect, 2);

            const char *strings[] =
                {" Blue/Cyan", " Olive/Aqua", " Magenta/Yellow"};
            for (unsigned int i = 0; i < Length(strings); i++)
                ComboBox_AddString(colours.hwnd, strings[i]);

            // Select Olive/Aqua
            ComboBox_SetCurSel(colours.hwnd, strobe.colours);

            // Add edit to tooltip
            tooltip.info.uId = (UINT_PTR)colours.hwnd;
            tooltip.info.lpszText = (LPSTR)"Strobe colours";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create text
            text.hwnd =
                CreateWindow(WC_STATIC, "Reference:",
                             WS_VISIBLE | WS_CHILD | SS_LEFT,
                             group.rect.left + MARGIN,
                             text.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT, hWnd,
                             (HMENU)TEXT_ID, hInst, NULL);
            GetWindowRect(text.hwnd, &text.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&text.rect, 2);

            // Create edit control
            sprintf(s, " %6.2lf", audio.reference);

            reference.hwnd =
                CreateWindow(WC_EDIT, s,
                             WS_VISIBLE | WS_CHILD |
                             WS_BORDER,
                             width / 2 + MARGIN, text.rect.top,
                             CHECK_WIDTH, CHECK_HEIGHT, hWnd,
                             (HMENU)REFERENCE_ID, hInst, NULL);
            GetWindowRect(reference.hwnd, &reference.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&reference.rect, 2);

            // Add edit to tooltip
            tooltip.info.uId = (UINT_PTR)reference.hwnd;
            tooltip.info.lpszText = (LPSTR)"Reference";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create up-down control
            updown.hwnd =
                CreateWindow(UPDOWN_CLASS, NULL,
                             WS_VISIBLE | WS_CHILD |
                             UDS_AUTOBUDDY | UDS_ALIGNRIGHT,
                             0, 0, 0, 0, hWnd,
                             (HMENU)UPDOWN_ID, hInst, NULL);

            SendMessage(updown.hwnd, UDM_SETRANGE32, MIN_REF, MAX_REF);
            SendMessage(updown.hwnd, UDM_SETPOS32, 0, audio.reference * 10);

            // Add updown to tooltip
            tooltip.info.uId = (UINT_PTR)updown.hwnd;
            tooltip.info.lpszText = (LPSTR)"Reference";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create text
            text.hwnd =
                CreateWindow(WC_STATIC, "Transpose:",
                             WS_VISIBLE | WS_CHILD | SS_LEFT,
                             group.rect.left + MARGIN,
                             text.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT, hWnd,
                             (HMENU)TEXT_ID, hInst, NULL);
            GetWindowRect(text.hwnd, &text.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&text.rect, 2);

            // Create combo box
            transpose.hwnd =
                CreateWindow(WC_COMBOBOX, "",
                             WS_VISIBLE | WS_CHILD |
                             CBS_DROPDOWNLIST,
                             width / 2 + MARGIN, text.rect.top,
                             CHECK_WIDTH, CHECK_HEIGHT, hWnd,
                             (HMENU)TRANSPOSE_ID, hInst, NULL);
            GetWindowRect(transpose.hwnd, &transpose.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&transpose.rect, 2);

            const char *trans[] =
                {" +6[Key:F#]", " +5[Key:F]", " +4[Key:E]",
                 " +3[Key:Eb]", " +2[Key:D]",
                 " +1[Key:C#]", " +0[Key:C]", " -1[Key:B]",
                 " -2[Key:Bb]", " -3[Key:A]",
                 " -4[Key:Ab]", " -5[Key:G]",
                 " -6[Key:F#]"};
            for (unsigned int i = 0; i < Length(trans); i++)
                ComboBox_AddString(transpose.hwnd, trans[i]);

            // Select +0[Key:C]
            ComboBox_SetCurSel(transpose.hwnd, 6 - display.transpose);

            // Add to tooltip
            tooltip.info.uId = (UINT_PTR)transpose.hwnd;
            tooltip.info.lpszText = (LPSTR)"Transpose display";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create text
            text.hwnd =
                CreateWindow(WC_STATIC, "Temperament:",
                             WS_VISIBLE | WS_CHILD | SS_LEFT,
                             group.rect.left + MARGIN,
                             text.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT, hWnd,
                             (HMENU)TEXT_ID, hInst, NULL);
            GetWindowRect(text.hwnd, &text.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&text.rect, 2);

            // Create combo box
            temperament.hwnd =
                CreateWindow(WC_COMBOBOX, "",
                             WS_VISIBLE | WS_CHILD | WS_VSCROLL |
                             CBS_DROPDOWNLIST,
                             width / 2 - MARGIN, text.rect.top,
                             CHECK_WIDTH + MARGIN * 2, CHECK_HEIGHT, hWnd,
                             (HMENU)TEMPERAMENT_ID, hInst, NULL);
            GetWindowRect(temperament.hwnd, &temperament.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&temperament.rect, 2);

            const char *temp[] =
                {" Kirnberger II", " Kirnberger III",
                 " Werckmeister III", " Werckmeister IV",
                 " Werckmeister V", " Werckmeister VI",
                 " Bach (Klais)", " Just (Barbour)",
                 " Equal Temperament", " Pythagorean",
                 " Van Zwolle", " Meantone (-1/4)",
                 " Silbermann (-1/6)", " Salinas (-1/3)",
                 " Zarlino (-2/7)", " Rossi (-1/5)",
                 " Rossi (-2/9)", " Rameau (-1/4)",
                 " Kellner", " Vallotti",
                 " Young II", " Bendeler III",
                 " Neidhardt I", " Neidhardt II",
                 " Neidhardt III", " Bruder 1829",
                 " Barnes 1977", " Lambert 1774",
                 " Schlick (H. Vogel)", " Meantone # (-1/4)",
                 " Meantone b (-1/4)", " Lehman-Bach"};
            for (unsigned int i = 0; i < Length(temp); i++)
                ComboBox_AddString(temperament.hwnd, temp[i]);

            // Select Equal Temperament
            ComboBox_SetCurSel(temperament.hwnd, audio.temperament);

            // Add edit to tooltip
            tooltip.info.uId = (UINT_PTR)temperament.hwnd;
            tooltip.info.lpszText = (LPSTR)"Temperament";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create text
            text.hwnd =
                CreateWindow(WC_STATIC, "Key:",
                             WS_VISIBLE | WS_CHILD | SS_LEFT,
                             group.rect.left + MARGIN,
                             text.rect.bottom + SPACING,
                             CHECK_WIDTH, CHECK_HEIGHT, hWnd,
                             (HMENU)TEXT_ID, hInst, NULL);
            GetWindowRect(text.hwnd, &text.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&text.rect, 2);

            // Create combo box
            key.hwnd =
                CreateWindow(WC_COMBOBOX, "",
                             WS_VISIBLE | WS_CHILD |
                             CBS_DROPDOWNLIST,
                             width / 4, text.rect.top,
                             MARGIN * 2, CHECK_HEIGHT, hWnd,
                             (HMENU)KEY_ID, hInst, NULL);
            GetWindowRect(key.hwnd, &key.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&key.rect, 2);

            const char *keys[] =
                {" C", " C#", " D", " Eb",
                 " E", " F", " F#", " G",
                 " Ab", " A", " Bb", " B"};
            for (unsigned int i = 0; i < Length(keys); i++)
                ComboBox_AddString(key.hwnd, keys[i]);

            // Select C
            ComboBox_SetCurSel(key.hwnd, audio.key);

            // Add edit to tooltip
            tooltip.info.uId = (UINT_PTR)key.hwnd;
            tooltip.info.lpszText = (LPSTR)"Key";
            SendMessage(tooltip.hwnd, TTM_ADDTOOL, 0,
                        (LPARAM) &tooltip.info);

            // Create filter button
            button.filter.hwnd =
                CreateWindow(WC_BUTTON, "Filters...",
                             WS_VISIBLE | WS_CHILD |
                             BS_PUSHBUTTON,
                             width / 2 + MARGIN, text.rect.top,
                             CHECK_WIDTH, BUTTON_HEIGHT,
                             hWnd, (HMENU)FILTERS_ID, hInst, NULL);

            // Enable button
            Button_Enable(button.filter.hwnd, audio.note);

            // Create close button
            // button.close.hwnd =
            //     CreateWindow(WC_BUTTON, "Close",
            //                  WS_VISIBLE | WS_CHILD |
            //                  BS_PUSHBUTTON,
            //                  width / 2 + MARGIN,
            //                  text.rect.bottom + SPACING,
            //                  CHECK_WIDTH, BUTTON_HEIGHT,
            //                  hWnd, (HMENU)CLOSE_ID, hInst, NULL);
        }
        break;

        // Colour static text
    case WM_CTLCOLORSTATIC:
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        break;

        // Draw item
    case WM_DRAWITEM:
        return DrawItem(wParam, lParam);
        break;

        // Updown control
    case WM_VSCROLL:
        switch (GetWindowLongPtr((HWND)lParam, GWLP_ID))
        {
        case UPDOWN_ID:
            ChangeReference(wParam, lParam);
            break;
        }

        // Set the focus back to the window
        SetFocus(hWnd);
        break;

        // Set the focus back to the window by clicking
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
        SetFocus(hWnd);
        break;

        // Display options menu
    case WM_RBUTTONDOWN:
        DisplayOptionsMenu(hWnd, MAKEPOINTS(lParam));
        break;

        // Commands
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            // Zoom control
        case ZOOM_ID:
            ZoomClicked(wParam, lParam);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;

            // Enable control
        case ENABLE_ID:
            EnableClicked(wParam, lParam);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;

            // Filter control
        case FILTER_ID:
            FilterClicked(wParam, lParam);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;

            // Downsample
        case DOWN_ID:
            DownClicked(wParam, lParam);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;

            // Lock control
        case LOCK_ID:
            LockClicked(wParam, lParam);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;

            // Multiple control
        case MULT_ID:
            MultipleClicked(wParam, lParam);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;

            // Fundamental control
        case FUND_ID:
            FundamentalClicked(wParam, lParam);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;

            // Note filter control
        case NOTE_ID:
            NoteFilterClicked(wParam, lParam);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;

            // Expand
        case EXPAND_ID:
            ExpandClicked(wParam, lParam);
            break;

            // Colours
        case COLOURS_ID:
            ColoursClicked(wParam, lParam);
            break;

            // Reference
        case REFERENCE_ID:
            EditReference(wParam, lParam);
            break;

            // Transpose
        case TRANSPOSE_ID:
            TransposeClicked(wParam, lParam);
            break;

            // Temperament
        case TEMPERAMENT_ID:
            TemperamentClicked(wParam, lParam);
            break;

            // Key
        case KEY_ID:
            KeyClicked(wParam, lParam);
            break;

            // Filters
        case FILTERS_ID:
            DisplayFilters(wParam, lParam);
            break;

            // Close
        case CLOSE_ID:
            SendMessage(hWnd, WM_DESTROY, 0, 0);
            ShowWindow(hWnd, false);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;
        }
        break;

        // Char pressed
    case WM_CHAR:
        CharPressed(wParam, lParam);
        break;

    case WM_DESTROY:
        options.hwnd = NULL;
        break;

        // Everything else
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

// Zoom clicked
BOOL ZoomClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        SpectrumClicked(wParam, lParam);
    }
    return true;
}

// Enable clicked

BOOL EnableClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        StrobeClicked(wParam, lParam);
    }
    return true;
}

// Filter clicked
BOOL FilterClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        audio.filter = !audio.filter;
        break;

    default:
        return false;
    }

    if (filt.hwnd != NULL)
        Button_SetCheck(filt.hwnd, audio.filter? BST_CHECKED: BST_UNCHECKED);

    HKEY hkey;
    LONG error;

    error = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\CTuner", 0,
                           NULL, 0, KEY_WRITE, NULL, &hkey, NULL);

    if (error == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey, "Filter", 0, REG_DWORD,
                      (LPBYTE)&audio.filter, sizeof(audio.filter));

        RegCloseKey(hkey);
    }

    else
    {
        static TCHAR s[64];

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error,
                      0, s, sizeof(s), NULL);

        MessageBox(window.hwnd, s, "RegCreateKeyEx", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

// Colours clicked
BOOL ColoursClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        strobe.colours++;

        if (strobe.colours > MAGENTA)
            strobe.colours = BLUE;

        if (colours.hwnd != NULL)
            ComboBox_SetCurSel(colours.hwnd, strobe.colours);

        strobe.changed = true;
        break;

    case CBN_SELENDOK:
        strobe.colours = ComboBox_GetCurSel(colours.hwnd);
        strobe.changed = true;
        break;

    default:
        return false;
    }

    HKEY hkey;
    LONG error;

    error = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\CTuner", 0,
                           NULL, 0, KEY_WRITE, NULL, &hkey, NULL);

    if (error == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey, "Colours", 0, REG_DWORD,
                      (LPBYTE)&strobe.colours, sizeof(strobe.colours));

        RegCloseKey(hkey);
    }

    else
    {
        static TCHAR s[64];

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error,
                      0, s, sizeof(s), NULL);

        MessageBox(window.hwnd, s, "RegCreateKeyEx", MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

// Down clicked
BOOL DownClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        audio.down = !audio.down;
        break;

    default:
        return false;
    }

    if (down.hwnd != NULL)
        Button_SetCheck(down.hwnd, audio.down? BST_CHECKED: BST_UNCHECKED);

    return true;
}

// Fundamental clicked
BOOL FundamentalClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        audio.fund = !audio.fund;
        break;

    default:
        return false;
    }

    if (fund.hwnd != NULL)
        Button_SetCheck(fund.hwnd, audio.fund? BST_CHECKED: BST_UNCHECKED);

    return true;
}

// Note filter clicked
BOOL NoteFilterClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        audio.note = !audio.note;
        break;

    default:
        return false;
    }

    if (note.hwnd != NULL)
        Button_SetCheck(note.hwnd, audio.note? BST_CHECKED: BST_UNCHECKED);

    if (button.filter.hwnd != NULL)
        Button_Enable(button.filter.hwnd, audio.note);

    return true;
}

// Expand clicked
BOOL ExpandClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        if (spectrum.expand < 16)
            spectrum.expand *= 2;

        if (expand.hwnd != NULL)
            ComboBox_SetCurSel(expand.hwnd, round(log2(spectrum.expand)));
        break;

    case CBN_SELENDOK:
        spectrum.expand = round(pow(2, ComboBox_GetCurSel(expand.hwnd)));
        break;

    default:
        return false;
    }

    return true;
}

// Contract clicked
BOOL ContractClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        if (spectrum.expand > 1)
            spectrum.expand /= 2;

        if (expand.hwnd != NULL)
            ComboBox_SetCurSel(expand.hwnd, round(log2(spectrum.expand)));
        break;

    default:
        return false;
    }

    return true;
}

// Transpose clicked
BOOL TransposeClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case CBN_SELENDOK:
        display.transpose = 6 - ComboBox_GetCurSel(transpose.hwnd);
        staff.transpose = display.transpose;
        break;

    default:
        return false;
    }

    return true;
}

// Temperament clicked
BOOL TemperamentClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case CBN_SELENDOK:
        audio.temperament = ComboBox_GetCurSel(temperament.hwnd);
        break;

    default:
        return false;
    }

    return true;
}

// Key clicked
BOOL KeyClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case CBN_SELENDOK:
        audio.key = ComboBox_GetCurSel(key.hwnd);
        break;

    default:
        return false;
    }

    return true;
}

// Lock clicked
BOOL LockClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        display.lock = !display.lock;
        break;

    default:
        return false;
    }

    InvalidateRgn(display.hwnd, NULL, true);

    if (lock.hwnd != NULL)
        Button_SetCheck(lock.hwnd, display.lock? BST_CHECKED: BST_UNCHECKED);
    return true;
}

// Multiple clicked
BOOL MultipleClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        display.mult = !display.mult;
        break;

    default:
        return false;
    }

    if (mult.hwnd != NULL)
        Button_SetCheck(mult.hwnd, display.mult? BST_CHECKED: BST_UNCHECKED);

    InvalidateRgn(display.hwnd, NULL, true);

    return true;
}

// Display options menu
BOOL DisplayOptionsMenu(HWND hWnd, POINTS points)
{
    HMENU menu;
    POINT point;

    POINTSTOPOINT(point, points);
    ClientToScreen(hWnd, &point);

    menu = CreatePopupMenu();
    AppendMenu(menu, MF_STRING, CLOSE_ID, "Close");

    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                   point.x, point.y,
                   0, hWnd, NULL);
    return true;
}

// Char pressed
BOOL CharPressed(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        // Copy display
    case 'C':
    case 'c':
    case 0x3:
        CopyDisplay(wParam, lParam);
        break;

        // Downsample

    case 'D':
    case 'd':
        DownClicked(wParam, lParam);
        break;

        // Filter
    case 'F':
    case 'f':
        FilterClicked(wParam, lParam);
        break;

        // Colour
    case 'K':
    case 'k':
        ColoursClicked(wParam, lParam);
        break;

        // Lock
    case 'L':
    case 'l':
        LockClicked(wParam, lParam);
        break;

        // Options
    case 'O':
    case 'o':
        DisplayOptions(wParam, lParam);
        break;

        // Strobe
    case 'S':
    case 's':
        EnableClicked(wParam, lParam);
        break;

        // Multiple
    case 'M':
    case 'm':
        MultipleClicked(wParam, lParam);
        break;

        // Zoom
    case 'Z':
    case 'z':
        ZoomClicked(wParam, lParam);
        break;

        // Expand
    case '+':
        ExpandClicked(wParam, lParam);
        break;

        // Contract
    case '-':
        ContractClicked(wParam, lParam);
        break;
    }

    return true;
}

// Copy display
BOOL CopyDisplay(WPARAM wParam, LPARAM lParam)
{
    // Memory size
    enum
    {MEM_SIZE = 1024};

    static TCHAR s[64];

    static const TCHAR *notes[] =
        {"C", "C#", "D", "Eb", "E", "F",
         "F#", "G", "Ab", "A", "Bb", "B"};

    // Open clipboard
    if (!OpenClipboard(window.hwnd))
        return false;

    // Empty clipboard
    EmptyClipboard(); 

    // Allocate memory
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, MEM_SIZE);

    if (mem == NULL)
    {
        CloseClipboard();
        return false;
    }

    // Lock the memory
    LPTSTR text = (LPTSTR)GlobalLock(mem);

    // Check if multiple
    if (display.mult && display.count > 0)
    {
        // For each set of values
        for (int i = 0; i < display.count; i++)
        {
            double f = display.maxima[i].f;

            // Reference freq
            double fr = display.maxima[i].fr;

            int n = display.maxima[i].n;

            if (n < 0)
                n = 0;

            double c = -12.0 * log2(fr / f);

            // Ignore silly values
            if (!isfinite(c))
                continue;

            // Print the text
            wsprintf(s, "%s%d\t%+6.2lf\t%9.2lf\t%9.2lf\t%+8.2lf\r\n",
                    notes[n % Length(notes)], n / 12,
                    c * 100.0, fr, f, f - fr);

            // Copy to the memory
            if (i == 0)
                lstrcpy(text, s);

            else
                lstrcat(text, s);
        }
    }

    else
    {
        // Print the values
        wsprintf(s, "%s%d\t%+6.2lf\t%9.2lf\t%9.2lf\t%+8.2lf\r\n",
                notes[display.n % Length(notes)], display.n / 12,
                display.c * 100.0, display.fr, display.f,
                display.f - display.fr);

        // Copy to the memory
        lstrcpy(text, s);
    }

    // Place in clipboard
    GlobalUnlock(text);
    SetClipboardData(CF_TEXT, mem);
    CloseClipboard(); 
 
    return true;
}

// Display filters
BOOL DisplayFilters(WPARAM wParam, LPARAM lParam)
{
    WNDCLASS wc = 
        {CS_HREDRAW | CS_VREDRAW, FilterWProc,
         0, 0, hInst,
         LoadIcon(hInst, "Tuner"),
         LoadCursor(NULL, IDC_ARROW),
         GetSysColorBrush(COLOR_WINDOW),
         NULL, FCLASS};

    // Register the window class.
    RegisterClass(&wc);

    // Get the options window rect
    GetWindowRect(options.hwnd, &options.rect);

    // Create the window, offset right
    filters.hwnd =
        CreateWindow(FCLASS, "Tuner Filters",
                     WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION,
                     options.rect.left + OFFSET,
                     options.rect.top + OFFSET,
                     FILTERS_WIDTH, FILTERS_HEIGHT,
                     window.hwnd, (HMENU)NULL, hInst, NULL);
    return true;
}

// Filters Procedure
LRESULT CALLBACK FilterWProc(HWND hWnd,
                             UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam)
{
    // Switch on message
    switch (uMsg)
    {
    case WM_CREATE:
        {
            // Get the window and client dimensions
            GetWindowRect(hWnd, &filters.wind);
            GetClientRect(hWnd, &filters.rect);

            // Calculate desired window width and height
            int border = (filters.wind.right - filters.wind.left) -
                filters.rect.right;
            int header = (filters.wind.bottom - filters.wind.top) -
                filters.rect.bottom;
            int width  = FILTERS_WIDTH + border;
            int height = FILTERS_HEIGHT + header;

            // Set new dimensions
            SetWindowPos(hWnd, NULL, 0, 0,
                         width, height,
                         SWP_NOMOVE | SWP_NOZORDER);

            // Get client dimensions
            GetWindowRect(hWnd, &filters.wind);
            GetClientRect(hWnd, &filters.rect);

            width = filters.rect.right;
            height = filters.rect.bottom;

            // Create group box
            group.hwnd =
                CreateWindow(WC_BUTTON, NULL,
                             WS_VISIBLE | WS_CHILD |
                             BS_GROUPBOX,
                             MARGIN, MARGIN,
                             width - MARGIN * 2, FILTER_HEIGHT,
                             hWnd, NULL, hInst, NULL);
            GetWindowRect(group.hwnd, &group.rect);
            MapWindowPoints(NULL, hWnd, (POINT *)&group.rect, 2);

            static const TCHAR *labels[] =
                {"C:", "C#:", "D:", "Eb:", "E:", "F:",
                 "F#:", "G:", "Ab:", "A:", "Bb:", "B:"};
            static const UINT_PTR noteIds[] =
                {NOTES_C, NOTES_Cs, NOTES_D, NOTES_Eb,
                 NOTES_E, NOTES_F, NOTES_Fs, NOTES_G,
                 NOTES_Ab, NOTES_A, NOTES_Bb, NOTES_B};
            static const UINT_PTR octaveIds[] =
                {OCTAVES_0, OCTAVES_1, OCTAVES_2,
                 OCTAVES_3, OCTAVES_4, OCTAVES_5,
                 OCTAVES_6, OCTAVES_7, OCTAVES_8};

            for (unsigned int i = 0; i < Length(labels); i++)
            {
                boxes.notes[i].hwnd =
                    CreateWindow(WC_BUTTON, labels[i],
                                 WS_VISIBLE | WS_CHILD | BS_LEFTTEXT |
                                 BS_CHECKBOX, (i < Length(labels) / 2)?
                                 group.rect.left + MARGIN:
                                 group.rect.left + (MARGIN * 2) + NOTE_WIDTH,
                                 group.rect.top + MARGIN +
                                 (NOTE_HEIGHT + SPACING) *
                                 (i % (Length(labels) / 2)),
                                 NOTE_WIDTH, NOTE_HEIGHT,
                                 hWnd, (HMENU)noteIds[i], hInst, NULL);
                GetWindowRect(boxes.notes[i].hwnd, &boxes.notes[i].rect);
                MapWindowPoints(NULL, hWnd, (POINT *)&boxes.notes[i].rect, 2);

                Button_SetCheck(boxes.notes[i].hwnd,
                                filter.note[i]? BST_CHECKED: BST_UNCHECKED);
            }

            for (unsigned int i = 0; i < Length(filter.octave); i++)
            {
                static TCHAR s[64];
                sprintf(s, "Octave %d:", i);

                boxes.octaves[i].hwnd =
                    CreateWindow(WC_BUTTON, s,
                                 WS_VISIBLE | WS_CHILD | BS_LEFTTEXT |
                                 BS_CHECKBOX,
                                 (i < (Length(filter.octave) / 2) + 1)?
                                 group.rect.left + MARGIN +
                                 ((NOTE_WIDTH + MARGIN) * 2):
                                 group.rect.left + MARGIN +
                                 ((NOTE_WIDTH + MARGIN) * 2) +
                                 (OCTAVE_WIDTH + MARGIN),
                                 group.rect.top + MARGIN +
                                 (OCTAVE_HEIGHT + SPACING) *
                                 (i % ((Length(filter.octave) / 2) + 1)),
                                 OCTAVE_WIDTH, OCTAVE_HEIGHT,
                                 hWnd, (HMENU)octaveIds[i], hInst, NULL);
                GetWindowRect(boxes.octaves[i].hwnd, &boxes.octaves[i].rect);
                MapWindowPoints(NULL, hWnd, (POINT *)&boxes.octaves[i].rect, 2);

                Button_SetCheck(boxes.octaves[i].hwnd,
                                filter.octave[i]? BST_CHECKED: BST_UNCHECKED);
            }

            // Create close button
            button.close.hwnd =
                CreateWindow(WC_BUTTON, "Close",
                             WS_VISIBLE | WS_CHILD |
                             BS_PUSHBUTTON,
                             width / 2 + MARGIN,
                             boxes.notes[11].rect.top,
                             CHECK_WIDTH, BUTTON_HEIGHT,
                             hWnd, (HMENU)CLOSE_ID, hInst, NULL);
        }
        break;

        // Colour static text
    case WM_CTLCOLORSTATIC:
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        break;

        // Draw item
    case WM_DRAWITEM:
        return DrawItem(wParam, lParam);
        break;

        // Set the focus back to the window by clicking
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
        SetFocus(hWnd);
        break;

        // Display options menu
    case WM_RBUTTONDOWN:
        DisplayOptionsMenu(hWnd, MAKEPOINTS(lParam));
        break;

        // Commands
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            // Close
        case CLOSE_ID:
            SendMessage(hWnd, WM_DESTROY, 0, 0);
            ShowWindow(hWnd, false);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;

        default:
            BoxClicked(wParam, lParam);

            // Set the focus back to the window
            SetFocus(hWnd);
            break;
        }
        break;

        // Char pressed
    case WM_CHAR:
        CharPressed(wParam, lParam);
        break;

    case WM_DESTROY:
        options.hwnd = NULL;
        break;

        // Everything else
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

BOOL BoxClicked(WPARAM wParam, LPARAM lParam)
{
    static const UINT_PTR noteIds[] =
        {NOTES_C, NOTES_Cs, NOTES_D, NOTES_Eb,
         NOTES_E, NOTES_F, NOTES_Fs, NOTES_G,
         NOTES_Ab, NOTES_A, NOTES_Bb, NOTES_B};
    static const UINT_PTR octaveIds[] =
        {OCTAVES_0, OCTAVES_1, OCTAVES_2,
         OCTAVES_3, OCTAVES_4, OCTAVES_5,
         OCTAVES_6, OCTAVES_7, OCTAVES_8};

    int id = LOWORD(wParam);

    // Check notes
    for (unsigned int i = 0; i < Length(filter.note); i++)
    {
        if (id == noteIds[i])
        {
            filter.note[i] = !filter.note[i];
            Button_SetCheck((HWND)lParam,
                            filter.note[i]? BST_CHECKED: BST_UNCHECKED);
            return true;
        }
    }

    // Check octaves
    for (unsigned int i = 0; i < Length(filter.octave); i++)
    {
        if (id == octaveIds[i])
        {
            filter.octave[i] = !filter.octave[i];
            Button_SetCheck((HWND)lParam,
                            filter.octave[i]? BST_CHECKED: BST_UNCHECKED);
            return true;
        }
    }

    return false;
}

// Meter callback
VOID CALLBACK MeterCallback(PVOID lpParam, BOOL TimerFired)
{
    METERP meter = (METERP)lpParam;

    // Update meter
    InvalidateRgn(meter->hwnd, NULL, true);
}

// Strobe callback
VOID CALLBACK StrobeCallback(PVOID lpParam, BOOL TimerFired)
{
    STROBEP strobe = (STROBEP)lpParam;

    // Update strobe
    if (strobe->enable)
        InvalidateRgn(strobe->hwnd, NULL, true);
}

// Drawing functions moved to drawing.cpp
// Display clicked
BOOL DisplayClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        display.lock = !display.lock;
        break;

    default:
        return false;
    }

    if (lock.hwnd != NULL)
        Button_SetCheck(lock.hwnd, display.lock? BST_CHECKED: BST_UNCHECKED);

    InvalidateRgn(display.hwnd, NULL, true);

    return true;
}

// Scope clicked
BOOL ScopeClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        FilterClicked(wParam, lParam);
        break;

    default:
        return false;
    }

    return true;
}

// Spectrum clicked
BOOL SpectrumClicked(WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        spectrum.zoom = !spectrum.zoom;
        break;

    default:
        return false;
    }

    if (zoom.hwnd != NULL)
        Button_SetCheck(zoom.hwnd, spectrum.zoom? BST_CHECKED: BST_UNCHECKED);

    HKEY hkey;
    LONG error;

    error = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\CTuner", 0,
                           NULL, 0, KEY_WRITE, NULL, &hkey, NULL);

    if (error == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey, "Zoom", 0, REG_DWORD,
                      (LPBYTE)&spectrum.zoom, sizeof(spectrum.zoom));

        RegCloseKey(hkey);
    }

    else
    {
        static TCHAR s[64];

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error,
                      0, s, sizeof(s), NULL);

        MessageBox(window.hwnd, s, "RegCreateKeyEx", MB_OK | MB_ICONERROR);
    }

    return true;
}

// Strobe clicked
BOOL StrobeClicked(WPARAM wParam, LPARAM lParam)
{

    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        strobe.enable = !strobe.enable;
        staff.enable = !strobe.enable;
        break;

    default:
        return false;
    }

    ShowWindow(strobe.hwnd, strobe.enable? SW_SHOW: SW_HIDE);
    ShowWindow(staff.hwnd, staff.enable? SW_SHOW: SW_HIDE);

    InvalidateRgn(strobe.hwnd, NULL, true);
    InvalidateRgn(staff.hwnd, NULL, true);

    if (enable.hwnd != NULL)
        Button_SetCheck(enable.hwnd, strobe.enable? BST_CHECKED: BST_UNCHECKED);

    HKEY hkey;
    LONG error;

    error = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\CTuner", 0,
                           NULL, 0, KEY_WRITE, NULL, &hkey, NULL);

    if (error == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey, "Strobe", 0, REG_DWORD,
                      (LPBYTE)&strobe.enable, sizeof(strobe.enable));

        RegCloseKey(hkey);
    }

    else
    {
        static TCHAR s[64];

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error,
                      0, s, sizeof(s), NULL);

        MessageBox(window.hwnd, s, "RegCreateKeyEx", MB_OK | MB_ICONERROR);
    }

    return true;
}

// Staff clicked
BOOL StaffClicked(WPARAM wParam, LPARAM lParam)
{

    switch (HIWORD(wParam))
    {
    case BN_CLICKED:
        staff.enable = !staff.enable;
        strobe.enable = !staff.enable;
        break;

    default:
        return false;
    }

    ShowWindow(staff.hwnd, staff.enable? SW_SHOW: SW_HIDE);
    ShowWindow(strobe.hwnd, strobe.enable? SW_SHOW: SW_HIDE);

    InvalidateRgn(staff.hwnd, NULL, true);
    InvalidateRgn(strobe.hwnd, NULL, true);

    if (enable.hwnd != NULL)
        Button_SetCheck(enable.hwnd, strobe.enable? BST_CHECKED: BST_UNCHECKED);

    HKEY hkey;
    LONG error;

    error = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\CTuner", 0,
                           NULL, 0, KEY_WRITE, NULL, &hkey, NULL);

    if (error == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey, "Strobe", 0, REG_DWORD,
                      (LPBYTE)&strobe.enable, sizeof(strobe.enable));

        RegCloseKey(hkey);
    }

    else
    {
        static TCHAR s[64];

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error,
                      0, s, sizeof(s), NULL);

        MessageBox(window.hwnd, s, "RegCreateKeyEx", MB_OK | MB_ICONERROR);
    }

    return true;
}

// Meter clicked
BOOL MeterClicked(WPARAM wParam, LPARAM lParam)
{
    return DisplayClicked(wParam, lParam);
}

// Edit reference
BOOL EditReference(WPARAM wParam, LPARAM lParam)
{
    static TCHAR s[64];

    if (audio.reference == 0)
        return false;

    switch (HIWORD(wParam))
    {
    case EN_KILLFOCUS:
        GetWindowText(reference.hwnd, s, sizeof(s));
        audio.reference = atof(s);

        SendMessage(reference.hwnd, UDM_SETPOS32, 0,
                    audio.reference * 10);
        break;

    default:
        return false;
    }

    InvalidateRgn(display.hwnd, NULL, true);

    HKEY hkey;
    LONG error;

    error = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\CTuner", 0,
                           NULL, 0, KEY_WRITE, NULL, &hkey, NULL);

    if (error == ERROR_SUCCESS)
    {
        int value = audio.reference * 10;

        RegSetValueEx(hkey, "Reference", 0, REG_DWORD,
                      (LPBYTE)&value, sizeof(value));

        RegCloseKey(hkey);
    }

    else
    {
        static TCHAR s[64];

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error,
                      0, s, sizeof(s), NULL);

        MessageBox(window.hwnd, s, "RegCreateKeyEx", MB_OK | MB_ICONERROR);
    }

    return true;
}

// Change reference
BOOL ChangeReference(WPARAM wParam, LPARAM lParam)
{
    static TCHAR s[64];

    long value = SendMessage(updown.hwnd, UDM_GETPOS32, 0, 0);
    audio.reference = (double)value / 10.0;

    sprintf(s, " %6.2lf", audio.reference);
    SetWindowText(reference.hwnd, s);

    InvalidateRgn(display.hwnd, NULL, true);

    HKEY hkey;
    LONG error;

    error = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\CTuner", 0,
                           NULL, 0, KEY_WRITE, NULL, &hkey, NULL);

    if (error == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey, "Reference", 0, REG_DWORD,
                      (LPBYTE)&value, sizeof(value));

        RegCloseKey(hkey);
    }

    else
    {
        static TCHAR s[64];

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error,
                      0, s, sizeof(s), NULL);

        MessageBox(window.hwnd, s, "RegCreateKeyEx", MB_OK | MB_ICONERROR);
    }

    return true;
}

// Audio functions moved to audio.cpp
