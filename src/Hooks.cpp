// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include "Hooks.h"
#include "Globals.h"
#include <sl.h>
#include <d3d11.h>        
#include <dxgi.h>         
#include <Streamline.h>

namespace Hooks {
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

            if (Globals::DLSS_Available) {
                // Streamline::Streamline::getSingleton()->loadDlSSBuffers(); This has been moved inside the
                // HandlePresent function
                Streamline::Streamline::getSingleton()->updateConstants();
                Streamline::Streamline::getSingleton()->HandlePresent();
            }



        // Call original function, otherwise game will cease rendering and effectively freeze
        return func(pSwapChain, SyncInterval, Flags);
    }

    void HkDRS::thunk(RE::BSGraphics::State* a_state) {
        // Set Dynamic Resolution ratios to be the same as the render resolution
        a_state->dynamicResolutionHeightRatio = Globals::getScaleFactor();
        a_state->dynamicResolutionWidthRatio = Globals::getScaleFactor();
        logger::info("Dynamic Resolution Ratios set to: {}", Globals::getScaleFactor());
    }
    void HkDRS::InstallHook() {
        SKSE::AllocTrampoline(14);

        auto& trampoline = SKSE::GetTrampoline();
        trampoline.write_call<5>(REL::RelocationID(35556, 36555).address() + REL::Relocate(0x2D, 0x2D, 0x25), thunk);

        Globals::DRS_Active = true;
    }

    void Install() {
        HkDX11PresentSwapChain::InstallHook();
        HkDRS::InstallHook();
    }
}