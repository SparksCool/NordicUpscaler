#pragma once

#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*_ARR))) // This is here because it is not defined in SKSE menu framework be default (afaik)

namespace MCP {
    void Register();
    void _stdcall RenderSettings();
    void _stdcall RenderDebugInfo();
}