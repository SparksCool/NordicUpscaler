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

    struct HkOMSetRenderTargets {
        static void InstallHook();
        static void WINAPI thunk(ID3D11DeviceContext* ctx, UINT numViews, ID3D11RenderTargetView* const* rtv,
                                 ID3D11DepthStencilView* dsv);
        static inline decltype(&thunk) func;
    };

    struct HkCreate2DTexture {
        static void InstallHook();
        static HRESULT WINAPI thunk(ID3D11Device* pDevice,
        const D3D11_TEXTURE2D_DESC* pDesc,
        const D3D11_SUBRESOURCE_DATA* pInitialData,
        ID3D11Texture2D** ppTexture2D);
        static inline decltype(&thunk) func;
    };

}