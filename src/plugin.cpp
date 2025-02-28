// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include "logger.h"
#include "MCP.h"
#include <Settings.h>
#include <Utils.h>
#include <Globals.h>
#include <Hooks.h>

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        // Start
        Globals::fullInit();
        Hooks::Install();
        MCP::Register();
    }
    if (message->type == SKSE::MessagingInterface::kNewGame ||
        message->type == SKSE::MessagingInterface::kPostLoadGame ||
        message->type == SKSE::MessagingInterface::kPostLoad) {
        // Post-load
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    logger::info("Game version: {}", skse->RuntimeVersion().string());
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    Settings::LoadSettings();
    Globals::earlyInit();
    return true;
}
