// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include <Globals.h>
#include <Utils.h>
#include <Streamline.h>
#include <sl.h>

namespace Globals {
    // The skyrim renderer 
    RE::BSGraphics::Renderer* renderer = nullptr;
    // The Skyrim Graphics State
    RE::BSGraphics::State* graphicsState = nullptr;
    // The D3D11 device used by the renderer AKA our GPU
    REX::W32::ID3D11Device* g_D3D11Device = nullptr;
    // The swap chain used by the renderer
    REX::W32::IDXGISwapChain* swapChain = nullptr;
    // The context used by the renderer
    REX::W32::ID3D11DeviceContext* context = nullptr;
    // Viewport
    sl::ViewportHandle viewport {0};

    // Initalize all globals
    void init() {
        renderer = RE::BSGraphics::Renderer::GetSingleton(); 
        logger::info("Renderer load attempted");
        graphicsState = RE::BSGraphics::State::GetSingleton();
        logger::info("State load attempted");
        g_D3D11Device = renderer->GetDevice();
        logger::info("D3D11 Device load attempted");
        swapChain = renderer->GetCurrentRenderWindow()->swapChain;
        logger::info("Swap Chain: load attempted");
        context = renderer->GetRendererData()->context;
        logger::info("Context load attempted");

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

        /* Set our target render res */
        OutputResolutionWidth = renderer->GetScreenSize().width;
        OutputResolutionHeight = renderer->GetScreenSize().height;

        /* Initialize Streamline then check if DLSS is available */
        Streamline::Streamline::getSingleton()->loadInterposer();
        Streamline_Init = Streamline::Streamline::getSingleton()->initSL();
        DLSS_Available = Streamline::Streamline::getSingleton()->DLSSAvailable();
        Streamline::Streamline::getSingleton()->loadDlSSBuffers();
        Streamline::Streamline::getSingleton()->getDLSSRenderResolution();
        
        logger::info("Globals initialized.");
    }
}