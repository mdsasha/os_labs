#include <windows.h>
#include <iostream>
#include <vector>
#include <clocale>

using namespace std;

volatile int* arr = nullptr;
int arrSize = 0;
int markersCount = 0;

CRITICAL_SECTION csArray;
CRITICAL_SECTION csConsole;

HANDLE* hStartEvents = nullptr;
HANDLE* hCantContinueEvents = nullptr;
HANDLE* hContinueEvents = nullptr;
HANDLE* hStopEvents = nullptr;
HANDLE* hThreads = nullptr;
DWORD* dwThreadIds = nullptr;

struct MarkerParams {
    int number;
};

DWORD WINAPI MarkerThread(LPVOID lpParam)
{
    MarkerParams* p = (MarkerParams*)lpParam;
    int num = p->number;

    WaitForSingleObject(hStartEvents[num - 1], INFINITE);

    srand(num);

    int markedCount = 0;

    while (true)
    {
        int r = rand();
        int idx = r % arrSize;

        EnterCriticalSection(&csArray);
        bool canMark = (arr[idx] == 0);
        LeaveCriticalSection(&csArray);

        if (canMark)
        {
            Sleep(5);

            EnterCriticalSection(&csArray);
            if (arr[idx] == 0) {
                arr[idx] = num;
                markedCount++;
            }
            LeaveCriticalSection(&csArray);

            Sleep(5);

            continue;
        }
        else
        {
            EnterCriticalSection(&csConsole);
            cout << "\n[Marker #" << num << "]";
            cout << "  Количество помеченных элементов: " << markedCount;
            cout << "  Невозможно пометить элемент с индексом: " << idx << endl;
            LeaveCriticalSection(&csConsole);

            SetEvent(hCantContinueEvents[num - 1]);

            HANDLE waitArr[2] = { hContinueEvents[num - 1], hStopEvents[num - 1] };
            DWORD res = WaitForMultipleObjects(2, waitArr, FALSE, INFINITE);

            if (res == WAIT_OBJECT_0 + 1)
            {
                EnterCriticalSection(&csArray);
                for (int i = 0; i < arrSize; i++) {
                    if (arr[i] == num) arr[i] = 0;
                }
                LeaveCriticalSection(&csArray);

                return 0;
            }
        }
    }
    return 0;
}

void PrintArray(const char* title)
{
    EnterCriticalSection(&csConsole);
    cout << title << ": ";
    for (int i = 0; i < arrSize; i++)
        cout << arr[i] << " ";
    cout << endl;
    LeaveCriticalSection(&csConsole);
}

int ReadInt(const char* prompt)
{
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) return value;
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Ошибка ввода. Повторите." << endl;
    }
}

int main()
{
    setlocale(LC_ALL, "Russian");

    arrSize = ReadInt("Введите размерность массива: ");
    if (arrSize <= 0) { cout << "Некорректный размер." << endl; return 1; }

    arr = new int[arrSize];

    for (int i = 0; i < arrSize; i++) arr[i] = 0;

    markersCount = ReadInt("Введите количество потоков marker: ");
    if (markersCount <= 0) { cout << "Некорректное количество." << endl; return 1; }

    InitializeCriticalSection(&csArray);
    InitializeCriticalSection(&csConsole);

    hStartEvents = new HANDLE[markersCount];
    hCantContinueEvents = new HANDLE[markersCount];
    hContinueEvents = new HANDLE[markersCount];
    hStopEvents = new HANDLE[markersCount];
    hThreads = new HANDLE[markersCount];
    dwThreadIds = new DWORD[markersCount];

    MarkerParams* params = new MarkerParams[markersCount];

    for (int i = 0; i < markersCount; i++)
    {
        hStartEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        hCantContinueEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        hContinueEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        hStopEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (!hStartEvents[i] || !hCantContinueEvents[i] ||
            !hContinueEvents[i] || !hStopEvents[i])
        {
            cout << "Ошибка создания события." << endl;
            return GetLastError();
        }

        params[i].number = i + 1;

        hThreads[i] = CreateThread(NULL, 0, MarkerThread, &params[i],
            CREATE_SUSPENDED, &dwThreadIds[i]);
        if (hThreads[i] == NULL)
        {
            cout << "Ошибка создания потока." << endl;
            return GetLastError();
        }
    }

    for (int i = 0; i < markersCount; i++)
        ResumeThread(hThreads[i]);

    for (int i = 0; i < markersCount; i++)
        SetEvent(hStartEvents[i]);

    int activeCount = markersCount;
    while (activeCount > 0)
    {
        vector<HANDLE> waitEvents;
        for (int i = 0; i < markersCount; i++)
        {
            DWORD code = 0;
            GetExitCodeThread(hThreads[i], &code);
            if (code == STILL_ACTIVE)
                waitEvents.push_back(hCantContinueEvents[i]);
        }

        if (waitEvents.empty()) break;

        WaitForMultipleObjects((DWORD)waitEvents.size(),
            waitEvents.data(), TRUE, INFINITE);

        PrintArray("Текущее состояние массива");

        int stopNum = -1;
        while (true)
        {
            stopNum = ReadInt("Введите номер потока marker для завершения (0 — выход из программы): ");
            if (stopNum == 0) break;
            if (stopNum >= 1 && stopNum <= markersCount)
            {
                DWORD code = 0;
                GetExitCodeThread(hThreads[stopNum - 1], &code);
                if (code == STILL_ACTIVE) break;
                cout << "Этот поток уже завершён. Выберите другой." << endl;
            }
            else
            {
                cout << "Некорректный номер. Диапазон: 1.." << markersCount << endl;
            }
        }

        if (stopNum == 0)
        {
            for (int i = 0; i < markersCount; i++)
            {
                DWORD code = 0;
                GetExitCodeThread(hThreads[i], &code);
                if (code == STILL_ACTIVE)
                    SetEvent(hStopEvents[i]);
            }
            break;
        }

        SetEvent(hStopEvents[stopNum - 1]);

        WaitForSingleObject(hThreads[stopNum - 1], INFINITE);

        PrintArray("Состояние массива после завершения потока");

        for (int i = 0; i < markersCount; i++)
        {
            if (i == stopNum - 1) continue;
            DWORD code = 0;
            GetExitCodeThread(hThreads[i], &code);
            if (code == STILL_ACTIVE)
                SetEvent(hContinueEvents[i]);
        }

        activeCount = 0;
        for (int i = 0; i < markersCount; i++)
        {
            DWORD code = 0;
            GetExitCodeThread(hThreads[i], &code);
            if (code == STILL_ACTIVE) activeCount++;
        }
    }

    for (int i = 0; i < markersCount; i++)
    {
        DWORD code = 0;
        GetExitCodeThread(hThreads[i], &code);
        if (code == STILL_ACTIVE)
        {
            SetEvent(hStopEvents[i]);
            WaitForSingleObject(hThreads[i], INFINITE);
        }
        CloseHandle(hThreads[i]);
        CloseHandle(hStartEvents[i]);
        CloseHandle(hCantContinueEvents[i]);
        CloseHandle(hContinueEvents[i]);
        CloseHandle(hStopEvents[i]);
    }

    DeleteCriticalSection(&csArray);
    DeleteCriticalSection(&csConsole);

    delete[] arr;
    delete[] hStartEvents;
    delete[] hCantContinueEvents;
    delete[] hContinueEvents;
    delete[] hStopEvents;
    delete[] hThreads;
    delete[] dwThreadIds;
    delete[] params;

    cout << "Работа завершена." << endl;
    return 0;
}