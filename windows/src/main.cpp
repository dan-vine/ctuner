////////////////////////////////////////////////////////////////////////////////
//
//  main.cpp - CTuner entry point with ImGui and DirectX11
//
//  Copyright (C) 2024
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <d3d11.h>
#include <tchar.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "app_state.h"
#include "audio/audio_capture.h"
#include "audio/pitch_detector.h"
#include "ui/main_window.h"

// DirectX data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Application state
static ctuner::AppState g_appState;
static ctuner::AudioCapture g_audioCapture;
static ctuner::PitchDetector g_pitchDetector;
static ctuner::MainWindow g_mainWindow;

// Audio callback
void OnAudioData(const short* samples, int count)
{
    // Update scope data
    g_appState.scopeData.assign(samples, samples + count);

    // Process through pitch detector
    g_pitchDetector.processBuffer(samples, count);

    // Get the result directly from pitch detector (thread-safe copy)
    ctuner::PitchResult result = g_pitchDetector.getResult();

    // Update app state with results
    if (!g_appState.displayLock) {
        g_appState.currentPitch = result;
        g_appState.spectrumData = g_pitchDetector.getSpectrumData();
        g_appState.maxima.assign(
            g_pitchDetector.getMaxima().begin(),
            g_pitchDetector.getMaxima().end());
        g_appState.maximaCount = g_pitchDetector.getMaximaCount();

        g_appState.spectrumFreq = g_pitchDetector.getFrequencyBin();
        g_appState.spectrumRef = g_pitchDetector.getReferenceBin();
        g_appState.spectrumLow = g_pitchDetector.getLowBin();
        g_appState.spectrumHigh = g_pitchDetector.getHighBin();
    }

    // Log if enabled - use the local copy to avoid race conditions
    if (g_appState.loggingEnabled) {
        g_mainWindow.getLogger().addEntry(result);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    // Create application window
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"CTuner", nullptr
    };
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowW(
        wc.lpszClassName, L"CTuner",
        WS_OVERLAPPEDWINDOW,
        100, 100, 450, 600,
        nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Customize style for tuner
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 4.0f;
    style.WindowRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Initialize application
    g_mainWindow.initialize();

    // Initialize audio
    g_appState.scopeData.resize(ctuner::STEP);
    g_appState.spectrumData.resize(ctuner::RANGE);
    g_appState.maxima.resize(ctuner::MAXIMA);

    g_pitchDetector.setReference(g_appState.referenceFrequency);
    g_pitchDetector.setTemperament(g_appState.currentTemperament);

    g_audioCapture.setCallback(OnAudioData);
    if (!g_audioCapture.start()) {
        MessageBoxA(hwnd, g_audioCapture.getLastError(),
                    "Audio Error", MB_OK | MB_ICONERROR);
    }
    g_appState.audioRunning = g_audioCapture.isRunning();

    // Main loop
    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight,
                                         DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Sync pitch detector settings with app state
        g_pitchDetector.setReference(g_appState.referenceFrequency);
        g_pitchDetector.setTemperament(g_appState.currentTemperament);
        g_pitchDetector.setKey(g_appState.key);
        g_pitchDetector.setFilter(g_appState.audioFilter);
        g_pitchDetector.setDownsample(g_appState.downsample);
        g_pitchDetector.setFundamental(g_appState.fundamentalFilter);
        g_pitchDetector.setNoteFilter(g_appState.noteFilter);
        g_pitchDetector.setFilterSettings(g_appState.filters);

        // Update spectrum view data
        g_mainWindow.getSpectrumView().updateData(
            g_appState.spectrumData,
            g_appState.spectrumFreq,
            g_appState.spectrumRef,
            g_appState.spectrumLow,
            g_appState.spectrumHigh,
            g_appState.maxima,
            g_appState.maximaCount);

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render main window
        g_mainWindow.render(g_appState);

        // Rendering
        ImGui::Render();
        const float clear_color[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);  // VSync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    g_audioCapture.stop();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED) {
        // Try high-performance WARP software driver
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    }

    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                                             WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;

    case WM_SYSCOMMAND:
        // Disable ALT application menu
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        // Keyboard shortcuts
        switch (wParam) {
        case 'O':
            // Toggle options - handled by menu
            break;
        case 'Z':
            g_appState.spectrumZoom = !g_appState.spectrumZoom;
            break;
        case 'F':
            g_appState.audioFilter = !g_appState.audioFilter;
            break;
        case 'D':
            g_appState.downsample = !g_appState.downsample;
            break;
        case 'L':
            g_appState.displayLock = !g_appState.displayLock;
            break;
        case 'M':
            g_appState.multipleNotes = !g_appState.multipleNotes;
            break;
        case 'S':
            g_appState.toggleStrobe();
            break;
        case 'V':
            g_mainWindow.getLogViewer().toggle();
            break;
        case VK_OEM_PLUS:
        case VK_ADD:
            if (g_appState.spectrumExpand < 16)
                g_appState.spectrumExpand *= 2;
            break;
        case VK_OEM_MINUS:
        case VK_SUBTRACT:
            if (g_appState.spectrumExpand > 1)
                g_appState.spectrumExpand /= 2;
            break;
        }
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
