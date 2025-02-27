// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once
#include <sl_core_api.h>

namespace Streamline {
    // Streamline class
    class Streamline {
    private:
        HMODULE interposer = NULL;
        sl::Constants constants;
        sl::ViewportHandle viewport{0};
        bool dlssInitialized = false;
        ID3D11Texture2D* colorBufferShared = nullptr;
        ID3D11Texture2D* motionVectorsShared = nullptr;
        ID3D11Texture2D* depthBufferShared = nullptr;

    public:
        static Streamline* getSingleton() {
            static Streamline singleton;
            return &singleton;
        }

        void loadInterposer();

        bool initSL();
        bool DLSSAvailable();

        void getDLSSRenderResolution();
        void loadDlSSBuffers();
        void updateConstants();
        void HandlePresent();
        void allocateBuffers();
    };
}