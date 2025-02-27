// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once

namespace Streamline {
    // Streamline class
    class Streamline {
    private:
        HMODULE interposer = NULL;

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
    };
}