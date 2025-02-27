// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once

namespace Globals
{
    extern RE::BSGraphics::Renderer* renderer;
    extern REX::W32::ID3D11Device* g_D3D11Device;  
    extern REX::W32::IDXGISwapChain* swapChain;

    inline bool Streamline_Init = false;
    inline bool DLSS_Available = false;
    inline bool SwapChain_Hooked = false;

    void init();
}