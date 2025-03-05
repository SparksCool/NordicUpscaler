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

        Globals::omIndex = finalRenderTargetIdx;

        

        // Call original function, otherwise game will cease rendering and effectively freeze
        return func(pSwapChain, SyncInterval, Flags);
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
        if (DLSSProcessing || !Settings::Plugin_Enabled) {
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

        // Return if not output resolution
        D3D11_TEXTURE2D_DESC desc;
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

    void HkCreateRenderTexture::InstallHook() {
        SKSE::AllocTrampoline(14);
        auto& trampoline = SKSE::GetTrampoline();

        // For specific function size, hard-code the appropriate write_call size (like 6 or 5)
        func = trampoline.write_call<6>(RELOCATION_ID(75507, 77299).address(), thunk);
        stl::write_thunk_call<HkCreateRenderTexture>(RELOCATION_ID(75507, 77299).address());
    }

    RE::NiTexture::RendererData* __stdcall HkCreateRenderTexture::thunk(RE::BSGraphics::Renderer* This, std::uint32_t width,
                                                   std::uint32_t height) {
        //float scale = Globals::getScaleFactor();

        if (width == Globals::OutputResolutionWidth && height == Globals::OutputResolutionHeight) {
            width = Globals::RenderResolutionWidth;
            height = Globals::RenderResolutionHeight;
        }

       return func(This, width, height);
    }

    /*END CREATE RENDER TEXTURE HOOK*/


    void earlyInstall() { 
        //HkCreateRenderTexture::InstallHook();
    }

    void Install() {
        HkDX11PresentSwapChain::InstallHook();
        HkOMSetRenderTargets::InstallHook();
        
    }

    




}