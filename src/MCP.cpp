// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include "MCP.h"

#include "SKSEMCP/SKSEMenuFramework.hpp"
#include <Settings.h>
#include <Globals.h>
#include <SKSEMCP/SKSEMenuFramework.hpp>

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

        ImGui::Text("Nordic Upscaler Settings");

        ImGui::Checkbox("Plugin Enabled", &Settings::Plugin_Enabled);

        ImGui::Combo("DLSS Preset", &Settings::Selected_Preset_DLSS, Settings::DLSS_Presets, IM_ARRAYSIZE(Settings::DLSS_Presets));

    }

    void _stdcall RenderDebugInfo() {
        ImGui::Text("Nordic Upscaler Debug Info");
        ImGui::Text("Current Resolution: %dx%d", Globals::renderer->GetScreenSize().width, Globals::renderer->GetScreenSize().height);
        ImGui::Text("Render Resolution: %dx%d", Globals::RenderResolutionWidth, Globals::RenderResolutionHeight);
        ImGui::Text("Output Resolution: %dx%d", Globals::OutputResolutionWidth, Globals::OutputResolutionHeight);

        // Change MaxFrameViewPortUpdates to a setting
        ImGui::Text("Max Frame View Port Updates: %d", Settings::MaxFrameViewPortUpdates);

        // Buttons to increase and decrease MaxFrameViewPortUpdates
        if (ImGui::Button("Increase Max Frame View Port Updates")) {
            Settings::MaxFrameViewPortUpdates++;
        }

        if (ImGui::Button("Decrease Max Frame View Port Updates")) {
            Settings::MaxFrameViewPortUpdates--;
        }

        ImGui::Checkbox("Viewport Changing Enabled", &Settings::Viewport_Enabled);

        ImGui::Checkbox("Debug Frames Enabled", &Settings::Debug_Enabled);


        if (Globals::Streamline_Init) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),"Streamline Initialized: True");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),"Streamline Initialized: False");
        }

        if (Globals::DLSS_Available) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "DLSS Capable: True");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "DLSS Capable: False");
        }

        if (Globals::SwapChain_Hooked) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "DirectX Swap Chain Hooked: True");
            ImGui::Text("Swap Chain Memory Address: %p", Globals::swapChain);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "DirectX Swap Chain Hooked: False");
        }

        ImGui::Text("Scale Factor: %f", Globals::getScaleFactor());
        ImGui::Text("DRS Active: %s", Globals::DRS_Active ? "True" : "False");

    }
}