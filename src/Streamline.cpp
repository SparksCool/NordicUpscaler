// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include <Streamline.h>
#include <Globals.h>
#include <Utils.h>
#include <sl.h>
#include <sl_consts.h>
#include <sl_core_api.h>
#include <sl_dlss.h>
#include <sl_result.h>
#include <Settings.h>
#include <d3d11.h>

namespace Streamline {

    // Setup variables
    sl::Constants constants = {};

    void Streamline::loadInterposer() {
        // TODO(SparksCool): Ensure this is compatible with community shaders sl.interposer.dll
        interposer = LoadLibraryW(L"Data/SKSE/Plugins/Streamline/sl.interposer.dll");
    }

    bool Streamline::initSL() {

        sl::Preferences pref{};

        static const sl::Feature features[] = {sl::kFeatureDLSS, sl::kFeatureImGUI};
        pref.featuresToLoad = features;
        pref.numFeaturesToLoad = sizeof(features) / sizeof(sl::Feature);

        // Need to set these otherwise we cannot use DLSS features
        pref.engine = sl::EngineType::eCustom;
        pref.engineVersion = "1.0.0";
        pref.renderAPI = sl::RenderAPI::eD3D11;
        pref.projectId = "e2bf2b68-3fb3-4393-9f62-3b1239a8128e";

#ifndef NDEBUG
        // Only enabling during debug, otherwise it might cause a lot of spam in NordicUpscaler.log + the DLSS console
        // is annoying for end users
        pref.showConsole = true;
        pref.logLevel = sl::LogLevel::eVerbose;
        pref.logMessageCallback = [](sl::LogType type, const char* msg) { logger::info("{}", msg); };
#endif

        sl::Result result = slInit(pref);

        // This helps to debug, cross reference results with the result enum contained in sl_result.h
        // more descriptive messages might be added in the future
        logger::info("[Streamline] Streamline start attempted with result: {}", static_cast<int>(result));

        // Set D3D device to the one actually being used by the game renderer
        //Globals::g_D3D11Device = RE::BSGraphics::Renderer::GetSingleton()->GetDevice();
        //sl::Result dev_result = slSetD3DDevice(Globals::g_D3D11Device);

        //logger::info("[Streamline] Set D3D11 device with result: {}", static_cast<int>(dev_result));

        return result == sl::Result::eOk;
    }

    /* Primarily referenced from https://github.com/NVIDIAGameWorks/Streamline/blob/main/docs/ProgrammingGuideDLSS.md
     * part 2.0
     * This might need to be reworked as this cycles through ALL adapters while we should really only have it use Globals::g_D3D11Device
     * Otherwise we could face issues where theres two adapters e.g an iGPU and a dGPU, SLI too but its kind of extinct in this day */
    bool Streamline::DLSSAvailable() {

        logger::info("Setting Streamline Device");
        slSetD3DDevice(Globals::g_D3D11Device);

        Microsoft::WRL::ComPtr<REX::W32::IDXGIFactory> factory;

        if (FAILED(CreateDXGIFactory(REX::W32::IID_IDXGIFactory, (void**)&factory))) {
            logger::warn("[Streamline] Failed to create DXGI factory.");
            return false;
        }

        Microsoft::WRL::ComPtr<REX::W32::IDXGIAdapter> adapter{};
        uint32_t adapterIndex = 0;
        while (factory->EnumAdapters(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND) {
            REX::W32::DXGI_ADAPTER_DESC desc{};

            if (SUCCEEDED(adapter->GetDesc(&desc))) {
                sl::AdapterInfo adapterInfo{};
                adapterInfo.deviceLUID = (uint8_t*)&desc.adapterLuid;
                adapterInfo.deviceLUIDSizeInBytes = sizeof(LUID);

                if (SL_FAILED(result, slIsFeatureSupported(sl::kFeatureDLSS, adapterInfo))) {
                    // Requested feature is not supported on the system, fallback to the default method
                    logger::warn("[Streamline] DLSS is not supported on this adapter. Error: {}",
                                 static_cast<int>(result));
                } else {
                    // Feature is supported on this adapter!
                    logger::info("[Streamline] DLSS is supported on this adapter.");

                    return true;
                }
            }
            adapterIndex++;
        }

        return false;
    }

    // This function is called to get the optimal render resolution for DLSS
    void Streamline::getDLSSRenderResolution() { 
        sl::DLSSOptions options{};
        options.mode = static_cast<sl::DLSSMode>(Settings::Selected_Preset_DLSS);
        options.outputWidth = Globals::OutputResolutionWidth;
        options.outputHeight = Globals::OutputResolutionHeight;
        slSetFeatureLoaded(sl::kFeatureImGUI, false);

        if (SL_FAILED(res, slSetFeatureLoaded(sl::kFeatureImGUI, true))) {
            logger::warn("[Streamline] Failed to set ImGui feature loaded: {}", static_cast<int>(res));
        }

        // Set our settings
        if (SL_FAILED(result, slDLSSSetOptions(viewport, options))) {
            logger::warn("[Streamline] Failed to set DLSS options: {}", static_cast<int>(result));
            return;
        } else {
            logger::info("[Streamline] Set DLSS options successfully.");
        }

        sl::DLSSOptimalSettings optimalSettings{};
        if (slDLSSGetOptimalSettings(options, optimalSettings) == sl::Result::eOk) {
            Globals::RenderResolutionWidth = optimalSettings.optimalRenderWidth;
            Globals::RenderResolutionHeight = optimalSettings.optimalRenderHeight;

            logger::info("[DLSS] Set global render resolution to {}x{}", Globals::RenderResolutionHeight,
                         Globals::RenderResolutionWidth);
        } else {
            logger::error("[DLSS] Failed to retrieve optimal DLSS resolution.");
        }
    }

    void Streamline::allocateBuffers() {
        logger::info("Allocating DLSS shared buffers...");

        HRESULT hr = S_OK;

        // --- Allocate Color Buffer ---
        {
            D3D11_TEXTURE2D_DESC colorDesc = {};
            colorDesc.Width = Globals::OutputResolutionWidth; 
            colorDesc.Height = Globals::OutputResolutionHeight;
            colorDesc.MipLevels = 1;
            colorDesc.ArraySize = 1;
            colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // Common color format
            colorDesc.SampleDesc.Count = 1;
            colorDesc.Usage = D3D11_USAGE_DEFAULT;
            colorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
            colorDesc.CPUAccessFlags = 0;
            colorDesc.MiscFlags = 0;

            hr = Globals::g_D3D11Device->CreateTexture2D(REX_CAST(&colorDesc, D3D11_TEXTURE2D_DESC), nullptr,
                                                         REX_CAST(&colorBufferShared, ID3D11Texture2D*));
            if (FAILED(hr)) {
                logger::error("Failed to create color buffer. HRESULT: {0}", hr);
            } else {
                logger::info("Color buffer created: {}x{}", colorDesc.Width, colorDesc.Height);
            }
        }

        // --- Allocate Depth Buffer ---
        {
            D3D11_TEXTURE2D_DESC depthDesc = {};
            depthDesc.Width = Globals::OutputResolutionWidth;
            depthDesc.Height = Globals::OutputResolutionHeight;
            depthDesc.MipLevels = 1;
            depthDesc.ArraySize = 1;
            depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;  // Typeless for flexibility with depth/stencil views
            depthDesc.SampleDesc.Count = 1;
            depthDesc.Usage = D3D11_USAGE_DEFAULT;
            depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            depthDesc.CPUAccessFlags = 0;
            depthDesc.MiscFlags = 0;

            hr = Globals::g_D3D11Device->CreateTexture2D(REX_CAST(&depthDesc, D3D11_TEXTURE2D_DESC), nullptr,
                                                         REX_CAST(&depthBufferShared, ID3D11Texture2D*));
            if (FAILED(hr)) {
                logger::error("Failed to create depth buffer. HRESULT: {0}", hr);
            } else {
                logger::info("Depth buffer created: {}x{}", depthDesc.Width, depthDesc.Height);
            }
        }

        // --- Allocate Motion Vectors Buffer ---
        {
            D3D11_TEXTURE2D_DESC motionDesc = {};
            motionDesc.Width = Globals::OutputResolutionWidth;
            motionDesc.Height = Globals::OutputResolutionHeight;
            motionDesc.MipLevels = 1;
            motionDesc.ArraySize = 1;
            motionDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
            motionDesc.SampleDesc.Count = 1;
            motionDesc.Usage = D3D11_USAGE_DEFAULT;
            motionDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
            motionDesc.CPUAccessFlags = 0;
            motionDesc.MiscFlags = 0;

            hr = Globals::g_D3D11Device->CreateTexture2D(REX_CAST(&motionDesc, D3D11_TEXTURE2D_DESC), nullptr,
                                                         REX_CAST(&motionVectorsShared, ID3D11Texture2D*));
            if (FAILED(hr)) {
                logger::error("Failed to create motion vectors buffer. HRESULT: {0}", hr);
            } else {
                logger::info("Motion vectors buffer created: {}x{}", motionDesc.Width, motionDesc.Height);
            }
        }

        logger::info("All DLSS shared buffers allocated successfully.");
    }

    void Streamline::loadDlSSBuffers() {
        // Setup viewport
        sl::ViewportHandle view{viewport};

        // Create shared buffers
        sl::Resource colorIn = {sl::ResourceType::eTex2d, colorBufferShared, 0};

        sl::Resource depthIn = {sl::ResourceType::eTex2d, depthBufferShared, 0};

        sl::Resource motionVectorsIn = {sl::ResourceType::eTex2d, motionVectorsShared, 0};

        sl::Resource colorOut = {sl::ResourceType::eTex2d, colorBufferShared, 0};

        sl::Extent fullExtent{0, 0, (UINT)Globals::RenderResolutionWidth, (UINT)Globals::RenderResolutionHeight};
 
        // Tagging resources
        sl::ResourceTag colorInTag = sl::ResourceTag {&colorIn, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eOnlyValidNow, &fullExtent};
        sl::ResourceTag depthInTag = sl::ResourceTag{&depthIn, sl::kBufferTypeDepth, sl::ResourceLifecycle::eOnlyValidNow, &fullExtent};
        sl::ResourceTag motionVectorsInTag = sl::ResourceTag{&motionVectorsIn, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eOnlyValidNow, &fullExtent};
        sl::ResourceTag outputBufferTag = sl::ResourceTag{&colorOut, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eOnlyValidNow, &fullExtent};

        sl::ResourceTag inputs[] = {colorInTag, outputBufferTag, depthInTag, motionVectorsInTag};

        if (!colorBufferShared) {
            logger::error("[Streamline] colorBufferShared is null before tagging!");
        }
        if (!depthBufferShared) {
            logger::error("[Streamline] depthBufferShared is null before tagging!");
        }
        if (!motionVectorsShared) {
            logger::error("[Streamline] motionVectorsShared is null before tagging!");
        }

        if (SL_FAILED(res, slSetTag(view, inputs, _countof(inputs), Globals::context))) {
            logger::warn("[Streamline] Failed to set tags: {}", static_cast<int>(res));
        }
    }

    void Streamline::updateConstants() {
    
        logger::info("Updating DLSS constants...");

        if (!Globals::DLSS_Available) {
            logger::warn("DLSS is not available. Skipping update.");
            return;
        }


        auto renderer = Globals::renderer;
        if (!renderer) {
            logger::warn("Renderer is null, cannot update constants.");
            return;
        }

        // Set up matrices (dummy identity matrices for now)
        constants.cameraViewToClip = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
        constants.clipToCameraView = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
        constants.clipToLensClip = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
        constants.clipToPrevClip = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
        constants.prevClipToClip = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};

        // Other DLSS constants
        constants.jitterOffset = {0.0f, 0.0f};
        constants.mvecScale = {1.0f, 1.0f};
        constants.cameraPinholeOffset = {1.f, 0.f};
        constants.cameraPos = {1.0f, 1.0f, 1.0f};
        constants.cameraUp = {0.0f, 1.0f, 0.0f};
        constants.cameraRight = {1.0f, 0.0f, 0.0f};
        constants.cameraFwd = {0.0f, 0.0f, -1.0f};

        // Retrieve near/far values
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

        logger::info("DLSS constants updated.");
    }

    void Streamline::HandlePresent() {
        logger::info("Handling frame present...");

        if (!Settings::Plugin_Enabled) {
            logger::info("Plugin is disabled. Skipping frame processing.");
            return;
        }


        if (!Globals::DLSS_Available) {
            logger::warn("DLSS is not available. Skipping frame processing.");
            return;
        }

        if (!Globals::context) {
            logger::warn("D3D11 context is null. Cannot process frame.");
            return;
        }

        auto renderer = Globals::renderer;
        if (!renderer) {
            logger::warn("Renderer is null. Cannot process frame.");
            return;
        }

        // Retrieve game buffers
        auto& swapChain = renderer->GetRendererData()->renderTargets[RE::RENDER_TARGET::kFRAMEBUFFER];
        auto& motionVectors = renderer->GetRendererData()->renderTargets[RE::RENDER_TARGETS::RENDER_TARGET::kMOTION_VECTOR];
        RE::BSGraphics::DepthStencilData depth{};
        //= renderer->GetRendererData()->depthStencils[RE::RENDER_TARGETS_DEPTHSTENCIL::kPOST_ZPREPASS_COPY];

        ID3D11ShaderResourceView* dummyDepthSRV = nullptr;
        // Create dummy depth texture if it doesn't exist
        if (!depth.depthSRV) {
            logger::warn("Depth buffer SRV is missing, generating dummy.");

            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = Globals::OutputResolutionWidth;
            desc.Height = Globals::OutputResolutionHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

            // Create the depth texture
            ID3D11Resource* depthTexture = nullptr;
            Globals::g_D3D11Device->CreateTexture2D(REX_CAST(&desc, D3D11_TEXTURE2D_DESC), nullptr, REX_CAST(&depthTexture, ID3D11Texture2D*));

            // Create the depth srv
            D3D11_SHADER_RESOURCE_VIEW_DESC dsvDesc{};
            dsvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            dsvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipLevels = 1;

            // create the srv
            HRESULT hr = Globals::g_D3D11Device->CreateShaderResourceView(REX_CAST(depthTexture, ID3D11Resource),
                                                             REX_CAST(&dsvDesc, D3D11_SHADER_RESOURCE_VIEW_DESC),
                                                             REX_CAST(&dummyDepthSRV, ID3D11ShaderResourceView*));

            if (FAILED(hr)) {
                logger::warn("Failed to create dummy depth SRV.");
                // Log error
                logger::warn("Error code: {}", hr);
                depthTexture->Release();
                return;
            }

            depth.depthSRV = dummyDepthSRV;
            depthTexture->Release();


        }

         // Ensure all resources are available before continuing
        if (!swapChain.SRV || !motionVectors.SRV || !depth.depthSRV) {
            logger::warn("Required buffers (swap chain, motion vectors, depth) are missing.");
            return;
        }

        if (!swapChain.SRV) {
            logger::warn("Swap chain SRV is missing.");
            return;
        }

        ID3D11Resource* swapChainResource = nullptr;
        swapChain.SRV->GetResource(&swapChainResource);
        if (!swapChainResource) {
            logger::warn("Failed to get swap chain resource.");
            return;
        }

        // Copy buffers
        Globals::context->CopyResource(REX_CAST(colorBufferShared, ID3D11Texture2D),
                              REX_CAST(swapChainResource, ID3D11Texture2D));


        if (motionVectors.SRV) {
            ID3D11Resource* motionVectorsResource = nullptr;
            motionVectors.SRV->GetResource(&motionVectorsResource);
            Globals::context->CopyResource(REX_CAST(motionVectorsShared, ID3D11Texture2D),
                                  REX_CAST(motionVectorsResource, ID3D11Texture2D));

            // Release motion vectors resource
            motionVectorsResource->Release();
        } else {
            logger::warn("Motion vectors SRV is missing.");
        }

        if (depth.depthSRV) {
            ID3D11Resource* depthResource = nullptr;
            dummyDepthSRV->GetResource(&depthResource);
            Globals::context->CopyResource(REX_CAST(depthBufferShared, ID3D11Texture2D),
                                  REX_CAST(depthResource, ID3D11Texture2D));

            // Release depth buffer resource
            depthResource->Release();
        } else {
            logger::warn("Depth buffer SRV is missing.");
            if (!dummyDepthSRV) {
                logger::warn("Dummy depth SRV is missing.");
            } else {
                ID3D11Resource* depthResource = nullptr;
                depth.depthSRV->GetResource(&depthResource);
                Globals::context->CopyResource(REX_CAST(depthBufferShared, ID3D11Texture2D),
                                               REX_CAST(depthResource, ID3D11Texture2D));

                // Release depth buffer resource
                depthResource->Release();
            }
        }


        // Load shared buffers
        loadDlSSBuffers();

        // Return if all resources are not available
        if (!colorBufferShared && !depthBufferShared && !motionVectorsShared) {
            logger::warn("All buffers are missing.");
            swapChainResource->Release();
            return;
        }



        sl::FrameToken* frameToken = nullptr;
        if (SL_FAILED(res, slGetNewFrameToken(frameToken, nullptr))) {
            logger::warn("Failed to get new frame token.");
            return;
        }

        if (SL_FAILED(res, slSetConstants(constants, *frameToken, viewport))) {
            logger::warn("Failed to set DLSS constants.");
            return;
        }

        const sl::BaseStructure* inputs[] = {&viewport};
       if (SL_FAILED(res, slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, _countof(inputs), Globals::context))) {
            logger::warn("Failed to evaluate DLSS feature: {}", static_cast<int>(res));
            swapChainResource->Release();
            return;
        } else {
            logger::info("DLSS feature evaluated successfully.");
        }

        // Release depth buffer resource
        if (dummyDepthSRV) {
            dummyDepthSRV->Release();
        }
        
        // Free all resources
        if (swapChainResource) {
            swapChainResource->Release();
        }

        if (motionVectorsShared) {
            motionVectorsShared->Release();
        }

        if (depthBufferShared) {
            depthBufferShared->Release();
        }

        if (colorBufferShared) {
            colorBufferShared->Release();
        }
        logger::info("Frame present handled successfully.");
    }

    void Streamline::updateBuffers() { 
        auto renderer = Globals::renderer;
        auto context = UNREX_CAST(Globals::context, ID3D11DeviceContext);

        
       
    }


    
}