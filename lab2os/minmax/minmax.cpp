#include "pch.h"          
#include "minmax.h"
#include <windows.h>

extern "C" {
    __declspec(dllexport) void find_min_max(const int* arr, int size, int* outMin, int* outMax) {
        int loc_min = arr[0];
        int loc_max = arr[0];
        for (int i = 1; i < size; ++i) {
            if (arr[i] < loc_min) loc_min = arr[i];
            Sleep(7);                       
            if (arr[i] > loc_max) loc_max = arr[i];
            Sleep(7);
        }
        *outMin = loc_min;
        *outMax = loc_max;
    }
}