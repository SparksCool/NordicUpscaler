#include <Globals.h>

namespace Globals {
    RE::BSGraphics::Renderer* renderer = nullptr;

    void init() {
        renderer = RE::BSGraphics::Renderer::GetSingleton(); 
    }
}