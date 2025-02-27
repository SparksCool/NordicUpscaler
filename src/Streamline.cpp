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

namespace Streamline {
    void Streamline::loadInterposer() {
        // TODO(SparksCool): Ensure this is compatible with community shaders sl.interposer.dll
        interposer = LoadLibraryW(L"Data/SKSE/Plugins/Streamline/sl.interposer.dll");
    }

    bool Streamline::initSL() {
        // Set D3D11 device to the one actually being used by the game renderer 
        slSetD3DDevice(Globals::g_D3D11Device);

        sl::Preferences pref{};

        static const sl::Feature features[] = {sl::kFeatureDLSS};
        pref.featuresToLoad = features;
        pref.numFeaturesToLoad = sizeof(features) / sizeof(sl::Feature);

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
}