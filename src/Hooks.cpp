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

        REX::W32::ID3D11Texture2D* backBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, REX::W32::IID_ID3D11Texture2D, (void**)&backBuffer))) {

            if (Globals::DLSS_Available) {
                Streamline::Streamline::getSingleton()->loadDlSSBuffers();
                Streamline::Streamline::getSingleton()->updateConstants();
                Streamline::Streamline::getSingleton()->HandlePresent();
            }
        }



        // Call original function, otherwise game will cease rendering and effectively freeze
        return func(pSwapChain, SyncInterval, Flags);
    }

    void Install() {
        HkDX11PresentSwapChain::InstallHook();
    }
}