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

    struct hkRSSetViewports {
        static void InstallHook();
        static void WINAPI thunk(ID3D11DeviceContext* pContext, UINT NumViewports, const D3D11_VIEWPORT* pViewports);
        static inline decltype(&thunk) func;
    };

    struct hkDX11ExcCmdList {
        static void InstallHook();
        static void WINAPI thunk();
        static inline decltype(&thunk) func;

    };

}