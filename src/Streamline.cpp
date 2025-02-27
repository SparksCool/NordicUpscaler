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

namespace Streamline {
    void Streamline::loadInterposer() {
        // TODO(SparksCool): Ensure this is compatible with community shaders sl.interposer.dll
        interposer = LoadLibraryW(L"Data/SKSE/Plugins/Streamline/sl.interposer.dll");
    }

    bool Streamline::initSL() {

        sl::Preferences pref{};

        static const sl::Feature features[] = {sl::kFeatureDLSS};
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
        pref.logMessageCallback = [](sl::LogType type, const char* msg) { logger::info("[Streamline] {}", msg); };
#endif

        sl::Result result = slInit(pref);

        // This helps to debug, cross reference results with the result enum contained in sl_result.h
        // more descriptive messages might be added in the future
        logger::info("[Streamline] Streamline start attempted with result: {}", static_cast<int>(result));

        // Set D3D device to the one actually being used by the game renderer
        sl::Result dev_result = slSetD3DDevice(Globals::g_D3D11Device);

        logger::info("[Streamline] Set D3D11 device with result: {}", static_cast<int>(dev_result));

        return result == sl::Result::eOk;
    }

    /* Primarily referenced from https://github.com/NVIDIAGameWorks/Streamline/blob/main/docs/ProgrammingGuideDLSS.md
     * part 2.0
     * This might need to be reworked as this cycles through ALL adapters while we should really only have it use Globals::g_D3D11Device
     * Otherwise we could face issues where theres two adapters e.g an iGPU and a dGPU, SLI too but its kind of extinct in this day */
    bool Streamline::DLSSAvailable() {
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

    void Streamline::loadDlSSBuffers() {
        // Setup viewport
        sl::ViewportHandle view{Globals::viewport};

        // Create shared buffers
        sl::Resource colorIn = {sl::ResourceType::eTex2d, Globals::colorBufferShared, 0};

        sl::Resource depthIn = {sl::ResourceType::eTex2d, Globals::depthBufferShared, 0};

        sl::Resource motionVectorsIn = {sl::ResourceType::eTex2d, Globals::motionVectorsShared, 0};

        sl::Resource outputBuffer = {sl::ResourceType::eTex2d, Globals::dlssOutputBuffer, 0};

        sl::Extent fullExtent{0, 0, (UINT)Globals::renderer->GetScreenSize().width,
                              (UINT)Globals::renderer->GetScreenSize().height};
 
        // Tagging resources
        sl::ResourceTag colorInTag = sl::ResourceTag {&colorIn, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eOnlyValidNow, &fullExtent};
        sl::ResourceTag depthInTag = sl::ResourceTag{&depthIn, sl::kBufferTypeDepth, sl::ResourceLifecycle::eOnlyValidNow, &fullExtent};
        sl::ResourceTag motionVectorsInTag = sl::ResourceTag{&motionVectorsIn, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eOnlyValidNow, &fullExtent};
        sl::ResourceTag outputBufferTag = sl::ResourceTag{&outputBuffer, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eOnlyValidNow, &fullExtent};

        sl::ResourceTag inputs[] = {colorInTag, outputBufferTag, depthInTag, motionVectorsInTag};

        if (SL_FAILED(res, slSetTag(view, inputs, _countof(inputs), Globals::context))) {
            logger::warn("[Streamline] Failed to set tags: {}", static_cast<int>(res));
        }
    }


    
}