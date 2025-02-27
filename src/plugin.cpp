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
        Settings::LoadSettings();
        Globals::init();
        Hooks::Install();
        MCP::Register();
    }
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // Post-load
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    logger::info("Game version: {}", skse->RuntimeVersion().string());
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
