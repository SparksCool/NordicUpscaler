#include "MCP.h"

#include "SKSEMCP/SKSEMenuFramework.hpp"
#include <Settings.h>

namespace MCP {
    void Register() { 
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::warn("SKSE Menu Framework is not installed.");
            return;
        }

        SKSEMenuFramework::SetSection("Nordic Upscaler");
        SKSEMenuFramework::AddSectionItem("Nordic Upscaler Settings", RenderSettings);

        logger::info("SKSE Menu Framework registered.");
    
    }

    void _stdcall RenderSettings() { 
        if (ImGui::Button("Save Settings")) {
            Settings::SaveSettings();
        }

        if (ImGui::Button("Reset Settings")) {
            Settings::ResetSettings();
        }

        ImGui::Text("Nordic Upscaler Settings");

        ImGui::Checkbox("Plugin Enabled", &Settings::Plugin_Enabled);

    }
}