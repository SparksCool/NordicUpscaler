// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once
#include <REX/W32/DXGI.h>

namespace Hooks {
    void Install();

    struct HkDX11PresentSwapChain {
        static void InstallHook();
        static HRESULT WINAPI thunk(REX::W32::IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        static inline decltype(&thunk) func;
    };

    struct HkDRS {
        static void thunk(RE::BSGraphics::State* a_state);
        static inline decltype(&thunk) func;
        static void InstallHook();
    };

}