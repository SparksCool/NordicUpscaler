#pragma once

namespace Globals
{
    extern RE::BSGraphics::Renderer* renderer;
    extern REX::W32::ID3D11Device* g_D3D11Device;  // Added type specifier and extern keyword

    inline bool Streamline_Init = false;
    inline bool DLSS_Available = false;

    void init();
}