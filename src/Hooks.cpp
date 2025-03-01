// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include "Hooks.h"
#include "Globals.h"
#include <sl.h>  
#include <Streamline.h>
#include <Settings.h>

namespace Hooks {
    ID3D11RenderTargetView* g_BackBufferRTV = nullptr;
    bool g_IsBackBufferActive = false;
    ID3D11Texture2D* pTexture = nullptr;
    int omIndex = 0;
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
        Streamline::Streamline* stream = Streamline::Streamline::getSingleton();

            if (Globals::DLSS_Available && Settings::Plugin_Enabled) {
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

            }


        // Call original function, otherwise game will cease rendering and effectively freeze
        return func(pSwapChain, SyncInterval, Flags);
    }

    void HkDRS::thunk(RE::BSGraphics::State* a_state) {

    }
    void HkDRS::InstallHook() {
        SKSE::AllocTrampoline(14);

        auto& trampoline = SKSE::GetTrampoline();
        trampoline.write_call<5>(REL::RelocationID(35556, 36555).address() + REL::Relocate(0x2D, 0x2D, 0x25), thunk);

        Globals::DRS_Active = true;
    }

    void HkRSSetViewports::thunk(ID3D11DeviceContext* This, UINT NumViewports, const D3D11_VIEWPORT* pViewports) {

        if (NumViewports == 1) {
            if (pViewports[0].Height == Globals::RenderResolutionHeight &&
                pViewports[0].Width == Globals::RenderResolutionWidth) {
                D3D11_VIEWPORT viewport = pViewports[0];
                viewport.Height = Globals::OutputResolutionHeight;
                viewport.Width = Globals::OutputResolutionWidth;
                func(This, NumViewports, &viewport);
                return;
            }
        }

        func(This, NumViewports, pViewports);
    }

    void HkRSSetViewports::InstallHook() {
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

void hkOMSetRenderTargets::thunk(ID3D11DeviceContext* This, UINT NumViews,
                                  ID3D11RenderTargetView* const* ppRenderTargetViews,
                                  ID3D11DepthStencilView* pDepthStencilView) {
    logger::info("OMSetRenderTargets hook called");

    auto& renderer = Globals::renderer;
    auto& frameBuffer = renderer->GetRendererData()->renderTargets[RE::RENDER_TARGET::kFRAMEBUFFER];
    
    if (!ppRenderTargetViews[0] && pDepthStencilView) {
        logger::warn("Frame buffer render target view not found");
        return;
    }

    if (omIndex == 4) {
        omIndex = 0;
        g_IsBackBufferActive = true;
        logger::info("Back buffer is active");
    } else {
        g_IsBackBufferActive = false;
    }

    omIndex++;

    // Call the original function
    func(This, NumViews, ppRenderTargetViews, pDepthStencilView);
}

    void hkOMSetRenderTargets::InstallHook() {
       logger::info("Installing hkOMSetRenderTargets hook...");

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
       func = reinterpret_cast<decltype(func)>(vtable[33]);  // RSSetViewports is at index 44

       // Replace with our hook function
       DWORD oldProtect;
       VirtualProtect(&vtable[33], sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect);
       vtable[33] = reinterpret_cast<uintptr_t>(&thunk);
       VirtualProtect(&vtable[33], sizeof(uintptr_t), oldProtect, &oldProtect);

       logger::info("hkOMSetRenderTargets hook installed!");
    }

    void earlyInstall() { 
        
    }

    void Install() {
        g_BackBufferRTV = nullptr;
        g_IsBackBufferActive = false;

        //HkRSSetViewports::InstallHook();
        HkDX11PresentSwapChain::InstallHook();
        hkOMSetRenderTargets::InstallHook();
        //HkDRS::InstallHook();
    }



}