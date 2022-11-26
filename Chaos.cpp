#include <windows.h>
#include <commctrl.h>

// #define DEBUG
#define MAX_FIGHTERS 6
#define PERCENTS_OFFSET 0x1BF8
#define SHIELDS_OFFSET 0x894

#ifdef DEBUG
#include <iostream>
#endif

#pragma comment(lib, "comctl32.lib")

using namespace std;

typedef struct {
    HWND window;
    HWND percentsInfo;
    HWND shieldsInfo;
    HWND percentsStart;
    HWND shieldsStart;
    HWND percentsSliders[MAX_FIGHTERS];
    HWND shieldsSliders[MAX_FIGHTERS];
    HANDLE yuzu;
    HBITMAP startImage;
    HBITMAP stopImage;
} HANDLES;

typedef struct {
    BOOL searchingPercents;
    BOOL searchingShields;
    BOOL damage;
    BOOL shield;
} STATES;

typedef struct {
    uintptr_t index;
    uintptr_t percentsAddresses[MAX_FIGHTERS];
    uintptr_t shieldsAddresses[MAX_FIGHTERS];
    FLOAT previousPercents[MAX_FIGHTERS];
    FLOAT previousShields[MAX_FIGHTERS];
    INT8 modifiers[MAX_FIGHTERS];
} DATA;

typedef struct {
    HANDLES *handles;
    STATES *states;
    DATA *data;
} CHAOS;

//48 B5 2B 0D
static CONST BYTE percentsPattern[] = {0x48, 0xB5, 0x2B, 0x0D};
//10 22 F8 0C
static CONST BYTE shieldsPattern[] = {0x10, 0x22, 0xF8, 0x0C};

VOID HandleStatus(CHAOS *chaos, LPCSTR info, BOOL &putFalse, BOOL isShields)
{
    SetWindowTextA((isShields ? chaos->handles->shieldsInfo : chaos->handles->percentsInfo), info);
    SendMessageA((isShields ? chaos->handles->shieldsStart : chaos->handles->percentsStart), BM_SETIMAGE, IMAGE_BITMAP, (putFalse ? (LPARAM)chaos->handles->startImage : (LPARAM)chaos->handles->stopImage));
    putFalse = FALSE;
}

;;      ;; ;;;; ;;    ;; ;;;;;;;;   ;;;;;;;  ;;      ;; ;;;;;;;;  ;;;;;;;;   ;;;;;;;   ;;;;;; 
;;  ;;  ;;  ;;  ;;;   ;; ;;     ;; ;;     ;; ;;  ;;  ;; ;;     ;; ;;     ;; ;;     ;; ;;    ;;
;;  ;;  ;;  ;;  ;;;;  ;; ;;     ;; ;;     ;; ;;  ;;  ;; ;;     ;; ;;     ;; ;;     ;; ;;      
;;  ;;  ;;  ;;  ;; ;; ;; ;;     ;; ;;     ;; ;;  ;;  ;; ;;;;;;;;  ;;;;;;;;  ;;     ;; ;;      
;;  ;;  ;;  ;;  ;;  ;;;; ;;     ;; ;;     ;; ;;  ;;  ;; ;;        ;;   ;;   ;;     ;; ;;      
;;  ;;  ;;  ;;  ;;   ;;; ;;     ;; ;;     ;; ;;  ;;  ;; ;;        ;;    ;;  ;;     ;; ;;    ;;
 ;;;  ;;;  ;;;; ;;    ;; ;;;;;;;;   ;;;;;;;   ;;;  ;;;  ;;        ;;     ;;  ;;;;;;;   ;;;;;; 

VOID Destroy(CHAOS *chaos)
{
    free(chaos->handles);
    free(chaos->states);
    free(chaos->data);
    free(chaos);
    PostQuitMessage(0);
}

BOOL CALLBACK EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM lParam)
{
    INT nameLength = GetWindowTextLengthA(hwnd);
    LPSTR name = (LPSTR)malloc(nameLength + 1);

    GetWindowTextA(hwnd, name, nameLength + 1);
    if (strstr(name, "yuzu") != NULL && strstr(name, "Super Smash Bros. Ultimate") != NULL) {
        *(HWND *)(lParam) = hwnd;
        free(name);
        return FALSE;
    }
    free(name);
    return TRUE;
}

VOID Command(CHAOS *chaos, WPARAM wParam)
{
    DWORD pid = 0;
    HWND hYuzu = NULL;

    if ((wParam ? (!chaos->states->shield && (chaos->states->searchingShields = ~chaos->states->searchingShields)) :
        (!chaos->states->damage && (chaos->states->searchingPercents = ~chaos->states->searchingPercents)))) {
        HandleStatus(chaos, "Searching for game address...", (wParam ? chaos->states->shield : chaos->states->damage), wParam);
        EnumWindows(EnumWindowsProc, (LPARAM)(&hYuzu));
        if (!hYuzu)
            return HandleStatus(chaos, "Super Smash Bros. Ultimate couldn't be found",
                (wParam ? chaos->states->searchingShields : chaos->states->searchingPercents), wParam);
        GetWindowThreadProcessId(hYuzu, &pid);
        chaos->handles->yuzu = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        return;
    }
    HandleStatus(chaos, "Click Start to load the hack", (wParam ? chaos->states->shield : chaos->states->damage), wParam);
}

VOID Create(CHAOS *chaos)
{
    HBITMAP backgroundImage = (HBITMAP)LoadImageA(NULL, "ChaosBackground.bmp", IMAGE_BITMAP, 1280, 720, LR_LOADFROMFILE);
    HWND background = CreateWindowA("static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, 0, 0, 1280, 720, chaos->handles->window, NULL, NULL, NULL);

    SendMessage(background, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)backgroundImage);
    InitCommonControls();
    chaos->handles->percentsInfo = CreateWindowA("static", "Click Start to load the hack", WS_VISIBLE | WS_CHILD | SS_CENTER, 0, 290, 342, 20, chaos->handles->window, NULL, NULL, NULL);
    chaos->handles->shieldsInfo = CreateWindowA("static", "Click Start to load the hack", WS_VISIBLE | WS_CHILD | SS_CENTER, 939, 290, 342, 20, chaos->handles->window, NULL, NULL, NULL);
    chaos->handles->percentsStart = CreateWindowA("button", "", WS_VISIBLE | WS_CHILD | BS_BITMAP, 0, 310, 342, 100, chaos->handles->window, (HMENU)0, NULL, NULL);
    chaos->handles->shieldsStart = CreateWindowA("button", "", WS_VISIBLE | WS_CHILD | BS_BITMAP, 939, 310, 342, 100, chaos->handles->window, (HMENU)1, NULL, NULL);
    chaos->handles->startImage = (HBITMAP)LoadImageA(NULL, "ChaosStart.bmp", IMAGE_BITMAP, 342, 100, LR_LOADFROMFILE);
    chaos->handles->stopImage = (HBITMAP)LoadImageA(NULL, "ChaosStop.bmp", IMAGE_BITMAP, 342, 100, LR_LOADFROMFILE);
    SendMessageA(chaos->handles->percentsStart, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)chaos->handles->startImage);
    SendMessageA(chaos->handles->shieldsStart, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)chaos->handles->startImage);
    for (UINT8 i = 0; i < MAX_FIGHTERS; i++) {
        chaos->handles->percentsSliders[i] = CreateWindowA("msctls_trackbar32", "", WS_VISIBLE | WS_CHILD | TBS_NOTICKS | TBS_TOOLTIPS, 0, 410 + 26 + i * 47, 342, 20, chaos->handles->window, (HMENU)i, NULL, NULL);
        SendMessageA(chaos->handles->percentsSliders[i], TBM_SETRANGE, true, MAKELONG(-100, 100));
        SendMessageA(chaos->handles->percentsSliders[i], TBM_SETPAGESIZE, true, 1);
        chaos->handles->shieldsSliders[i] = CreateWindowA("msctls_trackbar32", "", WS_VISIBLE | WS_CHILD | TBS_NOTICKS | TBS_TOOLTIPS, 939, 410 + 26 + i * 47, 342, 20, chaos->handles->window, (HMENU)(i + MAX_FIGHTERS), NULL, NULL);
        SendMessageA(chaos->handles->shieldsSliders[i], TBM_SETRANGE, true, MAKELONG(-100, 100));
        SendMessageA(chaos->handles->shieldsSliders[i], TBM_SETPAGESIZE, true, 1);
    }
}

VOID NcCreate(CHAOS *chaos, HWND hwnd)
{
    chaos = (CHAOS *)calloc(1, sizeof(CHAOS));
    chaos->handles = (HANDLES *)calloc(1, sizeof(HANDLES));
    chaos->states = (STATES *)calloc(1, sizeof(STATES));
    chaos->data = (DATA *)calloc(1, sizeof(DATA));
    chaos->handles->window = hwnd;
    SetWindowLongPtrA(chaos->handles->window, GWLP_USERDATA, (LONG_PTR)chaos);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHAOS *chaos = (CHAOS *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    if (uMsg == WM_NCCREATE)
        NcCreate(chaos, hwnd);
    if (uMsg == WM_CREATE)
        Create(chaos);
    if (uMsg == WM_COMMAND)
        Command(chaos, wParam);
    if (uMsg == WM_HSCROLL && LOWORD(wParam) == TB_ENDTRACK)
        chaos->data->modifiers[(UINT8)GetMenu((HWND)lParam)] = SendMessageA(chaos->handles->percentsSliders[(UINT8)GetMenu((HWND)lParam)], TBM_GETPOS, 0, 0);
    if (uMsg == WM_DESTROY)
        Destroy(chaos);
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

;;     ;;    ;;;    ;;;; ;;    ;; ;;        ;;;;;;;   ;;;;;;;  ;;;;;;;; 
;;;   ;;;   ;; ;;    ;;  ;;;   ;; ;;       ;;     ;; ;;     ;; ;;     ;;
;;;; ;;;;  ;;   ;;   ;;  ;;;;  ;; ;;       ;;     ;; ;;     ;; ;;     ;;
;; ;;; ;; ;;     ;;  ;;  ;; ;; ;; ;;       ;;     ;; ;;     ;; ;;;;;;;; 
;;     ;; ;;;;;;;;;  ;;  ;;  ;;;; ;;       ;;     ;; ;;     ;; ;;       
;;     ;; ;;     ;;  ;;  ;;   ;;; ;;       ;;     ;; ;;     ;; ;;       
;;     ;; ;;     ;; ;;;; ;;    ;; ;;;;;;;;  ;;;;;;;   ;;;;;;;  ;;       

VOID Chaos(CHAOS *chaos)
{
    FLOAT percent = 0, shield = 0;

    for (UINT8 i = 0; i < MAX_FIGHTERS; i++) {
        if (chaos->data->percentsAddresses[i] &&
            !ReadProcessMemory(chaos->handles->yuzu, (LPCVOID)(chaos->data->percentsAddresses[i] + PERCENTS_OFFSET), &percent, sizeof(percent), 0)) {
            chaos->data->index = 0;
            for (UINT8 j = 0; j < MAX_FIGHTERS; j++)
                chaos->data->percentsAddresses[j] = chaos->data->shieldsAddresses[j] = 0;
            if (chaos->states->damage)
                HandleStatus(chaos, "1v1 finished, click Start when new one begins", chaos->states->damage, false);
            if (chaos->states->shield)
                HandleStatus(chaos, "1v1 finished, click Start when new one begins", chaos->states->shield, true);
            return;
        }
        if (chaos->states->damage) {
            if (percent == 0)
                chaos->data->previousPercents[i] = 0;
            if (percent > chaos->data->previousPercents[i] && chaos->data->previousPercents[i] != 0) {
                percent += (percent - chaos->data->previousPercents[i]) * ((FLOAT)chaos->data->modifiers[i] / 100);
                WriteProcessMemory(chaos->handles->yuzu, (LPVOID)(chaos->data->percentsAddresses[i] + PERCENTS_OFFSET), &percent, sizeof(percent), 0);
            }
            chaos->data->previousPercents[i] = percent;
        }
        if (chaos->data->shieldsAddresses[i] &&
            ReadProcessMemory(chaos->handles->yuzu, (LPCVOID)(chaos->data->shieldsAddresses[i] + SHIELDS_OFFSET), &shield, sizeof(shield), 0)) {
            if (chaos->states->damage && chaos->data->previousShields[i] - shield > 1) {
                shield -= (chaos->data->previousShields[i] - shield) * ((FLOAT)chaos->data->modifiers[i] / 100);
                WriteProcessMemory(chaos->handles->yuzu, (LPVOID)(chaos->data->shieldsAddresses[i] + SHIELDS_OFFSET), &shield, sizeof(shield), 0);
            }
            if (chaos->states->shield && 0 < shield - chaos->data->previousShields[i] && shield - chaos->data->previousShields[i] < 1) {
                shield += (shield - chaos->data->previousShields[i]) * ((FLOAT)chaos->data->modifiers[i + MAX_FIGHTERS] / 100);
                WriteProcessMemory(chaos->handles->yuzu, (LPVOID)(chaos->data->shieldsAddresses[i] + SHIELDS_OFFSET), &shield, sizeof(shield), 0);
            }
            chaos->data->previousShields[i] = shield;
        }
    }
}

DATA *ScanBasic(uintptr_t buffer, SIZE_T bytesRead, uintptr_t baseAddress)
{
    DATA data = {0};
    UINT8 nbPercentsFound = 0, nbShieldsFound = 0;

    for (uintptr_t i = ((((baseAddress >> 0x4) + ((baseAddress & 0xf) > 0x0)) << 0x4) | 0x0) - baseAddress; i < bytesRead; i += 0x10) {
        BOOL found = TRUE;
        for (uintptr_t j = 0; found && j < sizeof(shieldsPattern) / sizeof(shieldsPattern[0]); j++)
            if (shieldsPattern[j] != *(BYTE *)(buffer + i + j))
                found = FALSE;
        if (found && nbShieldsFound < MAX_FIGHTERS) {
            data.shieldsAddresses[nbShieldsFound++] = baseAddress + i;
            continue;
        }
        found = true;
        for (uintptr_t j = 0; found && j < sizeof(percentsPattern) / sizeof(percentsPattern[0]); j++)
            if (percentsPattern[j] != *(BYTE *)(buffer + i + 0x8 + j))
                found = FALSE;
        if (found && nbPercentsFound < MAX_FIGHTERS)
            data.percentsAddresses[nbPercentsFound++] = baseAddress + i + 0x8;
    }
    if (nbPercentsFound || nbShieldsFound)
        return &data;
    return 0;
}

VOID SearchAddress(CHAOS *chaos)
{
    MEMORY_BASIC_INFORMATION mbi = {0};
    DATA *result = NULL;
    LPSTR buffer = NULL;
    SIZE_T bytesRead = 0;

    if (VirtualQueryEx(chaos->handles->yuzu, (uintptr_t *)chaos->data->index, &mbi, sizeof(mbi)) && mbi.RegionSize > 0 &&
        mbi.RegionSize < 0x200000 && mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE && mbi.Type == MEM_MAPPED) {
        buffer = (LPSTR)malloc(mbi.RegionSize);
        ReadProcessMemory(chaos->handles->yuzu, mbi.BaseAddress, buffer, mbi.RegionSize, &bytesRead);
        if (result = ScanBasic((uintptr_t)buffer, bytesRead, (uintptr_t)mbi.BaseAddress)) {
            for (UINT8 i = 0, j = 0, k = 0; i < MAX_FIGHTERS; i++) {
                if (!chaos->data->percentsAddresses[i] && result->percentsAddresses[j])
                    chaos->data->percentsAddresses[i] = result->percentsAddresses[j++];
                if (!chaos->data->shieldsAddresses[i] && result->shieldsAddresses[k])
                    chaos->data->shieldsAddresses[i] = result->shieldsAddresses[k++];
            }
        }
        free(buffer);
    }
    if (!mbi.RegionSize) {
        if (!chaos->data->percentsAddresses[0]) {
            chaos->data->index = 0;
            if (chaos->states->searchingPercents)
                HandleStatus(chaos, "1v1 not started, click Start when it is", chaos->states->searchingPercents, false);
            if (chaos->states->searchingShields)
                HandleStatus(chaos, "1v1 not started, click Start when it is", chaos->states->searchingShields, true);
            return;
        }
        if (chaos->states->searchingPercents) {
            chaos->states->damage = TRUE;
            HandleStatus(chaos, "Running...", chaos->states->searchingPercents = FALSE, false);
        }
        if (chaos->states->searchingShields) {
            chaos->states->shield = TRUE;
            HandleStatus(chaos, "Running...", chaos->states->searchingShields = FALSE, true);
        }
        for (UINT8 i = 0; i < MAX_FIGHTERS; i++) {
            if (chaos->data->percentsAddresses[i]) {
                FLOAT percent = 0;
                ReadProcessMemory(chaos->handles->yuzu, (LPCVOID)(chaos->data->percentsAddresses[i] + PERCENTS_OFFSET), &percent, sizeof(percent), 0);
                chaos->data->previousPercents[i] = percent;
                FLOAT shield = 0;
                ReadProcessMemory(chaos->handles->yuzu, (LPCVOID)(chaos->data->shieldsAddresses[i] + SHIELDS_OFFSET), &shield, sizeof(shield), 0);
                chaos->data->previousShields[i] = shield;
            }
        }
        return;
    }
    chaos->data->index += mbi.RegionSize;
}

VOID MainLoop(HWND hWindow)
{
    RECT windowRect, clientRect;

    GetWindowRect(hWindow, &windowRect);
    GetClientRect(hWindow, &clientRect);
    SetWindowPos(hWindow, 0, 0, 0, (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left) + 1280,
    (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top) + 720, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    for (CHAOS *chaos = NULL;;) {
        for (MSG msg = {0}; PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE);) {
            if (!GetMessageA(&msg, NULL, 0, 0))
                return;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        chaos = (CHAOS *)GetWindowLongPtrA(hWindow, GWLP_USERDATA);
        if (chaos->states->searchingPercents || chaos->states->searchingShields)
            SearchAddress(chaos);
        if (chaos->states->damage || chaos->states->shield)
            Chaos(chaos);
    }
}

;;      ;; ;;;; ;;    ;; ;;     ;;    ;;;    ;;;; ;;    ;;
;;  ;;  ;;  ;;  ;;;   ;; ;;;   ;;;   ;; ;;    ;;  ;;;   ;;
;;  ;;  ;;  ;;  ;;;;  ;; ;;;; ;;;;  ;;   ;;   ;;  ;;;;  ;;
;;  ;;  ;;  ;;  ;; ;; ;; ;; ;;; ;; ;;     ;;  ;;  ;; ;; ;;
;;  ;;  ;;  ;;  ;;  ;;;; ;;     ;; ;;;;;;;;;  ;;  ;;  ;;;;
;;  ;;  ;;  ;;  ;;   ;;; ;;     ;; ;;     ;;  ;;  ;;   ;;;
 ;;;  ;;;  ;;;; ;;    ;; ;;     ;; ;;     ;; ;;;; ;;    ;;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    WNDCLASSA wc = {0};

    #ifdef DEBUG
    if(AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
        freopen("CONOUT$", "w", stdout);
    #endif
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = (HICON)LoadImageA(NULL, "Chaos.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    wc.hCursor = (HCURSOR)LoadImageA(NULL, MAKEINTRESOURCEA(32512), IMAGE_CURSOR, 0, 0, LR_SHARED);
    wc.lpszClassName = "Chaos";
    RegisterClassA(&wc);
    MainLoop(
        CreateWindowA(wc.lpszClassName, "Chaos", WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX, 0, 0, 1280, 720, NULL, NULL, hInstance, NULL)
    );
}