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
        }
    }

    /* Save our INI settings */
    void SaveSettings() {
        CSimpleIniW ini;
        ini.SetUnicode();

        ini.SetBoolValue(L"General", L"Plugin_Enabled", Plugin_Enabled);

        ini.SaveFile(settingsPath);

        logger::info("Settings saved.");
    }

    /* Reset our INI settings */
    void ResetSettings() {
        Plugin_Enabled = true;

        logger::info("Settings reset.");
    }

    

}