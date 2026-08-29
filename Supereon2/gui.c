#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GDI_IMPLEMENTATION
#include "third_party/nuklear/nuklear.h"
#include "third_party/nuklear/nuklear_gdi.h"

#include "settings.h"

static void dark_theme(struct nk_context *ctx) {
    struct nk_color t[NK_COLOR_COUNT];
    nk_style_from_table(ctx, t);
    t[NK_COLOR_TEXT]              = nk_rgb(220,220,224);
    t[NK_COLOR_WINDOW]           = nk_rgb(26,27,32);
    t[NK_COLOR_HEADER]           = nk_rgb(34,36,43);
    t[NK_COLOR_BORDER]           = nk_rgb(46,48,56);
    t[NK_COLOR_BUTTON]           = nk_rgb(48,50,60);
    t[NK_COLOR_BUTTON_HOVER]     = nk_rgb(62,64,76);
    t[NK_COLOR_BUTTON_ACTIVE]    = nk_rgb(80,120,220);
    t[NK_COLOR_TOGGLE]           = nk_rgb(48,50,60);
    t[NK_COLOR_TOGGLE_HOVER]     = nk_rgb(62,64,76);
    t[NK_COLOR_TOGGLE_CURSOR]    = nk_rgb(80,120,220);
    t[NK_COLOR_SLIDER]           = nk_rgb(34,36,43);
    t[NK_COLOR_SLIDER_CURSOR]    = nk_rgb(80,120,220);
    t[NK_COLOR_SLIDER_CURSOR_HOVER]  = nk_rgb(110,150,240);
    t[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgb(130,165,255);
    t[NK_COLOR_SCROLLBAR]        = nk_rgb(34,36,43);
    t[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgb(80,120,220);
    t[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgb(110,150,240);
    t[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgb(130,165,255);
    t[NK_COLOR_PROPERTY]         = nk_rgb(34,36,43);
    // Removed: t[NK_COLOR_PROPERTY_LABEL] - this constant doesn't exist in some versions
    nk_style_from_table(ctx, t);
}

static LRESULT CALLBACK WndProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    if (m == WM_DESTROY) { g_cfg.running = 0; PostQuitMessage(0); return 0; }
    if (nk_gdi_handle_event(w, m, wp, lp)) return 0;
    return DefWindowProcW(w, m, wp, lp);
}

int run_gui(void) {
    const int W = 380, H = 520;
    WNDCLASSW wc = {0};
    wc.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.lpszClassName = L"OPAutoGUI";
    RegisterClassW(&wc);

    DWORD style = WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
    RECT r = {0,0,W,H}; AdjustWindowRect(&r, style, FALSE);
    HWND wnd = CreateWindowExW(0, wc.lpszClassName, L"OP_Auto",
        style|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        r.right-r.left, r.bottom-r.top, 0,0, wc.hInstance, 0);

    if (!wnd) return 1;

    HDC hdc = GetDC(wnd);
    GdiFont *font = nk_gdifont_create("Segoe UI", 16);
    struct nk_context *ctx = nk_gdi_init(font, hdc, W, H);
    dark_theme(ctx);

    int running = 1;
    while (running && g_cfg.running) {
        MSG msg;
        nk_input_begin(ctx);
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        nk_input_end(ctx);

        if (nk_begin(ctx, "main", nk_rect(0,0,W,H), NK_WINDOW_NO_SCROLLBAR)) {

            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, "TOGGLES", NK_TEXT_LEFT);

            nk_layout_row_dynamic(ctx, 26, 2);
            nk_checkbox_label(ctx, "Aimbot",    (int*)&g_cfg.aimbot);
            nk_checkbox_label(ctx, "Jumpbot",   (int*)&g_cfg.jumpbot);
            nk_checkbox_label(ctx, "TeamCheck", (int*)&g_cfg.teamcheck);
            nk_checkbox_label(ctx, "Duels",     (int*)&g_cfg.duels);
            nk_checkbox_label(ctx, "FOV Check", (int*)&g_cfg.fov_check);

            nk_layout_row_dynamic(ctx, 12, 1); nk_spacing(ctx, 1);
            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, "AIM", NK_TEXT_LEFT);

            nk_layout_row_dynamic(ctx, 24, 1);
            nk_property_float(ctx, "Distance",     0.1f, (float*)&g_cfg.distance,     200.0f, 0.5f, 0.25f);
            nk_property_float(ctx, "Reach Margin", 0.0f, (float*)&g_cfg.reach_margin, 10.0f,  0.1f, 0.05f);
            nk_property_float(ctx, "Wiggle Speed", 0.1f, (float*)&g_cfg.wiggle_speed, 5.0f,   0.1f, 0.05f);
            nk_property_int(ctx,   "Delay (ms)",   0,    (int*)&g_cfg.delay_ms,       1000,   1, 1);

            nk_layout_row_dynamic(ctx, 12, 1); nk_spacing(ctx, 1);
            nk_layout_row_dynamic(ctx, 24, 1);
            nk_label(ctx, "JUMPBOT", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 24, 1);
            nk_property_float(ctx, "Jump Dist",  0.1f, (float*)&g_cfg.jump_dist, 100.0f, 0.5f, 0.25f);
            nk_property_int(ctx,   "Jump Delay", 0,    (int*)&g_cfg.jump_delay_ms, 1000, 1, 1);

            nk_layout_row_dynamic(ctx, 16, 1); nk_spacing(ctx, 1);
            nk_layout_row_dynamic(ctx, 30, 1);
            const char *rm = g_cfg.reach_margin < 0.05f ? "Reach mode: NORMAL" 
                                                        : "Reach mode: ASSISTED";
            nk_label(ctx, rm, NK_TEXT_LEFT);

            nk_layout_row_dynamic(ctx, 32, 1);
            if (nk_button_label(ctx, "Exit")) { g_cfg.running = 0; running = 0; }
        }
        nk_end(ctx);

        nk_gdi_render(nk_rgb(20,21,25));
    }

    nk_gdifont_del(font);
    ReleaseDC(wnd, hdc);
    return 0;
}