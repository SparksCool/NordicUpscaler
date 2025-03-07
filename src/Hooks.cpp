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
#include "MCP.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>

using Microsoft::WRL::ComPtr;

namespace Hooks {
    ID3D11RenderTargetView* g_BackBufferRTV = nullptr;
    bool g_IsBackBufferActive = false;
    ID3D11Texture2D* pTexture = nullptr;
    int omIndex = 0;
    int finalRenderTargetIdx = 0;

    bool DLSSProcessing = false;
    bool TextureProcessing = false;
    int resized = false;
    ID3D11RenderTargetView* lowResRTV = nullptr;
    ID3D11Texture2D* lowResTexture = nullptr;
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

        finalRenderTargetIdx = omIndex;
        omIndex = 0;
        resized = 0;

        Globals::omIndex = finalRenderTargetIdx;

        

        // Call original function, otherwise game will cease rendering and effectively freeze
        HRESULT hr = func(pSwapChain, SyncInterval, Flags);

        if (FAILED(hr)) {
            logger::warn("Present failed, HRESULT: {:#x}", hr);
            return hr;
        }

        if (Globals::Resize_Queued) {
            Globals::Resize_Queued = false;
            auto context = UNREX_CAST(Globals::context, ID3D11DeviceContext);
            context->ClearState();
            context->Flush();

            auto swapChain = UNREX_CAST(Globals::swapChain, IDXGISwapChain);

            DXGI_SWAP_CHAIN_DESC desc;
            swapChain->GetDesc(&desc);

            HRESULT hr = swapChain->ResizeBuffers(desc.BufferCount, Globals::RenderResolutionWidth, Globals::RenderResolutionHeight,
                                                  desc.BufferDesc.Format, desc.Flags);
            if (FAILED(hr)) {
                
                logger::info("ResizeBuffers failed! HRESULT: {:#X}", hr);
            }
        }

        return S_OK;
    }

    /*END PRESENT HOOK*/

    /*SET RENDER TARGETS HOOK*/


    void HkOMSetRenderTargets::InstallHook() {
        ID3D11DeviceContext* context = UNREX_CAST(Globals::context, ID3D11DeviceContext);

        void** vtable = Utils::get_vtable_ptr(context);  // Get the vtable of the swap chain
        int vIdx = 33;

        func = reinterpret_cast<decltype(func)>(vtable[vIdx]);

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


        // Copy RTV to streamline
        ID3D11RenderTargetView* renderTargetView = rtv[0];
        ID3D11Resource* colorBuffer = nullptr;
        renderTargetView->GetResource(&colorBuffer);


        D3D11_TEXTURE2D_DESC desc;
        auto device = UNREX_CAST(Globals::g_D3D11Device, ID3D11Device);
        UNREX_CAST(colorBuffer, ID3D11Texture2D)->GetDesc(&desc);

        if (desc.Width != Globals::OutputResolutionWidth || desc.Height != Globals::OutputResolutionHeight) {
            colorBuffer->Release();
            return func(ctx, numViews, rtv, dsv);
        }

        // This can be optimized in the future

        bool vanillaCond =
            (!(desc.BindFlags & D3D11_BIND_RENDER_TARGET) || desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM ||
             desc.MipLevels != 1 || desc.ArraySize != 1 || !(desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) ||
             desc.SampleDesc.Count != 1 || desc.SampleDesc.Quality != 0 || desc.CPUAccessFlags != 0 ||
             desc.MiscFlags != 0 || desc.Usage != D3D11_USAGE_DEFAULT || dsv);

        bool enbCond = (!(desc.BindFlags & D3D11_BIND_RENDER_TARGET) || desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM ||
                        desc.MipLevels != 1 || desc.ArraySize != 1 || desc.BindFlags != 40 ||
                        desc.SampleDesc.Count != 1 || desc.SampleDesc.Quality != 0 || desc.CPUAccessFlags != 0 ||
                        desc.MiscFlags != 0 || desc.Usage != D3D11_USAGE_DEFAULT || dsv);
        bool actualCond;

        if (Settings::Enb_Enabled) {
            actualCond = enbCond;
        } else {
            actualCond = vanillaCond;
        }
        
 

         // Check if this does not meet our criteria for a render target
        if (actualCond) {
            colorBuffer->Release();
            return func(ctx, numViews, rtv, dsv);  // Skip if not bindable as render target
        }


        omIndex++;
        auto context = UNREX_CAST(Globals::context, ID3D11DeviceContext);
        context->CopyResource(Streamline::Streamline::getSingleton()->colorBufferShared, colorBuffer);

        Streamline::Streamline* stream = Streamline::Streamline::getSingleton();
        if (Globals::DLSS_Available && Settings::Plugin_Enabled && omIndex == finalRenderTargetIdx + Globals::omOffset && !RE::UI::GetSingleton()->GameIsPaused()) {
            DLSSProcessing = true;
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
        }

        func(ctx, numViews, rtv, dsv);
        
        colorBuffer->Release();
    }

       


    /*END SET RENDER TARGETS HOOK*/
    /*CREATE RENDER TEXTURE HOOK*/

    void HkCreate2DTexture::InstallHook() {
        // Vtable hook
        auto device = UNREX_CAST(Globals::g_D3D11Device, ID3D11Device);

        void** vtable = Utils::get_vtable_ptr(device);  // Get the vtable of the device
        int vIdx = 5;

        func = reinterpret_cast<decltype(func)>(vtable[vIdx]);

        // Overwrite the Present function with our own
        DWORD oldProtect;
        VirtualProtect(&vtable[vIdx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        vtable[vIdx] = &thunk;
        VirtualProtect(&vtable[vIdx], sizeof(void*), oldProtect, &oldProtect);
    }

    HRESULT __stdcall HkCreate2DTexture::thunk(ID3D11Device* pDevice, const D3D11_TEXTURE2D_DESC* pDesc,
                                            const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
    {
        //float scale = Globals::getScaleFactor();

        D3D11_TEXTURE2D_DESC newDesc = *pDesc;

        if (newDesc.Width == Globals::OutputResolutionWidth && newDesc.Height == Globals::OutputResolutionHeight) {
            newDesc.Width = Globals::RenderResolutionWidth;
            newDesc.Height = Globals::RenderResolutionHeight;
        }

        return func(pDevice, &newDesc, pInitialData, ppTexture2D);
    }

    /*END CREATE RENDER TEXTURE HOOK*/


    void earlyInstall() { 
        
    }

    void Install() {
       // HkCreate2DTexture::InstallHook();
        HkDX11PresentSwapChain::InstallHook();
        HkOMSetRenderTargets::InstallHook();
        
    }

    




}