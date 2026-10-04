#pragma once

extern "C" {
    __declspec(dllexport) void find_min_max(const int* arr, int size, int* outMin, int* outMax);
}
