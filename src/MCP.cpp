#include "MCP.h"

#include "SKSEMCP/SKSEMenuFramework.hpp"
#include <Settings.h>
#include <Globals.h>

namespace MCP {
    void Register() { 
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::warn("SKSE Menu Framework is not installed.");
            return;
        }

        SKSEMenuFramework::SetSection("Nordic Upscaler");
        SKSEMenuFramework::AddSectionItem("Nordic Upscaler Settings", RenderSettings);
        SKSEMenuFramework::AddSectionItem("Nordic Upscaler Debug Info", RenderDebugInfo);

        logger::info("SKSE Menu Framework registered.");
    
    }

    void _stdcall RenderSettings() { 
        if (ImGui::Button("Save Settings")) {
            Settings::SaveSettings();
        }

        if (ImGui::Button("Reset Settings")) {
            Settings::ResetSettings();
        }

        ImGui::Separator();

        ImGui::Text("Nordic Upscaler Settings");

        ImGui::Checkbox("Plugin Enabled", &Settings::Plugin_Enabled);

    }

    void _stdcall RenderDebugInfo() {
        ImGui::Text("Nordic Upscaler Debug Info");
        ImGui::Text("Current Resolution: %dx%d", Globals::renderer->GetScreenSize().width, Globals::renderer->GetScreenSize().height);

        if (Settings::Plugin_Enabled) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),"Streamline Initialized: True");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),"Streamline Initialized: False");
        }
    }
}