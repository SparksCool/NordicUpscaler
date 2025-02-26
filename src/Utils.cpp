#include <Utils.h>
#include <sl.h>
#include <sl_result.h>
#include <sl_core_api.h>
#include <sl_consts.h>
#include <sl_dlss.h>

namespace Utils {
    void Streamline::loadInterposer() {
        interposer = LoadLibraryW(L"Data/SKSE/Plugins/Streamline/sl.interposer.dll");
    }

    bool Streamline::initSL() {
        sl::Preferences pref{};
        sl::Result result = slInit(pref);

        logger::info("[Streamline] Streamline start attempted with result: {}", static_cast<int>(result));

        return result == sl::Result::eOk;
    }
}