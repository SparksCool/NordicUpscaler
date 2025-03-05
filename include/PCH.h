// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <wrl/client.h>

namespace logger = SKSE::log;
using namespace std::literals;

namespace stl {
    using namespace SKSE::stl;

    template <class T, std::size_t Size = 5>
    void write_thunk_call(std::uintptr_t a_src) {
        SKSE::AllocTrampoline(14);
        auto& trampoline = SKSE::GetTrampoline();
        if (Size == 6) {
            T::func = *(uintptr_t*)trampoline.write_call<6>(a_src, T::thunk);
        } else {
            T::func = trampoline.write_call<Size>(a_src, T::thunk);
        }
    }
}