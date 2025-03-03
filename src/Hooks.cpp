// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include "Hooks.h"
#include "Globals.h"
#include <sl.h>  
#include <Streamline.h>
#include <Settings.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Hooks {
    ID3D11RenderTargetView* g_BackBufferRTV = nullptr;
    bool g_IsBackBufferActive = false;
    ID3D11Texture2D* pTexture = nullptr;
    int omIndex = 0;
    bool DLSSProcessing = false;
    int FrameViewPortChanged = 0;
    inline int& MaxFrameViewPortChanged = Settings::MaxFrameViewPortUpdates;
    // Hooks into the DirectX 11 swap chain's Present function to intercept frame rendering
    void HkDX11PresentSwapChain::InstallHook() { 
        if (!Globals::swapChain) {
            logger::warn("Swap chain not found, skipping hook installation.");
            return;
        }

        Globals::SwapChain_Hooked = true;

        void** vtable = *reinterpret_cast<void***>(Globals::swapChain);  // Get the vtable of the swap chain
        func = reinterpret_cast<decltype(func)>(vtable[8]);  // Present is the 8th function in the swap chain's vtable

        // Overwrite the Present function with our own
        DWORD oldProtect;
        VirtualProtect(&vtable[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        vtable[8] = &thunk;
        VirtualProtect(&vtable[8], sizeof(void*), oldProtect, &oldProtect);
    }

    HRESULT WINAPI HkDX11PresentSwapChain::thunk(REX::W32::IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {

        if (!pSwapChain) {
            logger::warn("Swap chain not found, skipping hook.");
            return func(pSwapChain, SyncInterval, Flags);
        }

        FrameViewPortChanged = 0;

        Streamline::Streamline* stream = Streamline::Streamline::getSingleton();

            if (Globals::DLSS_Available && Settings::Plugin_Enabled) {
            DLSSProcessing = true;
                //Streamline::Streamline::getSingleton()->loadDlSSBuffers(); This has been moved inside the
                // HandlePresent function
               stream->updateConstants();

               stream->HandlePresent();

                // Mess with RenderTargets
               ID3D11Device* device = UNREX_CAST(Globals::g_D3D11Device, ID3D11Device);
               ID3D11DeviceContext* context = UNREX_CAST(Globals::context, ID3D11DeviceContext);

               ID3D11RenderTargetView* renderTargetView = nullptr;
               ID3D11DepthStencilView* depthStencilView = nullptr;
               context->OMGetRenderTargets(1, &renderTargetView, &depthStencilView);
               if (renderTargetView) {
                   // Replace the render target using the shared color buffer
                   ID3D11Resource* colorBuffer = nullptr;
                   renderTargetView->GetResource(&colorBuffer);
                   context->CopyResource(colorBuffer, stream->colorBufferShared);
                   context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
                   colorBuffer->Release();
                   renderTargetView->Release();
               }

               DLSSProcessing = false;
            }
        // Call original function, otherwise game will cease rendering and effectively freeze
        return func(pSwapChain, SyncInterval, Flags);
    }

    void hkRSSetViewports::InstallHook() {
        logger::info("Installing RSSetViewports hook...");

        // Get the device context
        ID3D11DeviceContext* context = nullptr;
        ID3D11Device* device = UNREX_CAST(Globals::g_D3D11Device, ID3D11Device);
        device->GetImmediateContext(&context);
        if (!context) {
            logger::error("Failed to get ID3D11DeviceContext");
            return;
        }

        // Get VTable from device context
        uintptr_t* vtable = *reinterpret_cast<uintptr_t**>(context);

        // Store the original function
        func = reinterpret_cast<decltype(func)>(vtable[44]);  // RSSetViewports is at index 44

        // Replace with our hook function
        DWORD oldProtect;
        VirtualProtect(&vtable[44], sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect);
        vtable[44] = reinterpret_cast<uintptr_t>(&thunk);
        VirtualProtect(&vtable[44], sizeof(uintptr_t), oldProtect, &oldProtect);

        logger::info("RSSetViewports hook installed!");
    }

    void __stdcall hkRSSetViewports::thunk(ID3D11DeviceContext* pContext, UINT NumViewports,
                                           const D3D11_VIEWPORT* pViewports) {
        if (DLSSProcessing || FrameViewPortChanged >= MaxFrameViewPortChanged || !Settings::Plugin_Enabled) {
            func(pContext, 1, pViewports);
            return;
        }
        // Handle our excluded viewports
        if (FrameViewPortChanged == VIEWPORT_GAME_P2 || 
            FrameViewPortChanged == VIEWPORT_COLOR_P2 ||
            FrameViewPortChanged == VIEWPORT_GRASS_SHDW_P2 ||
            FrameViewPortChanged == VIEWPORT_MCP) {
            // Skip this run of the function to prevent issues
            FrameViewPortChanged += 1;
            func(pContext, 1, pViewports);
            return;
        }


        std::vector<D3D11_VIEWPORT> modifiedViewports(pViewports, pViewports + NumViewports);

        // Only modify the scene viewport(s) behind the UI.
        // Heuristic: assume the first viewport corresponds to the scene.
        if (!modifiedViewports.empty()) {
            // Log the viewport dimensions.
            logger::info("Viewport dimensions: {}x{}", modifiedViewports[0].Width, modifiedViewports[0].Height);
            if (modifiedViewports[0].Width == Globals::OutputResolutionWidth &&
                modifiedViewports[0].Height == Globals::OutputResolutionHeight) {

                modifiedViewports[0].Width = static_cast<float>(Globals::RenderResolutionWidth);
                modifiedViewports[0].Height = static_cast<float>(Globals::RenderResolutionHeight);
                logger::info("ViewPort Changed: {}", FrameViewPortChanged);
            }
            FrameViewPortChanged += 1;
        }

        // Call the original RSSetViewports with the (possibly) modified viewport array.
        func(pContext, NumViewports, modifiedViewports.data());
    }

    void earlyInstall() { 
    }

    void Install() {
        g_BackBufferRTV = nullptr;
        g_IsBackBufferActive = false;

        hkRSSetViewports::InstallHook();
        HkDX11PresentSwapChain::InstallHook();
    }

}