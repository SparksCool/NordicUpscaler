// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include <sl_core_types.h>

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

    inline bool Streamline_Init = false;
    inline bool DLSS_Available = false;
    inline bool SwapChain_Hooked = false;

    // These are the resolutions that the game is rendering at
    inline int RenderResolutionWidth = 0;
    inline int RenderResolutionHeight = 0;
    // This is what DLSS will upscale the game to
    inline int OutputResolutionWidth = 0;
    inline int OutputResolutionHeight = 0;

    // DLSS Buffers
    inline ID3D11Texture2D* colorBufferShared = nullptr;
    inline ID3D11Texture2D* depthBufferShared = nullptr;
    inline ID3D11Texture2D* motionVectorsShared = nullptr;
    inline ID3D11Texture2D* dlssOutputBuffer = nullptr;
    // Viewport
    extern sl::ViewportHandle viewport;

    void init();
}