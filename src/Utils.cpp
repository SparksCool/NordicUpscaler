#include <Utils.h>
#include <sl.h>
#include <sl_result.h>
#include <sl_core_api.h>
#include <sl_consts.h>
#include <sl_dlss.h>

namespace Utils {
    void Streamline::loadInterposer() {
        // TODO(SparksCool): Ensure this is compatible with community shaders sl.interposer.dll
        interposer = LoadLibraryW(L"Data/SKSE/Plugins/Streamline/sl.interposer.dll");
    }

    bool Streamline::initSL() {
        sl::Preferences pref{};
        sl::Result result = slInit(pref);

        // This helps to debug, cross reference results with the result enum contained in sl_result.h
        // more descriptive messages might be added in the future
        logger::info("[Streamline] Streamline start attempted with result: {}", static_cast<int>(result));

        return result == sl::Result::eOk;
    }
}