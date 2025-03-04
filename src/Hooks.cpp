// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include "Hooks.h"
#include "Globals.h"
#include <sl.h>  
#include "Utils.h"
#include <Streamline.h>
#include <Settings.h>
#include <wrl/client.h>
#include <unordered_set>

using Microsoft::WRL::ComPtr;

namespace Hooks {
    ID3D11RenderTargetView* g_BackBufferRTV = nullptr;
    bool g_IsBackBufferActive = false;
    ID3D11Texture2D* pTexture = nullptr;

    bool DLSSProcessing = false;
    /*PRESENT HOOK*/

    // Hooks into the DirectX 11 swap chain's Present function to intercept frame rendering
    void HkDX11PresentSwapChain::InstallHook() { 
        if (!Globals::swapChain) {
            logger::warn("Swap chain not found, skipping hook installation.");
            return;
        }

        Globals::SwapChain_Hooked = true;

        void** vtable = Utils::get_vtable_ptr(Globals::swapChain);  // Get the vtable of the swap chain

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
                   context->CopyResource(colorBuffer, stream->colorOutBufferShared);
                   context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
                   colorBuffer->Release();
                   renderTargetView->Release();
               }

               DLSSProcessing = false;
            } else {
                DLSSProcessing = true;
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

    /*END PRESENT HOOK*/

    /*SET RENDER TARGETS HOOK*/


    void HkOMSetRenderTargets::InstallHook() {
        ID3D11DeviceContext* context = UNREX_CAST(Globals::context, ID3D11DeviceContext);

        void** vtable = Utils::get_vtable_ptr(context);  // Get the vtable of the swap chain
        int vIdx = 33;

        func = reinterpret_cast<decltype(func)>(vtable[vIdx]);  // Present is the 8th function in the swap chain's vtable

        // Overwrite the Present function with our own
        DWORD oldProtect;
        VirtualProtect(&vtable[vIdx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        vtable[vIdx] = &thunk;
        VirtualProtect(&vtable[vIdx], sizeof(void*), oldProtect, &oldProtect);
    }

    void __stdcall HkOMSetRenderTargets::thunk(ID3D11DeviceContext* ctx, UINT numViews,
                                               ID3D11RenderTargetView* const* rtv, ID3D11DepthStencilView* dsv) {
        if (DLSSProcessing) {
            return func(ctx, numViews, rtv, dsv);
        }

        float scale = Globals::getScaleFactor();

        if (!rtv || !rtv[0] || numViews != 1 || scale >= 1.0f) {
            return func(ctx, numViews, rtv, dsv);
        }

        func(ctx, numViews, rtv, dsv);
        
    }
       


    /*END SET RENDER TARGETS HOOK*/


    void earlyInstall() { 
    }

    void Install() {
        HkDX11PresentSwapChain::InstallHook();
        HkOMSetRenderTargets::InstallHook();
    }

    




}