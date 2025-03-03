// Copyright (c) 2025 SparksCool
// Licensed under the MIT license.

#pragma once

namespace Utils {
    void** get_vtable_ptr(void* obj);
    void loop_vtable(void** vtable, int size);
}