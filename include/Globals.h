// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once

namespace Globals
{
    extern RE::BSGraphics::Renderer* renderer;
    extern RE::BSGraphics::State* state;
    extern REX::W32::ID3D11Device* g_D3D11Device;  
    extern REX::W32::IDXGISwapChain* swapChain;

    inline bool Streamline_Init = false;
    inline bool DLSS_Available = false;
    inline bool SwapChain_Hooked = false;

    // These are the resolutions that the game is rendering at
    inline int RenderResolutionWidth = 0;
    inline int RenderResolutionHeight = 0;
    // This is what DLSS will upscale the game to
    inline int OutputResolutionWidth = 0;
    inline int OutputResolutionHeight = 0;

    void init();
}