// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include <sl_core_types.h>
#include <dxgi1_2.h>

// Defines for specific viewports and what buffer they correspond to

#define VIEWPORT_GAME_P2 19
#define VIEWPORT_UI 22
#define VIEWPORT_MOUSE 23
#define VIEWPORT_MCP 24
#define VIEWPORT_COLOR 31
#define VIEWPORT_LANDSCP 37
#define VIEWPORT_GRASS_SHDW_P2 44
#define VIEWPORT_COLOR_P2 59
#define VIEWPORT_HUD 117

// End defines for specific viewports and what buffer they correspond to

#pragma once
#define REX_CAST(resource, type) reinterpret_cast<REX::W32::type*>(resource)
#define UNREX_CAST(resource, type) reinterpret_cast<type*>(resource)

namespace Globals
{
    extern RE::BSGraphics::Renderer* renderer;
    extern RE::BSGraphics::State* graphicsState;
    extern REX::W32::ID3D11Device* g_D3D11Device;  
    extern REX::W32::IDXGISwapChain* swapChain;
    extern REX::W32::ID3D11DeviceContext* context;
    extern IDXGIFactory2* factory;
    extern int omIndex;
    extern int omOffset;

    inline bool Streamline_Init = false;
    inline bool DLSS_Available = false;
    inline bool SwapChain_Hooked = false;
    inline bool DRS_Active = false;
    inline bool Resize_Queued = false;

    // These are the resolutions that the game is rendering at
    inline int RenderResolutionWidth = 0;
    inline int RenderResolutionHeight = 0;
    // This is what DLSS will upscale the game to
    inline int OutputResolutionWidth = 0;
    inline int OutputResolutionHeight = 0;

    // SSE Display Tweaks ini paths
    inline const char* displayTweaksIni{"Data/SKSE/Plugins/SSEDisplayTweaks.ini"};
    inline const char* customDisplayTweaksIni{"Data/SKSE/Plugins/SSEDisplayTweaks_custom.ini"};


    void earlyInit();
    void fullInit();
    float getScaleFactor();
}