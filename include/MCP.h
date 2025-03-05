// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once

#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*_ARR))) // This is here because it is not defined in SKSE menu framework be default (afaik)


namespace MCP {
    using IsMenuOpen_t = bool(*)(); 

    // Register the Mod Control Panel with the SKSE Menu Framework
    void Register();
    // Render the settings for the Mod Control Panel
    void _stdcall RenderSettings();
    // Render the debug info for the Mod Control Panel
    void _stdcall RenderDebugInfo();
    // Function to check if menus are open
    extern IsMenuOpen_t IsMenuOpen;
}