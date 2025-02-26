#include <Globals.h>
#include <Utils.h>

namespace Globals {
    RE::BSGraphics::Renderer* renderer = nullptr;

    void init() {
        renderer = RE::BSGraphics::Renderer::GetSingleton(); 

        /* Initialize Streamline then check if DLSS is available */
        Utils::Streamline::getSingleton()->loadInterposer();
        Streamline_Init = Utils::Streamline::getSingleton()->initSL();
        DLSS_Available = Utils::Streamline::getSingleton()->DLSSAvailable();
    }
}