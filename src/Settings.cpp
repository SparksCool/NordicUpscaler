// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include "Settings.h"

namespace Settings {
    /* Load our INI settings, or make a new file if one does not exist*/
    void LoadSettings() {
        CSimpleIniW ini;
        ini.SetUnicode();

        if (!std::filesystem::exists(settingsPath)) {
            SaveSettings();
            logger::info("Settings file not found, creating a default one.");
        } else {
            ini.LoadFile(settingsPath);

            Plugin_Enabled = ini.GetBoolValue(L"General", L"Plugin_Enabled");
            Selected_Preset_DLSS = ini.GetLongValue(L"General", L"Selected_Preset_DLSS");
            Enb_Enabled = ini.GetBoolValue(L"General", L"Enb_Enabled");
        }
    }

    /* Save our INI settings */
    void SaveSettings() {
        CSimpleIniW ini;
        ini.SetUnicode();

        ini.SetBoolValue(L"General", L"Plugin_Enabled", Plugin_Enabled);
        ini.SetBoolValue(L"General", L"Enb_Enabled", Enb_Enabled);
        ini.SetLongValue(L"General", L"Selected_Preset_DLSS", Selected_Preset_DLSS);

        ini.SaveFile(settingsPath);

        logger::info("Settings saved.");
    }

    /* Reset our INI settings */
    void ResetSettings() {
        Plugin_Enabled = true;
        Selected_Preset_DLSS = 0;
        Enb_Enabled = false;


        logger::info("Settings reset.");
    }

    

}