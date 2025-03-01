// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once
#include <REX/W32/DXGI.h>
#include <d3d11.h>        
#include <dxgi.h>       

namespace Hooks {
    extern ID3D11RenderTargetView* g_BackBufferRTV;
    extern bool g_IsBackBufferActive;

    void Install();

    struct HkDX11PresentSwapChain {
        static void InstallHook();
        static HRESULT WINAPI thunk(REX::W32::IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        static inline decltype(&thunk) func;
    };

    struct HkDRS {
        static void thunk(RE::BSGraphics::State* a_state);
        static inline decltype(&thunk) func;
        static void InstallHook();
    };

    struct HkRSSetViewports {
        static void thunk(ID3D11DeviceContext* This, UINT NumViewports, const D3D11_VIEWPORT* pViewports);
        static inline decltype(&thunk) func;
        static void InstallHook();
    };

    struct hkOMSetRenderTargets {
        static void thunk(ID3D11DeviceContext* This, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews,
                          ID3D11DepthStencilView* pDepthStencilView);
        static inline decltype(&thunk) func;
        static void InstallHook();
    };
}