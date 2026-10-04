#include <windows.h>
#include <iostream>
#include <vector>
#include <climits>

typedef void (*FindFunc)(const int*, int, int*, int*);

std::vector<int> v;
int min = INT_MAX;
int max = INT_MIN;
double avrg = 0.0;

DWORD WINAPI min_max(LPVOID) {
    HMODULE hLib = LoadLibraryA("minmax.dll");
    FindFunc func = (FindFunc)GetProcAddress(hLib, "find_min_max");
    int locMin = 0, locMax = 0;
    func(v.data(), (int)v.size(), &locMin, &locMax);
    FreeLibrary(hLib);
    min = locMin;
    max = locMax;
    std::cout << "min = " << min << ' ' << "max = " << max << '\n';
    return 0;
}

DWORD WINAPI average(LPVOID) {
    long long sum = 0;
    int n = v.size();
    for (int i = 0; i < n; ++i) {
        sum += v[i];
        Sleep(12);
    }
    avrg = static_cast<double>(sum) / n;
    std::cout << "average value: " << avrg << '\n';
    return 0;
}
int main() {
    int size;
    std::cout << "enter size: ";
    std::cin >> size;
    v.resize(size);
    std::cout << "enter elements:" << '\n';
    for (int i = 0; i < size; ++i) {
        std::cin >> v[i];
    }
    HANDLE hMinMax = CreateThread(NULL, 0, min_max, NULL, 0, NULL);
    HANDLE hAverage = CreateThread(NULL, 0, average, NULL, 0, NULL);
    if (hMinMax == NULL || hAverage == NULL) {
        std::cerr << "thread aren't created" << '\n';
        return 1;
    }
    WaitForSingleObject(hMinMax, INFINITE);
    WaitForSingleObject(hAverage, INFINITE);
    for (int i = 0; i < size; ++i) {
        if (v[i] == min || v[i] == max) {
            v[i] = static_cast<int>(avrg);
        }
    }
    std::cout << "result: ";
    for (int i = 0; i < size; ++i) {
        std::cout << v[i] << ' ';
    }
    std::cout << '\n';
    CloseHandle(hMinMax);
    CloseHandle(hAverage);
    return 0;
}