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
            RE::BSGraphics::Renderer* renderer = Globals::renderer;
            auto state = Globals::graphicsState;

            // Retrieve game buffers
            auto& swapChain = renderer->GetRendererData()->renderTargets[RE::RENDER_TARGET::kFRAMEBUFFER];
            auto& motionVectors = renderer->GetRendererData()->renderTargets[RE::RENDER_TARGETS::RENDER_TARGET::kMOTION_VECTOR];
            auto& depth = renderer->GetRendererData()->depthStencils[RE::RENDER_TARGETS_DEPTHSTENCIL::kPOST_ZPREPASS_COPY];

            // Add the necessary type alias to match the expected parameter type
            ID3D11Resource* swapChainResource = nullptr;
            swapChain.SRV->GetResource(&swapChainResource);
            

            if (Globals::DLSS_Available) {
                sl::FrameToken* frameToken = nullptr;

                auto context = Globals::context;

                // Copy color buffer
                context->CopyResource(REX_CAST(Globals::colorBufferShared, ID3D11Texture2D), REX_CAST(swapChainResource, ID3D11Texture2D));
                context->CopyResource(REX_CAST(Globals::dlssOutputBuffer, ID3D11Texture2D), backBuffer);

                // Copy motion vectors
                if (motionVectors.SRV) {
                    ID3D11Resource* motionVectorsResource = nullptr;
                    motionVectors.SRV->GetResource(&motionVectorsResource);
                    context->CopyResource(REX_CAST(Globals::motionVectorsShared, ID3D11Texture2D),
                                          REX_CAST(motionVectorsResource, ID3D11Texture2D));
                }

                // Copy depth buffer
                if (depth.depthSRV) {
                    ID3D11Resource* depthResource = nullptr;
                    depth.depthSRV->GetResource(&depthResource);
                    context->CopyResource(REX_CAST(Globals::depthBufferShared, ID3D11Texture2D),
                                          REX_CAST(depthResource, ID3D11Texture2D));
                }

                sl::ViewportHandle view{Globals::viewport};

                // Update constants (dummy values for now)
                sl::Constants constants = {};
                // dummy values for now
                constants.cameraViewToClip = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};  // Identity matrix
                constants.clipToCameraView = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};  // Identity matrix
                constants.clipToLensClip = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};    // Identity matrix
                constants.clipToPrevClip = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};    // Identity matrix
                constants.prevClipToClip = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};    // Identity matrix
                constants.jitterOffset = {0.0f, 0.0f};
                constants.mvecScale = {1.0f, 1.0f};
                constants.cameraPinholeOffset = {1.f, 0.f};
                constants.cameraPos = {1.0f, 1.0f, 1.0f};
                constants.cameraUp = {0.0f, 1.0f, 0.0f};
                constants.cameraRight = {1.0f, 0.0f, 0.0f};
                constants.cameraFwd = {0.0f, 0.0f, -1.0f};
                constants.cameraNear = (*(float*)(REL::RelocationID(517032, 403540).address() + 0x40));
                constants.cameraFar = (*(float*)(REL::RelocationID(517032, 403540).address() + 0x44));
                constants.cameraFOV = 90.0f;
                constants.cameraAspectRatio = 16.0f / 9.0f;
                constants.motionVectorsInvalidValue = 1.0f;
                constants.depthInverted = sl::Boolean::eFalse;
                constants.cameraMotionIncluded = sl::Boolean::eTrue;
                constants.motionVectors3D = sl::Boolean::eFalse;
                constants.reset = sl::Boolean::eFalse;
                constants.orthographicProjection = sl::Boolean::eFalse;
                constants.motionVectorsDilated = sl::Boolean::eFalse;
                constants.motionVectorsJittered = sl::Boolean::eFalse;


                if (SL_FAILED(res, slGetNewFrameToken(frameToken, nullptr))) {
                    logger::warn("Failed to get new frame token");
                    return func(pSwapChain, SyncInterval, Flags);
                }

                if (SL_FAILED(res, slSetConstants(constants, *frameToken, view))) {
                    logger::warn("Failed to set constants: {}", static_cast<int>(res));
                }

                const sl::BaseStructure* inputs[] = {&view};

                
                Streamline::Streamline::getSingleton()->loadDlSSBuffers();

                if (SL_FAILED(res, slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, _countof(inputs), Globals::context))) {
                    logger::warn("Failed to evaluate DLSS feature: {}", static_cast<int>(res));
                } else {
                    logger::info("DLSS feature evaluated successfully: {}", static_cast<int>(res));
                }


                context->CopyResource(backBuffer, REX_CAST(Globals::dlssOutputBuffer, ID3D11Texture2D)); // Copy the DLSS output buffer to the back buffer
                D3D11_TEXTURE2D_DESC backBufferDesc, dlssOutputDesc;
                // Globals::dlssOutputBuffer->GetDesc(&dlssOutputDesc);
                backBuffer->GetDesc(REX_CAST(&backBufferDesc, D3D11_TEXTURE2D_DESC));

                logger::info("BackBuffer: {}x{}", backBufferDesc.Width, backBufferDesc.Height);
                // logger::info("DLSS Output Buffer: {}x{}", dlssOutputDesc.Width, dlssOutputDesc.Height);
            }

            swapChainResource->Release();
            backBuffer->Release();
        }



        // Call original function, otherwise game will cease rendering and effectively freeze
        return func(pSwapChain, SyncInterval, Flags);
    }

    void Install() {
        HkDX11PresentSwapChain::InstallHook();
    }
}