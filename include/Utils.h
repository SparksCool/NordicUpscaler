#pragma once

namespace Utils {
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
    };
}