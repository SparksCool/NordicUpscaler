// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#include <Utils.h>
#include <sl.h>
#include <sl_result.h>
#include <sl_core_api.h>
#include <sl_consts.h>
#include <sl_dlss.h>
#include <Globals.h>

namespace Utils {
    void** get_vtable_ptr(void* obj) { return *reinterpret_cast<void***>(obj); }

    void loop_vtable(void** vtable, int size) {
        for (int i = 0; i < size; i++) {
            if (!vtable[i]) {
                continue;
            }
            void* curVtable = vtable[i];
            logger::info("VTable index {}: {}", i, vtable[i]);
        }
    }

}