#include "logger.h"
#include "MCP.h"
#include <Settings.h>
#include <Utils.h>
#include <Globals.h>

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        // Start
        Settings::LoadSettings();

        // TODO(SparksCool): Create a streamline file for this
        // Init Streamline
        Utils::Streamline::getSingleton()->loadInterposer();
        Settings::Streamline_Init = Utils::Streamline::getSingleton()->initSL();

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

    Globals::init();

    return true;
}
