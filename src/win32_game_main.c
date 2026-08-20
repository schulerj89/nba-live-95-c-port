#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdbool.h>
#include "nba_game.h"
#include "nba_audio.h"

typedef struct {
    BITMAPINFO info;
    uint32_t *pixels;
    int width;
    int height;
} Win32Framebuffer;

static Win32Framebuffer g_framebuffer;
static bool g_is_running = true;
static uint16_t g_raw_input_buttons = 0;
static NbaGame g_game;

static void win32_init_framebuffer(Win32Framebuffer *fb, int width, int height) {
    fb->width = width;
    fb->height = height;
    fb->info.bmiHeader.biSize = sizeof(fb->info.bmiHeader);
    fb->info.bmiHeader.biWidth = width;
    fb->info.bmiHeader.biHeight = -height; /* Top-down DIB */
    fb->info.bmiHeader.biPlanes = 1;
    fb->info.bmiHeader.biBitCount = 32;
    fb->info.bmiHeader.biCompression = BI_RGB;

    fb->pixels = (uint32_t *)VirtualAlloc(0, (SIZE_T)width * (SIZE_T)height * sizeof(uint32_t),
                                          MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

static void win32_free_framebuffer(Win32Framebuffer *fb) {
    if (fb->pixels) {
        VirtualFree(fb->pixels, 0, MEM_RELEASE);
        fb->pixels = NULL;
    }
}

static void win32_handle_key(WPARAM key, bool is_down) {
    uint16_t mask = 0;
    switch (key) {
        case VK_RIGHT:
        case 'D': mask = NBA_BTN_RIGHT; break;
        case VK_LEFT:
        case 'A': mask = NBA_BTN_LEFT; break;
        case VK_UP:
        case 'W': mask = NBA_BTN_UP; break;
        case VK_DOWN:
        case 'S': mask = NBA_BTN_DOWN; break;

        case 'Z':
        case 'J': mask = NBA_BTN_B; break;
        case 'X':
        case 'K': mask = NBA_BTN_A; break;
        case 'C':
        case 'U': mask = NBA_BTN_Y; break;
        case 'V':
        case 'I': mask = NBA_BTN_X; break;

        case 'Q': mask = NBA_BTN_L; break;
        case 'E': mask = NBA_BTN_R; break;

        case VK_RETURN: mask = NBA_BTN_START; break;
        case VK_SPACE:
        case VK_SHIFT:  mask = NBA_BTN_SELECT; break;

        case VK_F10:    mask = NBA_BTN_DEBUG_F10; break;
        case VK_F11:    mask = NBA_BTN_DEBUG_F11; break;

        case VK_ESCAPE:
            if (is_down) g_is_running = false;
            break;

        default: break;
    }

    if (mask) {
        if (is_down) g_raw_input_buttons |= mask;
        else g_raw_input_buttons &= ~mask;
    }
}

static LRESULT CALLBACK win32_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            g_is_running = false;
            PostQuitMessage(0);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            win32_handle_key(wparam, true);
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            win32_handle_key(wparam, false);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);
            if (g_framebuffer.pixels) {
                StretchDIBits(hdc,
                              0, 0, rect.right - rect.left, rect.bottom - rect.top,
                              0, 0, g_framebuffer.width, g_framebuffer.height,
                              g_framebuffer.pixels,
                              &g_framebuffer.info,
                              DIB_RGB_COLORS,
                              SRCCOPY);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

/**
 * Offset/Address/Size: N/A | Win32 Host Application Entry | size: N/A
 * Purpose: Creates desktop application window, initializes 60 FPS pacing timer, and hosts message/render pump.
 */
int win32_run_game(const char *rom_path, const char *assets_path,
                   bool title_only, bool setup_only) {
    HINSTANCE hInstance = GetModuleHandleA(NULL);

    WNDCLASSA wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = win32_wnd_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "NbaLive95PortWindowClass";
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Failed to register window class", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    RECT wr = {0, 0, NBA_WINDOW_WIDTH, NBA_WINDOW_HEIGHT};
    DWORD style = (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX) | WS_VISIBLE;
    AdjustWindowRect(&wr, style, FALSE);

    HWND hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "NBA Live '95 (SNES Native C Port)",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBoxA(NULL, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    win32_init_framebuffer(&g_framebuffer, NBA_SNES_WIDTH, NBA_SNES_HEIGHT);

    if (!nba_game_init(&g_game, rom_path, assets_path)) {
        MessageBoxA(hwnd, "Failed to initialize game engine", "Error", MB_OK | MB_ICONERROR);
        win32_free_framebuffer(&g_framebuffer);
        DestroyWindow(hwnd);
        return 1;
    }

    if (title_only) {
        g_game.state = NBA_STATE_TITLE_SEQUENCE;
        g_game.state_frame = 0;
        g_game.state_timer = 0.0f;
        nba_title_sequence_init(&g_game.title_sequence);
    }

    if (setup_only) {
        g_game.state = NBA_STATE_GAME_SETUP;
        g_game.state_frame = 0;
        g_game.state_timer = 0.0f;
        nba_setup_screen_init(&g_game.setup, &g_game.assets);
        g_game.setup.bgm_started = nba_audio_play_setup_spc(&g_game.assets);
    }

    /* Pacing timer setup: 59.94 Hz SNES frame rate */
    LARGE_INTEGER perf_freq, current_time;
    LONGLONG frame_ticks;
    LONGLONG next_frame_tick;
    QueryPerformanceFrequency(&perf_freq);
    const double target_frame_time = 1.0 / 59.94;
    frame_ticks = (LONGLONG)(target_frame_time * (double)perf_freq.QuadPart);
    QueryPerformanceCounter(&current_time);
    next_frame_tick = current_time.QuadPart + frame_ticks;

    timeBeginPeriod(1);

    while (g_is_running) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_is_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (!g_is_running) break;

        /* Input & frame step */
        nba_game_input_update(&g_game.input, g_raw_input_buttons);
        nba_game_tick(&g_game, (float)target_frame_time);
        nba_game_render(&g_game);

        /* Copy pixels to DIB framebuffer */
        if (g_framebuffer.pixels) {
            memcpy(g_framebuffer.pixels, g_game.renderer.pixels,
                   NBA_SNES_WIDTH * NBA_SNES_HEIGHT * sizeof(uint32_t));
        }

        /* Present to window */
        HDC hdc = GetDC(hwnd);
        RECT rect;
        GetClientRect(hwnd, &rect);
        StretchDIBits(hdc,
                      0, 0, rect.right - rect.left, rect.bottom - rect.top,
                      0, 0, g_framebuffer.width, g_framebuffer.height,
                      g_framebuffer.pixels,
                      &g_framebuffer.info,
                      DIB_RGB_COLORS,
                      SRCCOPY);
        ReleaseDC(hwnd, hdc);

        /* Frame pacing */
        for (;;) {
            QueryPerformanceCounter(&current_time);
            if (current_time.QuadPart >= next_frame_tick) break;
            if (next_frame_tick - current_time.QuadPart > perf_freq.QuadPart / 500) {
                Sleep(1);
            } else {
                SwitchToThread();
            }
        }
        next_frame_tick += frame_ticks;
        if (current_time.QuadPart > next_frame_tick + frame_ticks * 4) {
            next_frame_tick = current_time.QuadPart + frame_ticks;
        }
    }

    timeEndPeriod(1);
    nba_game_shutdown(&g_game);
    win32_free_framebuffer(&g_framebuffer);

    return 0;
}
