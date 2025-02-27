// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include <Globals.h>
#include <Utils.h>
#include <Streamline.h>

namespace Globals {
    // The skyrim renderer 
    RE::BSGraphics::Renderer* renderer = nullptr;
    // The D3D11 device used by the renderer AKA our GPU
    REX::W32::ID3D11Device* g_D3D11Device = nullptr;
    // The swap chain used by the renderer
    REX::W32::IDXGISwapChain* swapChain = nullptr;

    // Initalize all globals
    void init() {
        renderer = RE::BSGraphics::Renderer::GetSingleton(); 
        logger::info("Renderer load attempted");
        g_D3D11Device = renderer->GetDevice();
        logger::info("D3D11 Device load attempted");
        swapChain = renderer->GetCurrentRenderWindow()->swapChain;
        logger::info("Swap Chain: load attempted");

        // Null checks

        if (!g_D3D11Device) {
            logger::warn("Failed to get D3D11 device.");
        }
        
        if (!swapChain) {
            logger::warn("Failed to get swap chain.");
        }

        if (!renderer) {
            logger::warn("Failed to get renderer.");
        }

        // Null checks end


        /* Initialize Streamline then check if DLSS is available */
        Streamline::Streamline::getSingleton()->loadInterposer();
        Streamline_Init = Streamline::Streamline::getSingleton()->initSL();
        DLSS_Available = Streamline::Streamline::getSingleton()->DLSSAvailable();
    }
}