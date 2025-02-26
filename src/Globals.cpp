#include <Globals.h>
#include <Utils.h>
#include <Streamline.h>

namespace Globals {
    RE::BSGraphics::Renderer* renderer = nullptr;
    REX::W32::ID3D11Device* g_D3D11Device = nullptr;

    void init() {
        renderer = RE::BSGraphics::Renderer::GetSingleton(); 
        g_D3D11Device = renderer->GetDevice();  // This is used for determining what adapter to use for DLSS
        

        /* Initialize Streamline then check if DLSS is available */
        Streamline::Streamline::getSingleton()->loadInterposer();
        Streamline_Init = Streamline::Streamline::getSingleton()->initSL();
        DLSS_Available = Streamline::Streamline::getSingleton()->DLSSAvailable();
    }
}