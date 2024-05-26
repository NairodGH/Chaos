#pragma region Requirements

#include <windows.h> // pretty much everything from the app window to memory handling and their TYPEs
#include <commctrl.h> // except trackbars...
#pragma comment(lib, "comctl32.lib") // that need to be linked manually...
#include "Chaos.h" // visual studio generated resources file for standalone executable

#define MAX_FIGHTERS 6 // enough for any 1v1, supports more depending on the chosen fighters
#define NB_CHEATS 2 // percents and shields, makes it adaptable for additional future cheats (cap)
#define PERCENTS 0
#define SHIELDS 1

// AOB anchor patterns and their respective offsets to find the real percent/shield address for each fighter,
// here was my process for finding them (percents example, it varies a lot from game to game/what you want):
// • attach CheatEngine to the process
// • make sure MEM_MAPPED scan (Settings->Scan Settings) is on since yuzu is an emulator
// • here we know percents in smash bros are decimal, so value type is float
// • damage the fighter a bit and first scan Exact value of what's displayed
// • refine the results by repeating damage->scan multiple times for increased/unchanged values
// • stop once there's fewer results/they all match the percentage
// • I've noticed yuzu copies the memory so I'd only move in the address list the first half of the results based on their addresses
// • only one of them is the actual percent value (others are for display etc...), change them 1 per 1,
//   if changing one changes some others it's probably the one, test in-game if it works to make sure
// • remove the useless others->select the good one->open Memory View
// • of course the ArrayOfByte pattern we'll search for every time won't be the percent bytes themselves since they change,
//   what we need to find is some constant AOB pattern anchor that we can get to the percent bytes from,
//   this part is tedious and requires you to understand what the batches of bytes you're seeing could mean if they were the game's code,
//   the percents are probably in some sort of Player class, by finding the AOB pattern of its start I can reliably get
//   the percent address off of it by adding the offset (distance from the AOB anchor to the actual percent address)
// • you can test it by starting another match and directly scanning for it with Exact Value/Array Of Byte, if
//   it exists and you can find the modifyable percent address by adding the offset then you're done !
// AOB anchor patterns don't usually change from yuzu/smash bros version to another but in case the cheat doesn't work anymore then
// you should first look into that, I've only had to update them once by changing their last byte
static CONST BYTE percentsPattern[] = {0x48, 0xB5, 0x2B, 0x85}; //48 B5 2B 85
static CONST BYTE shieldsPattern[] = {0x10, 0x22, 0xF8, 0x84}; //10 22 F8 84
#define PERCENTS_OFFSET 0x1BF8
#define SHIELDS_OFFSET 0x894

// #define DEBUG // allocates and attaches (in WinMain) a console to the parent process for stdout debugging
#ifdef DEBUG
#include <stdio.h>
#endif

// handles to the yuzu process, the app window and it's components
typedef struct {
    HANDLE yuzu;
    HWND window;
    HWND infos[NB_CHEATS];
    HWND starts[NB_CHEATS];
    HWND hotKeys[NB_CHEATS];
    HWND sliders[MAX_FIGHTERS * NB_CHEATS];
    HBITMAP startImage;
    HBITMAP stopImage;
} HANDLES;

// booleans to track the state of each cheat
typedef struct {
    BOOL searching[NB_CHEATS];
    BOOL active[NB_CHEATS];
    BOOL recordingKey[NB_CHEATS];
} STATES;

// data of various types needed throughout the code
typedef struct {
    uintptr_t index;
    uintptr_t addresses[MAX_FIGHTERS * NB_CHEATS];
    FLOAT previousValues[MAX_FIGHTERS * NB_CHEATS];
    INT8 modifiers[MAX_FIGHTERS * NB_CHEATS];
    DWORD keys[NB_CHEATS];
} DATA;

// main struct, basically has all values that are used in different functions
typedef struct {
    HANDLES *handles;
    STATES *states;
    DATA *data;
} CHAOS;

#pragma endregion Requirements

#pragma region WindowProc

// utility function called accross the code that handles changing the status (info and start/stop button) of cheats
VOID HandleStatus(CHAOS *chaos, LPCSTR info, BOOL *putFalse, UINT8 cheatId)
{
    SetWindowTextA(chaos->handles->infos[cheatId], info);
    SendMessageA(chaos->handles->starts[cheatId], BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(*putFalse ? chaos->handles->startImage : chaos->handles->stopImage));
    *putFalse = FALSE;
}

// sent when the window is about to be destroyed, frees the allocated chaos struct and posts an "all good" WM_QUIT message
VOID Destroy(CHAOS *chaos)
{
    free(chaos->handles);
    free(chaos->states);
    free(chaos->data);
    free(chaos);
    PostQuitMessage(0);
}

// callback function of EnumWindows that, for each of the opened windows, checks if it's yuzu with smash bros started (as it fortunately modifies it's name)
// and gets its hwnd handle through lParam if so
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

// sent when a button is clicked (or unfocused (BN_KILLFOCUS) for hot key buttons), wParam is the HMENU id defined in Create (for start/stop and hot key buttons),
// since unfocused is only used for hot key buttons (BS_NOTIFY), handle them first (based on their wParam):
// • if the button got clicked to record a key, unregister the existing one and notify that the recording is on
// • if the button got clicked to validate a key, register it and show it (as text, which means the recording is off)
// • if the key is already registered (cannot have the same HotKey registered even if different id), keep the recording on
// • if the button got clicked with no key, go back to the initial text (recording off)
// then handle start/stop buttons:
// • if we're not already searching for addresses or running the cheat, start the search by verifying Smash Bros is started with EnumWindowsProc and
//   getting its handle
// • otherwise stop the search/cheat and display the initial text
VOID Command(CHAOS *chaos, WPARAM wParam)
{
    for (UINT8 i = 0; i < NB_CHEATS; i++) {
        char text[256] = {0};

        if (LOWORD(wParam) == i + NB_CHEATS) {
            if (!chaos->states->recordingKey[i] && HIWORD(wParam) != BN_KILLFOCUS) {
                UnregisterHotKey(chaos->handles->window, i);
                SetWindowTextA(chaos->handles->hotKeys[i], "Recording...");
                chaos->states->recordingKey[i] = TRUE;
            }
            else {
                GetKeyNameTextA(MapVirtualKeyA(chaos->data->keys[i], MAPVK_VK_TO_VSC) << 16, text, sizeof(text));
                if (strlen(text)) {
                    if (chaos->states->recordingKey[i] && !RegisterHotKey(chaos->handles->window, i, MOD_NOREPEAT, chaos->data->keys[i]))
                        return;
                    memmove(text + strlen("hot key: "), text, strlen(text) + 1);
                    memcpy(text, "hot key: ", strlen("hot key: "));
                    strcat_s(text, sizeof(text), " (click to set another)");
                    SetWindowTextA(chaos->handles->hotKeys[i], text);
                }
                else
                    SetWindowTextA(chaos->handles->hotKeys[i], "Click to set a hot key");
                chaos->states->recordingKey[i] = FALSE;
            }
            return;
        }
    }
    if (!chaos->states->searching[wParam] && !chaos->states->active[wParam]) {
        DWORD pid = 0;
        HWND hYuzu = NULL;

        HandleStatus(chaos, "Searching for game address...", &chaos->states->searching[wParam], (UINT8)wParam);
        chaos->states->searching[wParam] = TRUE;
        EnumWindows(EnumWindowsProc, (LPARAM)(&hYuzu));
        if (!hYuzu) {
            HandleStatus(chaos, "Super Smash Bros. Ultimate couldn't be found", &chaos->states->searching[wParam], (UINT8)wParam);
            return;
        }
        GetWindowThreadProcessId(hYuzu, &pid);
        chaos->handles->yuzu = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        return;
    }
    if (chaos->states->searching[wParam])
        HandleStatus(chaos, "Click Start to load the hack", &chaos->states->searching[wParam], (UINT8)wParam);
    if (chaos->states->active[wParam])
        HandleStatus(chaos, "Click Start to load the hack", &chaos->states->active[wParam], (UINT8)wParam);
}

// sent when a key is released while a hot key button has keyboard focus to record it, wParam is vKey, lParam is scan code,
// displays the "Recording... *released key*" text and saves the key for validation in Command
VOID KeyUp(CHAOS *chaos, WPARAM wParam, LPARAM lParam, UINT8 cheatId)
{
    char text[256] = {0};

    GetKeyNameTextA((LONG)lParam, text, sizeof(text));
    memmove(text + strlen("Recording... "), text, strlen(text) + 1);
    memcpy(text, "Recording... ", strlen("Recording... "));
    SetWindowTextA(chaos->handles->hotKeys[cheatId], text);
    chaos->data->keys[cheatId] = (DWORD)wParam;
    
}

// subclass callback function attached to hot key buttons in Create that processes their messages like WindowProc, uIdSubclass is cheatId
LRESULT CALLBACK HotKeyProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    CHAOS *chaos = (CHAOS *)GetWindowLongPtrA(GetParent(hwnd), GWLP_USERDATA);

    if (uMsg == WM_KEYUP && chaos->states->recordingKey[uIdSubclass])
        KeyUp(chaos, wParam, lParam, (UINT8)uIdSubclass);
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

// sent after WM_NCCREATE when the app window is created,
// resizes it manually because for some reason the header bar is counted in the height which would offset the rest,
// creates all the elements of the app from the background to the sliders and stores their handles for later
VOID Create(CHAOS *chaos)
{
    RECT windowRect, clientRect;
    HBITMAP backgroundImage = LoadBitmapA((HINSTANCE)GetWindowLongPtrA(chaos->handles->window, GWLP_HINSTANCE), MAKEINTRESOURCEA(IDB_BITMAP1));
    HWND background = CreateWindowA("static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, 0, 0, 1280, 720, chaos->handles->window, NULL, NULL, NULL);

    GetWindowRect(chaos->handles->window, &windowRect);
    GetClientRect(chaos->handles->window, &clientRect);
    SetWindowPos(chaos->handles->window, 0, 0, 0, (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left) + 1280, (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top) + 720, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    SendMessageA(background, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)backgroundImage);
    InitCommonControls();
    chaos->handles->infos[PERCENTS] = CreateWindowA("static", "Click Start to load the hack", WS_VISIBLE | WS_CHILD | SS_CENTER, 0, 270, 342, 20, chaos->handles->window, NULL, NULL, NULL);
    chaos->handles->infos[SHIELDS] = CreateWindowA("static", "Click Start to load the hack", WS_VISIBLE | WS_CHILD | SS_CENTER, 939, 270, 342, 20, chaos->handles->window, NULL, NULL, NULL);
    chaos->handles->starts[PERCENTS] = CreateWindowA("button", "", WS_VISIBLE | WS_CHILD | BS_BITMAP, 0, 290, 342, 100, chaos->handles->window, (HMENU)0, NULL, NULL);
    chaos->handles->starts[SHIELDS] = CreateWindowA("button", "", WS_VISIBLE | WS_CHILD | BS_BITMAP, 939, 290, 342, 100, chaos->handles->window, (HMENU)1, NULL, NULL);
    chaos->handles->startImage = LoadBitmapA((HINSTANCE)GetWindowLongPtrA(chaos->handles->window, GWLP_HINSTANCE), MAKEINTRESOURCEA(IDB_BITMAP2));
    chaos->handles->stopImage = LoadBitmapA((HINSTANCE)GetWindowLongPtrA(chaos->handles->window, GWLP_HINSTANCE), MAKEINTRESOURCEA(IDB_BITMAP3));
    SendMessageA(chaos->handles->starts[PERCENTS], BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)chaos->handles->startImage);
    SendMessageA(chaos->handles->starts[SHIELDS], BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)chaos->handles->startImage);
    chaos->handles->hotKeys[PERCENTS] = CreateWindowA("button", "Click to set a hot key", WS_VISIBLE | WS_CHILD | BS_CENTER | BS_NOTIFY | BS_FLAT, 0, 390, 342, 20, chaos->handles->window, (HMENU)2, NULL, NULL);
    chaos->handles->hotKeys[SHIELDS] = CreateWindowA("button", "Click to set a hot key", WS_VISIBLE | WS_CHILD | BS_CENTER | BS_NOTIFY, 939, 390, 342, 20, chaos->handles->window, (HMENU)3, NULL, NULL);
    SetWindowSubclass(chaos->handles->hotKeys[PERCENTS], HotKeyProc, PERCENTS, 0);
    SetWindowSubclass(chaos->handles->hotKeys[SHIELDS], HotKeyProc, SHIELDS, 0);
    for (UINT8 i = 0; i < MAX_FIGHTERS * NB_CHEATS; i++) {
        chaos->handles->sliders[i] = CreateWindowA("msctls_trackbar32", "", WS_VISIBLE | WS_CHILD | TBS_NOTICKS | TBS_TOOLTIPS, i / MAX_FIGHTERS * 939, 410 + 26 + i % MAX_FIGHTERS * 47, 342, 20, chaos->handles->window, (HMENU)i, NULL, NULL);
        SendMessageA(chaos->handles->sliders[i], TBM_SETRANGE, TRUE, MAKELONG(-100, 100));
        SendMessageA(chaos->handles->sliders[i], TBM_SETPAGESIZE, 0, 1);
    }
}

// sent before WM_CREATE when the app window is created, allocates the chaos struct and stores it in a special space in the app window made for user data
// which allows use to retrieve it even in windows functions where we couldn't have it passed as parameter ! (and god forbid no global variables)
VOID NcCreate(CHAOS *chaos, HWND hwnd)
{
    chaos = (CHAOS *)calloc(1, sizeof(CHAOS));
    chaos->handles = (HANDLES *)calloc(1, sizeof(HANDLES));
    chaos->states = (STATES *)calloc(1, sizeof(STATES));
    chaos->data = (DATA *)calloc(1, sizeof(DATA));
    chaos->handles->window = hwnd;
    SetWindowLongPtrA(chaos->handles->window, GWLP_USERDATA, (LONG_PTR)chaos);
}

// callback function attached to WinMain that processes messages sent to it when the user interacts, DefWindowProcA if we don't care about it
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHAOS *chaos = (CHAOS *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    if (uMsg == WM_NCCREATE)
        NcCreate(chaos, hwnd);
    if (uMsg == WM_CREATE)
        Create(chaos);
    if (uMsg == WM_COMMAND && HIWORD(wParam) != BN_SETFOCUS || uMsg == WM_HOTKEY)
        Command(chaos, wParam);
    if (uMsg == WM_HSCROLL && LOWORD(wParam) == TB_THUMBPOSITION)
        chaos->data->modifiers[(UINT8)GetMenu((HWND)lParam)] = (INT8)HIWORD(wParam);
    if (uMsg == WM_DESTROY)
        Destroy(chaos);
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

#pragma endregion WindowProc

#pragma region MainLoop

// the final function that changes the percents and shields addresses based on the trackbar % modifiers (-100 to 100),
// for each of the maximum supported fighters
// • check if an address is readable, if its not valid anymore then the match ended (since addresses change every match)
// • if the damage cheat is on, write the new percent at its address ((current percent - previous percent) * modifier),
//   the modifier however works in all scenarios including the ones where we don't want it to such as when the player dies (from percent to 0)
//   or swaps fighters (aegis/pokemon trainer, when you swap from A to B it copies A's percent to B but for some reason it shortly becomes 0 inbetween),
//   fortunately we can filter out both exception when percent is 0 (it however means that the first hit from 0 will always be unmodified)
// • if the damage cheat is on, we also need to apply it on the shield, which doesn't follow the same percent logic, the problem is the shield can
//   also damage itself if we maintain it so we differentiate that from actual damage taken by the amount (if we maintain, the difference will be under
//   1% of damage while attacks will all do more than 1%) and apply the modifier
// • if the shield cheat is on, we modify it just like for percents and filter out the case where the shield goes from 0 to 37.5 when it breaks
// as mentionned in MainLoop, this function is the content of what a Chaos loop would be with addresses being read at every tick (fast)
VOID Chaos(CHAOS *chaos)
{
    FLOAT percent = 0, shield = 0;

    for (UINT8 i = 0; i < MAX_FIGHTERS; i++) {
        if (!chaos->data->addresses[i])
            break;
        if (!ReadProcessMemory(chaos->handles->yuzu, (LPCVOID)(chaos->data->addresses[i] + PERCENTS_OFFSET), &percent, sizeof(percent), 0)) {
            chaos->data->index = 0;
            for (UINT8 j = 0; j < MAX_FIGHTERS; j++)
                chaos->data->addresses[j] = chaos->data->addresses[MAX_FIGHTERS + j] = 0;
            for (UINT8 j = 0; j < NB_CHEATS; j++)
                if (chaos->states->active[j])
                    HandleStatus(chaos, "Match finished, click Start when new one begins", &chaos->states->active[j], j);
            return;
        }
        if (chaos->states->active[PERCENTS]) {
            if (percent == 0)
                chaos->data->previousValues[i] = 0;
            if (percent > chaos->data->previousValues[i] && chaos->data->previousValues[i] != 0) {
                percent += (percent - chaos->data->previousValues[i]) * ((FLOAT)chaos->data->modifiers[i] / 100);
                WriteProcessMemory(chaos->handles->yuzu, (LPVOID)(chaos->data->addresses[i] + PERCENTS_OFFSET), &percent, sizeof(percent), 0);
            }
            chaos->data->previousValues[i] = percent;
        }
        if (ReadProcessMemory(chaos->handles->yuzu, (LPCVOID)(chaos->data->addresses[MAX_FIGHTERS + i] + SHIELDS_OFFSET), &shield, sizeof(shield), 0)) {
            if (chaos->states->active[PERCENTS] && chaos->data->previousValues[MAX_FIGHTERS + i] - shield > 1) {
                shield -= (chaos->data->previousValues[MAX_FIGHTERS + i] - shield) * ((FLOAT)chaos->data->modifiers[i] / 100);
                WriteProcessMemory(chaos->handles->yuzu, (LPVOID)(chaos->data->addresses[MAX_FIGHTERS + i] + SHIELDS_OFFSET), &shield, sizeof(shield), 0);
            }
            if (chaos->states->active[SHIELDS] && 0 < shield - chaos->data->previousValues[MAX_FIGHTERS + i] && shield - chaos->data->previousValues[MAX_FIGHTERS + i] < 1) {
                shield += (shield - chaos->data->previousValues[MAX_FIGHTERS + i]) * ((FLOAT)chaos->data->modifiers[MAX_FIGHTERS + i] / 100);
                WriteProcessMemory(chaos->handles->yuzu, (LPVOID)(chaos->data->addresses[MAX_FIGHTERS + i] + SHIELDS_OFFSET), &shield, sizeof(shield), 0);
            }
            chaos->data->previousValues[MAX_FIGHTERS + i] = shield;
        }
    }
}

// scans for percent and shield AOB anchor patterns at baseAddress (the page of size bytesRead is copied in buffer),
// I've noticed that everytime all the hexadecimal addresses of shield AOBs end by 0 and by 8 for percents',
// i can therefore start as the offset between baseAddress and the address after baseAddress that ends with 0
// and iterate 10 by 10 to save lots of time (turns out i will always start as 0 because RAM pages are allocated directly one after the other and
// all have a multiple of 4KB (4096 or 0x1000) size which means their hexadecimal start address will always end with at least 000 but I still kept the
// cool bitwise operations that calculate the offset because it makes me feel smart),
// everytime, for both shield and percents (except that for percents we then add 8 to the address), it
// • iterates through the pattern-length next bytes comparing them to the pattern
// • if all the bytes correspond and we havent found MAX_FIGHTERS addresses already (remember how memory is duplicated ?
//   also turns out Nintendo always allocates shields for the maximum possible 3 (pokemon trainer) x 8 (max players in a match) = 24 instances 💀)
//   then save the address
// if we found some addresses (not all addresses will be in the same page) return them
DATA *ScanPatterns(uintptr_t buffer, SIZE_T bytesRead, uintptr_t baseAddress)
{
    DATA *data = (DATA *)calloc(1, sizeof(DATA));
    UINT8 nbPercentsFound = 0, nbShieldsFound = 0;

    for (uintptr_t i = ((((baseAddress >> 0x4) + ((baseAddress & 0xf) > 0x0)) << 0x4) | 0x0) - baseAddress; i < bytesRead; i += 0x10) {
        BOOL found = TRUE;
        for (uintptr_t j = 0; found && j < sizeof(shieldsPattern) / sizeof(shieldsPattern[0]); j++)
            if (shieldsPattern[j] != *(BYTE *)(buffer + i + j))
                found = FALSE;
        if (found && nbShieldsFound < MAX_FIGHTERS) {
            data->addresses[MAX_FIGHTERS + nbShieldsFound++] = baseAddress + i;
            continue;
        }
        found = TRUE;
        for (uintptr_t j = 0; found && j < sizeof(percentsPattern) / sizeof(percentsPattern[0]); j++)
            if (percentsPattern[j] != *(BYTE *)(buffer + i + 0x8 + j))
                found = FALSE;
        if (found && nbPercentsFound < MAX_FIGHTERS)
            data->addresses[nbPercentsFound++] = baseAddress + i + 0x8;
    }
    if (nbPercentsFound || nbShieldsFound)
        return data;
    free(data);
    return NULL;
}

// retrieves info on the yuzu RAM page at index, if it
// • has a size big enough to contain the AOB patterns we'll be searching for (the 0x200000 max size is an arbitrary value I've decided upon noticing the pages
//   where I'd find the patterns would always be smaller than it, scanning for giant pages where it's not would be a big waste of time)
// • is a committed page for which physical storage has been allocated (basically has data)
// • has read and write permissions since we know the values we'll be searching for are the actual game variables that read/write to it
// • is of type MEM_MAPPED, which is what emulators use
// then it's a valid candidate that we can read into a buffer, ScanPatterns and save found addresses
// if the page is empty then we reached the end of yuzu's RAM so:
// • if we didn't find our addresses then the match wasn't started (the percent/shield variables weren't created)
// • otherwise we change the status to running and set straight away the current percent/shield values (see why in Chaos)
// as mentionned in MainLoop, this function is the content of what a SearchAddresses loop would be with index being increased to the next page
VOID SearchAddresses(CHAOS *chaos)
{
    MEMORY_BASIC_INFORMATION mbi = {0};
    DATA *result = NULL;
    LPSTR buffer = NULL;
    SIZE_T bytesRead = 0;

    if (VirtualQueryEx(chaos->handles->yuzu, (uintptr_t *)chaos->data->index, &mbi, sizeof(mbi)) && mbi.RegionSize > 0 &&
        mbi.RegionSize < 0x200000 && mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE && mbi.Type == MEM_MAPPED) {
        buffer = (LPSTR)malloc(mbi.RegionSize);
        ReadProcessMemory(chaos->handles->yuzu, mbi.BaseAddress, buffer, mbi.RegionSize, &bytesRead);
        if (result = ScanPatterns((uintptr_t)buffer, bytesRead, (uintptr_t)mbi.BaseAddress)) {
            for (UINT8 i = 0, j = 0, k = 0; i < MAX_FIGHTERS; i++) {
                if (!chaos->data->addresses[i] && result->addresses[j])
                    chaos->data->addresses[i] = result->addresses[j++];
                if (!chaos->data->addresses[MAX_FIGHTERS + i] && result->addresses[MAX_FIGHTERS + k])
                    chaos->data->addresses[MAX_FIGHTERS + i] = result->addresses[MAX_FIGHTERS + k++];
            }
            free(result);
        }
        free(buffer);
    }
    if (!mbi.RegionSize) {
        if (!chaos->data->addresses[0]) {
            chaos->data->index = 0;
            for (UINT8 i = 0; i < NB_CHEATS; i++)
                if (chaos->states->searching[i])
                    HandleStatus(chaos, "Match not started, click Start when it is", &chaos->states->searching[i], i);
            return;
        }
        for (UINT8 i = 0; i < NB_CHEATS; i++) {
            if (chaos->states->searching[i]) {
                chaos->states->active[i] = TRUE;
                chaos->states->searching[i] = FALSE;
                HandleStatus(chaos, "Running...", &chaos->states->searching[i], i);
            }
        }
        for (UINT8 i = 0; i < MAX_FIGHTERS; i++) {
            if (chaos->data->addresses[i]) {
                FLOAT value = 0;
                ReadProcessMemory(chaos->handles->yuzu, (LPCVOID)(chaos->data->addresses[i] + PERCENTS_OFFSET), &value, sizeof(value), 0);
                chaos->data->previousValues[i] = value;
                ReadProcessMemory(chaos->handles->yuzu, (LPCVOID)(chaos->data->addresses[MAX_FIGHTERS + i] + SHIELDS_OFFSET), &value, sizeof(value), 0);
                chaos->data->previousValues[i] = value;
            }
        }
        return;
    }
    chaos->data->index += mbi.RegionSize;
}

// called by WinMain with the app's window, starts the main infinite loop which:
// • peeks messages sent to the window, if one's WM_QUIT then the window closes, if not then translate and dispatch them to WindowProc
// • if a search is on, run 1 SearchAddresses
// • if a cheat is on, run 1 Chaos
// since messages need to be checked at all time as they're used by WindowProc functions to make the window interactive,
// this basically avoids SearchAddresses and Chaos having their own loops which would block the application
VOID MainLoop(HWND hWindow)
{
    for (CHAOS *chaos = NULL;;) {
        for (MSG msg = {0}; PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE);) {
            if (!GetMessageA(&msg, NULL, 0, 0))
                return;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        chaos = (CHAOS *)GetWindowLongPtrA(hWindow, GWLP_USERDATA);
        for (UINT8 i = 0; i < NB_CHEATS; i++) {
            if (chaos->states->searching[i])
                SearchAddresses(chaos);
            if (chaos->states->active[i])
                Chaos(chaos);
        }
    }
}

#pragma endregion MainLoop

#pragma region WinMain

// conventional name used for a Win32 app entry point (replaces main), creates/registers a window class (WindowProc, instance, icon, cursor, name)
// and uses it to create the actual app window with a fixed 1280x720 size (works on most monitors, cba to make it responsive)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    WNDCLASSA wc = {0};

    #ifdef DEBUG
    if(AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
        FILE* filePointer = NULL;
        freopen_s(&filePointer, "CONOUT$", "w", stdout);
    }
    #endif
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_ICON1));
    wc.hCursor = (HCURSOR)LoadImageA(NULL, MAKEINTRESOURCEA(32512), IMAGE_CURSOR, 0, 0, LR_SHARED);
    wc.lpszClassName = "Chaos";
    RegisterClassA(&wc);
    MainLoop(CreateWindowA(wc.lpszClassName, "Chaos", WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX, 0, 0, 1280, 720, NULL, NULL, hInstance, NULL));
}

#pragma endregion WinMain