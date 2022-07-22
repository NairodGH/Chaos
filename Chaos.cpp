#include <windows.h>
#include <iostream>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

using namespace std;

typedef struct {
    HWND window;
    HWND info;
    HWND start;
    HWND *sliders;
    HANDLE yuzu;
    HBITMAP startImage;
    HBITMAP stopImage;
} HANDLES;

typedef struct {
    BOOL isSearching;
    BOOL isRunning;
} STATES;

typedef struct {
    HANDLES* handles;
    INT8 *modifiers;
    STATES* states;
} CHAOS;

static CONST BYTE pattern[] = {0x10, 0x00, 0x00, 0x00, 0xF8, 0xCE, 0xF6};

void HandleMessage(CHAOS* chaos, LPCSTR info, BOOL& putFalse)
{
    SetWindowTextA(chaos->handles->info, info);
    SendMessageA(chaos->handles->start, BM_SETIMAGE, IMAGE_BITMAP,
        (putFalse ? (LPARAM)chaos->handles->startImage : (LPARAM)chaos->handles->stopImage));
    putFalse = FALSE;
}

;;      ;; ;;;; ;;    ;; ;;;;;;;;   ;;;;;;;  ;;      ;; ;;;;;;;;  ;;;;;;;;   ;;;;;;;   ;;;;;; 
;;  ;;  ;;  ;;  ;;;   ;; ;;     ;; ;;     ;; ;;  ;;  ;; ;;     ;; ;;     ;; ;;     ;; ;;    ;;
;;  ;;  ;;  ;;  ;;;;  ;; ;;     ;; ;;     ;; ;;  ;;  ;; ;;     ;; ;;     ;; ;;     ;; ;;      
;;  ;;  ;;  ;;  ;; ;; ;; ;;     ;; ;;     ;; ;;  ;;  ;; ;;;;;;;;  ;;;;;;;;  ;;     ;; ;;      
;;  ;;  ;;  ;;  ;;  ;;;; ;;     ;; ;;     ;; ;;  ;;  ;; ;;        ;;   ;;   ;;     ;; ;;      
;;  ;;  ;;  ;;  ;;   ;;; ;;     ;; ;;     ;; ;;  ;;  ;; ;;        ;;    ;;  ;;     ;; ;;    ;;
 ;;;  ;;;  ;;;; ;;    ;; ;;;;;;;;   ;;;;;;;   ;;;  ;;;  ;;        ;;     ;;  ;;;;;;;   ;;;;;; 

VOID Destroy(CHAOS* chaos)
{
    free(chaos->handles->sliders);
    free(chaos->handles);
    free(chaos->modifiers);
    free(chaos->states);
    free(chaos);
    PostQuitMessage(0);
}

BOOL CALLBACK EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM lParam)
{
    int nameLength = GetWindowTextLengthA(hwnd);
    LPSTR name = (LPSTR)malloc(nameLength + 1);

    GetWindowTextA(hwnd, name, nameLength + 1);
    if (string(name).find("yuzu") != string::npos &&
        string(name).find("Super Smash Bros. Ultimate") != string::npos) {
        *(HWND*)(lParam) = hwnd;
        free(name);
        return FALSE;
    }
    free(name);
    return TRUE;
}

VOID Command(CHAOS* chaos)
{
    DWORD pid = 0;
    HWND hYuzu = NULL;

    if (!chaos->states->isRunning && (chaos->states->isSearching = ~chaos->states->isSearching)) {
        HandleMessage(chaos, "Searching for game address...", chaos->states->isRunning);
        EnumWindows(EnumWindowsProc, (LPARAM)(&hYuzu));
        if (!hYuzu)
            return HandleMessage(chaos, "Super Smash Bros. Ultimate couldn't be found", chaos->states->isSearching);
        GetWindowThreadProcessId(hYuzu, &pid);
        chaos->handles->yuzu = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        return;
    }
    CloseHandle(chaos->handles->yuzu);
    HandleMessage(chaos, "Click start to load the hack", chaos->states->isRunning);
}

VOID Create(CHAOS* chaos)
{
    HBITMAP backgroundImage = (HBITMAP)LoadImageA(NULL, "ChaosBackground.bmp", IMAGE_BITMAP, 1280, 720, LR_LOADFROMFILE);
    HWND background = CreateWindowA("static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, 0, 0, 1280, 720, chaos->handles->window, NULL, NULL, NULL);

    SendMessage(background, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)backgroundImage);
    InitCommonControls();
    chaos->handles->info = CreateWindowA("static", "Click start to load the hack", WS_VISIBLE | WS_CHILD | SS_CENTER, 0, 290, 342, 20, chaos->handles->window, NULL, NULL, NULL);
    chaos->handles->start = CreateWindowA("button", "", WS_VISIBLE | WS_CHILD | BS_BITMAP, 0, 310, 342, 100, chaos->handles->window, (HMENU)0, NULL, NULL);
    chaos->handles->startImage = (HBITMAP)LoadImageA(NULL, "ChaosStart.bmp", IMAGE_BITMAP, 342, 100, LR_LOADFROMFILE);
    chaos->handles->stopImage = (HBITMAP)LoadImageA(NULL, "ChaosStop.bmp", IMAGE_BITMAP, 342, 100, LR_LOADFROMFILE);
    SendMessageA(chaos->handles->start, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)chaos->handles->startImage);
    for (UINT8 i = 0; i < 6; i++) {
        chaos->handles->sliders[i] = CreateWindowA("msctls_trackbar32", "", WS_VISIBLE | WS_CHILD | TBS_NOTICKS | TBS_TOOLTIPS, 0, 410 + 26 + i * 47, 342, 20, chaos->handles->window, (HMENU)i, NULL, NULL);
        SendMessageA(chaos->handles->sliders[i], TBM_SETRANGE, true, MAKELONG(-100, 100));
        SendMessageA(chaos->handles->sliders[i], TBM_SETPAGESIZE, true, 1);
    }
}

VOID NcCreate(CHAOS *chaos, HWND hwnd)
{
    chaos = (CHAOS *)calloc(1, sizeof(CHAOS));
    chaos->handles = (HANDLES *)calloc(1, sizeof(HANDLES));
    chaos->handles->sliders = (HWND *)calloc(6, sizeof(HWND));
    chaos->modifiers = (INT8 *)calloc(6, sizeof(INT8));
    chaos->states = (STATES *)calloc(1, sizeof(STATES));
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
    if (uMsg == WM_COMMAND && !wParam)
        Command(chaos);
    if (uMsg == WM_HSCROLL && LOWORD(wParam) == TB_ENDTRACK)
        chaos->modifiers[(UINT8)GetMenu((HWND)lParam)] = SendMessageA(chaos->handles->sliders[(UINT8)GetMenu((HWND)lParam)], TBM_GETPOS, 0, 0);
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

VOID Chaos(CHAOS *chaos, uintptr_t &index, FLOAT (&previousPercents)[6], uintptr_t(&addresses)[6])
{
    FLOAT percent = 0.f;

    for (UINT8 i = 0; i < 6; i++) {
        if (!addresses[i])
            break;
        if (!ReadProcessMemory(chaos->handles->yuzu, (LPCVOID)(addresses[i] - 0x810C), &percent, sizeof(percent), 0)) {
            index = 0;
            for (UINT8 i = 0; i < 6; i++)
                addresses[i] = 0;
            return HandleMessage(chaos, "1v1 finished, click start when new one begins", chaos->states->isRunning);
        }
        if (percent == 0)
            previousPercents[i] = 0;
        if (percent == previousPercents[i])
            continue;
        if (previousPercents[i] != 0) {
            percent += abs(percent - previousPercents[i]) * ((FLOAT)chaos->modifiers[i] / 100);
            WriteProcessMemory(chaos->handles->yuzu, (LPVOID)(addresses[i] - 0x810C), &percent, sizeof(percent), 0);
        }
        previousPercents[i] = percent;
    }
}
uintptr_t *ScanBasic(uintptr_t buffer, SIZE_T bytesRead, uintptr_t baseAddress)
{
    //10 00 00 00 F8 CE F6
    uintptr_t result[6] = {0};
    UINT8 nbFound = 0;

    for (uintptr_t i = ((((baseAddress >> 0x4) + ((baseAddress & 0xf) > 0xC)) << 0x4) | 0xC) - baseAddress; i < bytesRead; i += 0x10) {
        BOOL found = TRUE;
        for (uintptr_t j = 0; found && j < sizeof(pattern) / sizeof(pattern[0]); j++)
            if (!(pattern[j] == *(BYTE *)(buffer + i + j) || !j && 0x08 == *(BYTE *)(buffer + i) || !j && 0x20 == *(BYTE *)(buffer + i)))
                found = FALSE;
        if (found && *(BYTE *)(buffer + i - 0x20) == 0xFF && nbFound < 6)
            result[nbFound++] = buffer + i;
    }
    if (nbFound)
        return result;
    return 0;
}

VOID SearchAddress(CHAOS *chaos, uintptr_t &index, FLOAT(&previousPercents)[6], uintptr_t(&addresses)[6])
{
    MEMORY_BASIC_INFORMATION mbi = {0};
    uintptr_t *result = NULL;
    LPSTR buffer = NULL;
    SIZE_T bytesRead = 0;

    if (VirtualQueryEx(chaos->handles->yuzu, (uintptr_t *)index, &mbi, sizeof(mbi)) && mbi.RegionSize > 0 &&
        mbi.RegionSize < 0x200000 && mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE && mbi.Type == MEM_MAPPED) {
        buffer = (LPSTR)malloc(mbi.RegionSize);
        ReadProcessMemory(chaos->handles->yuzu, mbi.BaseAddress, buffer, mbi.RegionSize, &bytesRead);
        if (result = ScanBasic((uintptr_t)buffer, bytesRead, (uintptr_t)mbi.BaseAddress))
            for (UINT8 i = 0, j = 0; i < 6; i++)
                if (!addresses[i] && result[j])
                    addresses[i] = (uintptr_t)mbi.BaseAddress + result[j++] - (uintptr_t)buffer;
        free(buffer);
    }
    if (!mbi.RegionSize) {
        if (!addresses[0]) {
            index = 0;
            return HandleMessage(chaos, "1v1 not started, click start when it is", chaos->states->isSearching);
        }
        chaos->states->isRunning = TRUE;
        for (UINT8 i = 0; i < 6; i++) {
            if (!addresses[i])
                break;
            FLOAT percent = 0;
            ReadProcessMemory(chaos->handles->yuzu, (LPCVOID)(addresses[i] - 0x810C), &percent, sizeof(percent), 0);
            previousPercents[i] = percent;
        }
        return HandleMessage(chaos, "Running...", chaos->states->isSearching = FALSE);
    }
    index += mbi.RegionSize;
}

VOID MainLoop(HWND hWindow)
{
    CHAOS *chaos = NULL;
    uintptr_t index = 0;
    uintptr_t addresses[6] = {0};
    FLOAT previousPercents[6] = {0};
    RECT windowRect, clientRect;

    GetWindowRect(hWindow, &windowRect);
    GetClientRect(hWindow, &clientRect);
    SetWindowPos(hWindow, 0, 0, 0, 1280,
    (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top) + 720, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    for (;;) {
        for (MSG msg = {0}; PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE);) {
            if (!GetMessageA(&msg, NULL, 0, 0))
                return;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        chaos = (CHAOS *)GetWindowLongPtrA(hWindow, GWLP_USERDATA);
        if (chaos->states->isSearching)
            SearchAddress(chaos, index, previousPercents, addresses);
        if (chaos->states->isRunning)
            Chaos(chaos, index, previousPercents, addresses);
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

    /*if(AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
        freopen("CONOUT$", "w", stdout);*/
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = (HICON)LoadImageA(NULL, "Chaos.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    wc.hCursor = (HCURSOR)LoadImageA(NULL, MAKEINTRESOURCEA(32512), IMAGE_CURSOR, 0, 0, LR_SHARED);
    wc.lpszClassName = "Chaos";
    RegisterClassA(&wc);
    MainLoop(
        CreateWindowA(wc.lpszClassName, "Chaos", WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX, 2200, 50, 1280, 720, NULL, NULL, hInstance, NULL)
    );
}