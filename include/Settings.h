#pragma once

#include "SimpleIni.h"
#include <RE/R/Renderer.h>

namespace Settings {

    inline const char* settingsPath{"Data/SKSE/Plugins/NordicUpscaler.ini"};

    void LoadSettings();
    void SaveSettings();
    void ResetSettings();

    inline bool Plugin_Enabled = true;
    inline bool Streamline_Init = false;
}