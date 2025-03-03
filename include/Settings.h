// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once

#include "SimpleIni.h"
#include <RE/R/Renderer.h>

namespace Settings {
    

    inline const char* settingsPath{"Data/SKSE/Plugins/NordicUpscaler.ini"};

    void LoadSettings();
    void SaveSettings();
    void ResetSettings();

    inline bool Plugin_Enabled = true;
    inline bool Viewport_Enabled = true;
    inline int Selected_Preset_DLSS = 2;
    inline int MaxFrameViewPortUpdates = 23;
    inline const char* DLSS_Presets[] = {"Off", "Performance", "Balanced", "Quality", "Ultra Performance", "Ultra Quality"};

}