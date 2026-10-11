#pragma once
#include <cstddef>
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_SPIRAM 2
inline size_t heap_caps_get_free_size(int cap){return cap==MALLOC_CAP_SPIRAM ? 8380000 : 250000;}
inline size_t heap_caps_get_minimum_free_size(int){return 240000;}
inline size_t heap_caps_get_largest_free_block(int){return 180000;}
