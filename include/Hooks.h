// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once
#include <REX/W32/DXGI.h>
#include <d3d11.h>        
#include <dxgi.h>       
#include <dxgi1_2.h>

namespace Hooks {
    extern ID3D11RenderTargetView* g_BackBufferRTV;
    extern bool g_IsBackBufferActive;
    extern int omIndex;

    void Install();
    void earlyInstall();

    struct HkDX11PresentSwapChain {
        static void InstallHook();
        static HRESULT WINAPI thunk(REX::W32::IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        static inline decltype(&thunk) func;
    };

    struct hkOMSetRenderTargets {
        static void thunk(ID3D11DeviceContext* This, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews,
                          ID3D11DepthStencilView* pDepthStencilView);
        static inline decltype(&thunk) func;
        static void InstallHook();
    };

    struct hkCreateRenderTarget {
        static void thunk(RE::BSGraphics::Renderer* This, RE::RENDER_TARGETS::RENDER_TARGET a_target,
                          RE::BSGraphics::RenderTargetProperties* a_properties);
        static inline REL::Relocation<decltype(&thunk)> func;
        static void InstallHook();
    };

}