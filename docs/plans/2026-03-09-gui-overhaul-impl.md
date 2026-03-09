# GUI Overhaul Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the single-panel dark UI with a Fluent/Win11-inspired owner-drawn interface featuring a 220 px left sidebar, three pages (Dashboard, Appliances, Settings), and a live light/dark theme toggle.

**Architecture:** All rendering stays in pure GDI — no new libraries. The sidebar is hit-tested manually in `WM_LBUTTONDOWN`; child Win32 controls (ListView, Edit, Buttons) are shown/hidden per page rather than destroyed. A `Theme` struct replaces all `#define CLR_*` macros and two instances (`g_dark`, `g_light`) are swapped at runtime via `RebuildBrushes()`.

**Tech Stack:** C++17, Win32 API, GDI (gdi32, comctl32, uxtheme, dwmapi), MinGW g++ or MSVC.

**Build command (MinGW):**
```bash
"/c/Program Files/CodeBlocks/MinGW/bin/g++.exe" -std=c++17 -O2 power_calculator.cpp \
  -o power_calculator.exe -mwindows -lcomctl32 -luxtheme -ldwmapi
```

---

## Task 1: Replace `#define` colors with a `Theme` struct

**Files:**
- Modify: `power_calculator.cpp` — top of file, color palette section (lines 31-49)

**Step 1: Define the Theme struct and two instances**

Replace the entire `// ── colour palette ──` block with:

```cpp
// ── theme system ──────────────────────────────────────────────────────────
struct Theme {
    COLORREF bg, surface, surfaceHov, accent, accentPre, danger, dangerHov;
    COLORREF text, subtext, border, editBg, sidebar, rowEven, rowOdd, rowSel;
    COLORREF cardBlue, cardGreen, cardAmber;
};

static const Theme DARK = {
    RGB( 32, 32, 32), // bg
    RGB( 43, 43, 43), // surface
    RGB( 48, 48, 48), // surfaceHov
    RGB( 96,205,255), // accent       #60CDFF
    RGB( 77,184,232), // accentPre    #4DB8E8
    RGB(255,102,102), // danger       #FF6666
    RGB(255,140,140), // dangerHov
    RGB(255,255,255), // text
    RGB(157,157,157), // subtext
    RGB( 61, 61, 61), // border
    RGB( 30, 30, 30), // editBg
    RGB( 24, 24, 24), // sidebar
    RGB( 32, 32, 32), // rowEven
    RGB( 38, 38, 38), // rowOdd
    RGB( 55, 85,130), // rowSel
    RGB( 96,205,255), // cardBlue
    RGB( 74,222,128), // cardGreen
    RGB(251,191, 36), // cardAmber
};

static const Theme LIGHT = {
    RGB(243,243,243), // bg
    RGB(255,255,255), // surface
    RGB(245,245,245), // surfaceHov
    RGB(  0,120,212), // accent       #0078D4
    RGB(  0,108,190), // accentPre    #006CBE
    RGB(196, 43, 28), // danger       #C42B1C
    RGB(210, 70, 50), // dangerHov
    RGB( 26, 26, 26), // text
    RGB( 96, 94, 92), // subtext
    RGB(224,224,224), // border
    RGB(251,251,251), // editBg
    RGB(232,232,232), // sidebar
    RGB(243,243,243), // rowEven
    RGB(249,249,249), // rowOdd
    RGB(204,228,247), // rowSel
    RGB(  0,120,212), // cardBlue
    RGB( 22,163, 74), // cardGreen
    RGB(217,119,  6), // cardAmber
};

static bool    g_darkMode = true;
static Theme   T = DARK;   // active theme — always use T.xxx for colors
```

**Step 2: Remove the old `#define CLR_*` lines**

Delete lines 31–49 (the original color defines). Also replace every `CLR_*` reference in the file with `T.*` using the mapping below:

| Old macro       | New field       |
|-----------------|-----------------|
| `CLR_BG`        | `T.bg`          |
| `CLR_SURFACE`   | `T.surface`     |
| `CLR_SURFACE2`  | `T.surfaceHov`  |
| `CLR_ACCENT`    | `T.accent`      |
| `CLR_ACCENT_HOV`| `T.accent`      |
| `CLR_ACCENT_PRE`| `T.accentPre`   |
| `CLR_DANGER`    | `T.danger`      |
| `CLR_DANGER_HOV`| `T.dangerHov`   |
| `CLR_TEXT`      | `T.text`        |
| `CLR_SUBTEXT`   | `T.subtext`     |
| `CLR_BORDER`    | `T.border`      |
| `CLR_EDIT_BG`   | `T.editBg`      |
| `CLR_ROW_EVEN`  | `T.rowEven`     |
| `CLR_ROW_ODD`   | `T.rowOdd`      |
| `CLR_ROW_SEL`   | `T.rowSel`      |
| `CLR_HDR_BG`    | `T.surface`     |
| `CLR_TOPBAR`    | `T.accent`      |

**Step 3: Add `RebuildBrushes()` and update `CreateBrushes()`/`DestroyBrushes()`**

```cpp
static void RebuildBrushes() {
    DestroyBrushes();
    brBg       = CreateSolidBrush(T.bg);
    brSurface  = CreateSolidBrush(T.surface);
    brSurface2 = CreateSolidBrush(T.surfaceHov);
    brAccent   = CreateSolidBrush(T.accent);
    brEditBg   = CreateSolidBrush(T.editBg);
    brBorder   = CreateSolidBrush(T.border);
}
```

Replace `CreateBrushes()` body with a call to `RebuildBrushes()`.

**Step 4: Build and verify no errors**

```bash
"/c/Program Files/CodeBlocks/MinGW/bin/g++.exe" -std=c++17 power_calculator.cpp \
  -o power_calculator.exe -mwindows -lcomctl32 -luxtheme -ldwmapi 2>&1 | grep -v "pragma comment"
```
Expected: no output (clean build). App should look identical to before (still dark).

**Step 5: Commit**
```bash
git add power_calculator.cpp
git commit -m "refactor: replace CLR_ defines with Theme struct, add dark/light palettes"
```

---

## Task 2: Add page enum, sidebar constants, and g_page global

**Files:**
- Modify: `power_calculator.cpp` — constants and globals sections

**Step 1: Add page enum and sidebar constant after the existing enums**

```cpp
// ── pages ─────────────────────────────────────────────────────────────────
enum Page { PAGE_DASHBOARD = 0, PAGE_APPLIANCES, PAGE_SETTINGS };
static Page g_page = PAGE_DASHBOARD;

const int SIDEBAR_W = 220;   // sidebar width in pixels
const int TITLEBAR_H = 52;   // content area page-title height
```

**Step 2: Add sidebar hover tracking global**

```cpp
static int g_sideHover = -1;  // index of hovered nav item (-1 = none)
```

**Step 3: Build and verify**

Same build command. Expected: clean build, no behavior change.

**Step 4: Commit**
```bash
git commit -am "feat: add Page enum, SIDEBAR_W constant, g_page global"
```

---

## Task 3: Draw the sidebar in WM_PAINT

**Files:**
- Modify: `power_calculator.cpp` — `WndProc` `WM_PAINT` handler, add `DrawSidebar()` helper

**Step 1: Add `DrawSidebar()` before `WndProc`**

```cpp
struct NavItem { const wchar_t* icon; const wchar_t* label; Page page; };
static const NavItem NAV[] = {
    { L"⊞", L"Dashboard",  PAGE_DASHBOARD  },
    { L"☰", L"Appliances", PAGE_APPLIANCES },
    { L"⚙", L"Settings",   PAGE_SETTINGS   },
};
static const int NAV_COUNT = 3;
static const int NAV_ITEM_H = 40;
static const int NAV_TOP    = 72;  // y of first nav item

static void DrawSidebar(HDC hdc, int H) {
    RECT sb = { 0, 0, SIDEBAR_W, H };
    HBRUSH brSb = CreateSolidBrush(T.sidebar);
    FillRect(hdc, &sb, brSb);
    DeleteObject(brSb);

    // App title
    HFONT oldF = (HFONT)SelectObject(hdc, fBold);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, T.accent);
    TextOutW(hdc, 18, 18, L"⚡ Energy Calc", 14);
    SelectObject(hdc, oldF);

    // Divider below title
    HPEN pen = CreatePen(PS_SOLID, 1, T.border);
    HPEN oldP = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, 12, 56, nullptr);
    LineTo(hdc, SIDEBAR_W - 12, 56);
    SelectObject(hdc, oldP);
    DeleteObject(pen);

    // Nav items
    for (int i = 0; i < NAV_COUNT; i++) {
        int y = NAV_TOP + i * NAV_ITEM_H;
        bool active = (NAV[i].page == g_page);
        bool hov    = (g_sideHover == i);

        // Background fill on hover / active
        if (active || hov) {
            COLORREF fill = active ? T.surfaceHov : T.surfaceHov;
            HBRUSH br = CreateSolidBrush(fill);
            RECT r = { 0, y, SIDEBAR_W, y + NAV_ITEM_H };
            FillRect(hdc, &r, br);
            DeleteObject(br);
        }

        // Accent pill on active
        if (active) {
            HBRUSH brPill = CreateSolidBrush(T.accent);
            RECT pill = { 0, y + 11, 3, y + NAV_ITEM_H - 11 };
            FillRect(hdc, &pill, brPill);
            DeleteObject(brPill);
        }

        // Icon
        SelectObject(hdc, fUI);
        SetTextColor(hdc, active ? T.accent : T.subtext);
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, 18, y + 12, NAV[i].icon, 1);

        // Label
        SelectObject(hdc, active ? fBold : fUI);
        SetTextColor(hdc, active ? T.text : T.subtext);
        TextOutW(hdc, 42, y + 12, NAV[i].label, (int)wcslen(NAV[i].label));
    }

    // Theme toggle at bottom
    int ty = H - 44;
    SelectObject(hdc, fUI);
    SetTextColor(hdc, T.subtext);
    const wchar_t* toggleTxt = g_darkMode ? L"☀  Light mode" : L"🌙  Dark mode";
    TextOutW(hdc, 18, ty, toggleTxt, (int)wcslen(toggleTxt));

    // Sidebar right border
    pen = CreatePen(PS_SOLID, 1, T.border);
    oldP = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, SIDEBAR_W - 1, 0, nullptr);
    LineTo(hdc, SIDEBAR_W - 1, H);
    SelectObject(hdc, oldP);
    DeleteObject(pen);
}
```

**Step 2: Update `WM_PAINT` to call `DrawSidebar`**

At the start of the `WM_PAINT` handler, before any other painting, add:
```cpp
DrawSidebar(hdc, rc.bottom);
```

Remove the old `DrawAccentLine` call at the top.

**Step 3: Build and run — verify sidebar appears on the left**

Expected: dark strip on the left with ⚡ title, three nav labels, theme toggle text.
The existing content will overlap until Task 4 fixes layout.

**Step 4: Commit**
```bash
git commit -am "feat: draw sidebar with nav items, active pill, and theme toggle text"
```

---

## Task 4: Sidebar hit-testing (click to change page, click to toggle theme)

**Files:**
- Modify: `power_calculator.cpp` — `WndProc` `WM_LBUTTONDOWN` and `WM_MOUSEMOVE` handlers

**Step 1: Add `WM_LBUTTONDOWN` case to `WndProc`**

```cpp
case WM_LBUTTONDOWN: {
    int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
    if (mx < SIDEBAR_W) {
        // Theme toggle
        if (my >= HIWORD(GetClientRect(hWnd, nullptr), nullptr) - 44) {
            // handled below with helper
        }
        // Nav items
        for (int i = 0; i < NAV_COUNT; i++) {
            int y = NAV_TOP + i * NAV_ITEM_H;
            if (my >= y && my < y + NAV_ITEM_H) {
                if (NAV[i].page != g_page) {
                    g_page = NAV[i].page;
                    DoLayout(hWnd);
                    InvalidateRect(hWnd, nullptr, TRUE);
                }
                return 0;
            }
        }
        // Theme toggle (bottom 44px)
        RECT rc2; GetClientRect(hWnd, &rc2);
        if (my >= rc2.bottom - 44) {
            g_darkMode = !g_darkMode;
            T = g_darkMode ? DARK : LIGHT;
            RebuildBrushes();
            // Update ListView colors
            ListView_SetBkColor(hList,     T.bg);
            ListView_SetTextBkColor(hList, T.bg);
            ListView_SetTextColor(hList,   T.text);
            DoLayout(hWnd);
            InvalidateRect(hWnd, nullptr, TRUE);
            UpdateWindow(hWnd);
        }
    }
    return 0;
}
```

**Step 2: Add `WM_MOUSEMOVE` case for sidebar hover**

```cpp
case WM_MOUSEMOVE: {
    int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
    int newHov = -1;
    if (mx < SIDEBAR_W) {
        for (int i = 0; i < NAV_COUNT; i++) {
            int y = NAV_TOP + i * NAV_ITEM_H;
            if (my >= y && my < y + NAV_ITEM_H) { newHov = i; break; }
        }
    }
    if (newHov != g_sideHover) {
        g_sideHover = newHov;
        // Invalidate only sidebar strip
        RECT sideRc; GetClientRect(hWnd, &sideRc);
        sideRc.right = SIDEBAR_W;
        InvalidateRect(hWnd, &sideRc, FALSE);
        // Track WM_MOUSELEAVE to clear hover
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
        TrackMouseEvent(&tme);
    }
    return 0;
}

case WM_MOUSELEAVE:
    if (g_sideHover != -1) {
        g_sideHover = -1;
        RECT sideRc; GetClientRect(hWnd, &sideRc);
        sideRc.right = SIDEBAR_W;
        InvalidateRect(hWnd, &sideRc, FALSE);
    }
    return 0;
```

**Step 3: Build and verify**

Click nav items — page should switch (content won't change until Task 5/6/7/8 but sidebar active state should update). Click theme toggle — colors should swap live.

**Step 4: Commit**
```bash
git commit -am "feat: sidebar click nav, hover highlight, theme toggle"
```

---

## Task 5: Rework DoLayout() for sidebar + three pages

**Files:**
- Modify: `power_calculator.cpp` — `DoLayout()` function

**Step 1: Replace `DoLayout()` entirely**

```cpp
static void DoLayout(HWND hWnd) {
    RECT rc; GetClientRect(hWnd, &rc);
    int W = rc.right, H = rc.bottom;

    // Status bar
    if (hStat) { SendMessage(hStat, WM_SIZE, 0, 0); }
    int SH = 0;
    if (hStat) { RECT sr; GetClientRect(hStat, &sr); SH = sr.bottom; }

    int cX = SIDEBAR_W + 1;          // content area left edge
    int cW = W - cX;                 // content area width
    int cY = TITLEBAR_H;             // content starts below page title
    int cH = H - SH - cY;           // content height
    const int PAD = 16;

    // ── Dashboard controls ─────────────────────────────────────────────
    // (stat cards are painted; only the mini-list needs positioning)
    // The mini list is hList when on dashboard — hidden on other pages
    // We handle show/hide below.

    // ── Appliances page ────────────────────────────────────────────────
    // Toolbar above list
    int tbY = cY + PAD;
    int BW = 100, BH = 32;
    struct { int id; } appBtns[] = { {ID_ADD},{ID_EDIT},{ID_DEL},{ID_EXP} };
    // Right-align buttons
    int btnX = cX + cW - PAD - (BW + 8) * 4 + 8;
    for (int i = 0; i < 4; i++) {
        HWND h = GetDlgItem(hWnd, appBtns[i].id);
        if (h) SetWindowPos(h, nullptr, btnX + i*(BW+8), tbY, BW, BH,
                            SWP_NOZORDER|SWP_NOACTIVATE);
    }

    // ListView fills rest of content area
    int listY = tbY + BH + PAD;
    int listH = H - SH - listY - PAD;
    if (listH < 80) listH = 80;
    if (hList) SetWindowPos(hList, nullptr, cX + PAD, listY,
                            cW - PAD*2, listH, SWP_NOZORDER|SWP_NOACTIVATE);

    // ── Settings page ──────────────────────────────────────────────────
    if (hRate) SetWindowPos(hRate, nullptr, cX + PAD, cY + 60,
                            200, 30, SWP_NOZORDER|SWP_NOACTIVATE);
    HWND hrb = GetDlgItem(hWnd, ID_RATEBTN);
    if (hrb) SetWindowPos(hrb, nullptr, cX + PAD + 208, cY + 60,
                          110, 30, SWP_NOZORDER|SWP_NOACTIVATE);
    HWND hrl = GetDlgItem(hWnd, ID_RATELBL);
    if (hrl) SetWindowPos(hrl, nullptr, cX + PAD, cY + 36,
                          300, 20, SWP_NOZORDER|SWP_NOACTIVATE);

    // Theme toggle buttons (ID_THMDARK, ID_THMLIGHT — added in Task 8)
    HWND hTD = GetDlgItem(hWnd, ID_THMDARK);
    HWND hTL = GetDlgItem(hWnd, ID_THMLIGHT);
    if (hTD) SetWindowPos(hTD, nullptr, cX + PAD,       cY + 130, 110, 32, SWP_NOZORDER|SWP_NOACTIVATE);
    if (hTL) SetWindowPos(hTL, nullptr, cX + PAD + 118, cY + 130, 110, 32, SWP_NOZORDER|SWP_NOACTIVATE);

    // hSum is unused now (stats painted on dashboard) — hide it
    if (hSum) ShowWindow(hSum, SW_HIDE);

    // ── Show/hide controls per page ────────────────────────────────────
    bool onAppliances = (g_page == PAGE_APPLIANCES);
    bool onSettings   = (g_page == PAGE_SETTINGS);

    for (int id : {ID_ADD, ID_EDIT, ID_DEL, ID_EXP})
        if (HWND h = GetDlgItem(hWnd, id)) ShowWindow(h, onAppliances ? SW_SHOW : SW_HIDE);

    if (hList)  ShowWindow(hList,  (g_page == PAGE_APPLIANCES) ? SW_SHOW : SW_HIDE);
    if (hRate)  ShowWindow(hRate,  onSettings ? SW_SHOW : SW_HIDE);
    if (hrb)    ShowWindow(hrb,    onSettings ? SW_SHOW : SW_HIDE);
    if (hrl)    ShowWindow(hrl,    onSettings ? SW_SHOW : SW_HIDE);
    if (hTD)    ShowWindow(hTD,    onSettings ? SW_SHOW : SW_HIDE);
    if (hTL)    ShowWindow(hTL,    onSettings ? SW_SHOW : SW_HIDE);
}
```

**Step 2: Build and verify**

Switching to Appliances shows the list + buttons. Switching to Dashboard/Settings hides them.

**Step 3: Commit**
```bash
git commit -am "refactor: DoLayout supports sidebar + three pages with show/hide"
```

---

## Task 6: Paint page title bar and Dashboard stat cards

**Files:**
- Modify: `power_calculator.cpp` — `WM_PAINT` in `WndProc`

**Step 1: Replace the `WM_PAINT` content-area painting section**

After the `DrawSidebar(hdc, rc.bottom)` call, add:

```cpp
// ── Content area background ────────────────────────────────────────
RECT content = { SIDEBAR_W, 0, W, rc.bottom };
HBRUSH brCont = CreateSolidBrush(T.bg);
FillRect(hdc, &content, brCont);
DeleteObject(brCont);

// ── Page title bar ─────────────────────────────────────────────────
const wchar_t* pageTitle = L"";
if (g_page == PAGE_DASHBOARD)  pageTitle = L"Dashboard";
if (g_page == PAGE_APPLIANCES) pageTitle = L"Appliances";
if (g_page == PAGE_SETTINGS)   pageTitle = L"Settings";

HFONT oldF = (HFONT)SelectObject(hdc, fBold);
SetTextColor(hdc, T.text);
SetBkMode(hdc, TRANSPARENT);
RECT titleRc = { SIDEBAR_W + 16, 0, W, TITLEBAR_H };
DrawTextW(hdc, pageTitle, -1, &titleRc, DT_VCENTER | DT_SINGLELINE | DT_LEFT);
SelectObject(hdc, oldF);

// Thin divider under title
HPEN pen = CreatePen(PS_SOLID, 1, T.border);
HPEN oldP = (HPEN)SelectObject(hdc, pen);
MoveToEx(hdc, SIDEBAR_W, TITLEBAR_H - 1, nullptr);
LineTo(hdc, W, TITLEBAR_H - 1);
SelectObject(hdc, oldP);
DeleteObject(pen);

// ── Dashboard stat cards ───────────────────────────────────────────
if (g_page == PAGE_DASHBOARD) {
    double totKwh = 0, totCday = 0;
    for (auto& a : g_apps) { totKwh += a.kWh(); totCday += a.cDay(); }

    struct Card { const wchar_t* label; wstring value; COLORREF bar; };
    Card cards[] = {
        { L"Daily kWh",    wfmt(totKwh),         T.cardBlue  },
        { L"Daily Cost",   L"$" + wfmt(totCday), T.cardGreen },
        { L"Monthly Cost", L"$" + wfmt(totCday * 30), T.cardGreen },
        { L"CO\u2082 / yr (kg)", wfmt(totKwh * 365 * CO2_FACTOR), T.cardAmber },
    };

    int cx = SIDEBAR_W + 16;
    int cy = TITLEBAR_H + 16;
    int cardW = (W - cx - 16 - 12) / 2;  // two columns
    int cardH = 88;

    for (int i = 0; i < 4; i++) {
        int col = i % 2, row = i / 2;
        int x = cx + col * (cardW + 12);
        int y = cy + row * (cardH + 12);
        RECT cr = { x, y, x + cardW, y + cardH };

        // Card background
        RoundRect2(hdc, cr, 8, T.surface, T.border);

        // Left color bar
        RECT bar = { x, y + 14, x + 4, y + cardH - 14 };
        HBRUSH brBar = CreateSolidBrush(cards[i].bar);
        FillRect(hdc, &bar, brBar);
        DeleteObject(brBar);

        // Label (small, secondary)
        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, fSm);
        SetTextColor(hdc, T.subtext);
        RECT lr = { x + 14, y + 14, x + cardW - 8, y + 34 };
        DrawTextW(hdc, cards[i].label, -1, &lr, DT_LEFT | DT_SINGLELINE);

        // Value (large, bold)
        SelectObject(hdc, fBold);
        SetTextColor(hdc, T.text);
        RECT vr = { x + 14, y + 36, x + cardW - 8, y + cardH - 10 };
        DrawTextW(hdc, cards[i].value.c_str(), -1, &vr,
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    }

    // Top-5 appliances mini table
    if (!g_apps.empty()) {
        int ty = TITLEBAR_H + 16 + 2*(cardH+12) + 16;
        SelectObject(hdc, fSm);
        SetTextColor(hdc, T.subtext);
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, cx, ty, L"TOP APPLIANCES", 14);
        ty += 20;

        // Header row
        HPEN hp = CreatePen(PS_SOLID, 1, T.border);
        HPEN op = (HPEN)SelectObject(hdc, hp);
        MoveToEx(hdc, cx, ty + 18, nullptr);
        LineTo(hdc, W - 16, ty + 18);
        SelectObject(hdc, op); DeleteObject(hp);

        struct Col { const wchar_t* hdr; int x; int w; UINT fmt; };
        int c1 = cx, c2 = W - 200, c3 = W - 110;
        Col cols[] = {
            {L"Appliance", c1, c2-c1-8, DT_LEFT},
            {L"kWh/day",   c2, 80,      DT_RIGHT},
            {L"$/day",     c3, 80,      DT_RIGHT},
        };
        for (auto& c : cols) {
            RECT hr = {c.x, ty, c.x+c.w, ty+18};
            DrawTextW(hdc, c.hdr, -1, &hr, c.fmt|DT_SINGLELINE|DT_VCENTER);
        }
        ty += 22;

        int shown = min((int)g_apps.size(), 5);
        SelectObject(hdc, fUI);
        SetTextColor(hdc, T.text);
        for (int i = 0; i < shown; i++, ty += 22) {
            auto& a = g_apps[i];
            RECT nr={c1,ty,c2-8,ty+20}; DrawTextW(hdc,a.name.c_str(),-1,&nr,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
            wstring kv=wfmt(a.kWh(),3); RECT kr={c2,ty,c2+80,ty+20}; DrawTextW(hdc,kv.c_str(),-1,&kr,DT_RIGHT|DT_SINGLELINE|DT_VCENTER);
            wstring cv=L"$"+wfmt(a.cDay(),4); RECT cr2={c3,ty,c3+80,ty+20}; DrawTextW(hdc,cv.c_str(),-1,&cr2,DT_RIGHT|DT_SINGLELINE|DT_VCENTER);
        }
    }
}

// ── Settings section labels (painted) ─────────────────────────────
if (g_page == PAGE_SETTINGS) {
    int cx = SIDEBAR_W + 16;
    SelectObject(hdc, fSm);
    SetTextColor(hdc, T.subtext);
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, cx, TITLEBAR_H + 16, L"ELECTRICITY RATE", 16);
    TextOutW(hdc, cx, TITLEBAR_H + 110, L"APPEARANCE", 10);
}
```

**Step 2: Remove old `PaintLabel` calls** (the old SUMMARY / RATE / APPLIANCES labels).

**Step 3: Build and verify**

Dashboard should show 4 stat cards. Switching pages should swap content correctly.

**Step 4: Commit**
```bash
git commit -am "feat: paint page title bar and dashboard stat cards"
```

---

## Task 7: Style the Appliances toolbar buttons (new layout)

**Files:**
- Modify: `power_calculator.cpp` — `WM_CREATE` handler, `DrawButton()`

**Step 1: Update `WM_CREATE` button labels and IDs**

Change the button definitions array:
```cpp
struct { const wchar_t* lbl; int id; } btnDefs[] = {
    {L"+ Add",      ID_ADD },
    {L"✎  Edit",   ID_EDIT},
    {L"Remove",     ID_DEL },
    {L"Export CSV", ID_EXP },
};
```
(Already correct — just confirm labels match. The styling comes from `DrawButton`.)

**Step 2: Update `DrawButton()` fill colors to use `T.*`**

```cpp
if (isDanger)
    fill = pre ? T.accentPre : (hov ? T.dangerHov : T.danger);
else if (GetDlgCtrlID(hw) == ID_ADD)
    fill = pre ? T.accentPre : (hov ? T.accent : T.accent);
else
    fill = pre ? T.surfaceHov : (hov ? T.surfaceHov : T.surface);

textClr = (GetDlgCtrlID(hw) == ID_ADD || isDanger) ? RGB(255,255,255) : T.text;
```

**Step 3: Build and verify**

Add button should be accent-colored; Edit/Export should be surface-toned; Remove should be danger-red.

**Step 4: Commit**
```bash
git commit -am "feat: update button colors to use Theme, accent Add, surface Edit/Export"
```

---

## Task 8: Add theme segmented buttons to Settings page

**Files:**
- Modify: `power_calculator.cpp` — add `ID_THMDARK`/`ID_THMLIGHT` to enum, `WM_CREATE`, `WM_COMMAND`, `DrawButton()`

**Step 1: Add IDs to the `enum`**

```cpp
enum {
    ID_LIST = 101,
    ID_ADD, ID_EDIT, ID_DEL, ID_EXP,
    ID_SUM, ID_RATE, ID_RATEBTN, ID_RATELBL, ID_STAT,
    ID_THMDARK, ID_THMLIGHT
};
```

**Step 2: Create the two buttons in `WM_CREATE`**

```cpp
HWND hTD = CreateWindowExW(0, L"BUTTON", L"🌙 Dark",
    WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
    0,0,0,0, hWnd, (HMENU)ID_THMDARK, hi, nullptr);
SendMessage(hTD, WM_SETFONT, (WPARAM)fUI, TRUE);
SetWindowSubclass(hTD, BtnSubclass, 1, 0);

HWND hTL = CreateWindowExW(0, L"BUTTON", L"☀ Light",
    WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
    0,0,0,0, hWnd, (HMENU)ID_THMLIGHT, hi, nullptr);
SendMessage(hTL, WM_SETFONT, (WPARAM)fUI, TRUE);
SetWindowSubclass(hTL, BtnSubclass, 1, 0);
```

**Step 3: Handle clicks in `WM_COMMAND`**

```cpp
case ID_THMDARK:
    if (!g_darkMode) {
        g_darkMode = true; T = DARK; RebuildBrushes();
        ListView_SetBkColor(hList, T.bg);
        ListView_SetTextBkColor(hList, T.bg);
        ListView_SetTextColor(hList, T.text);
        InvalidateRect(hWnd, nullptr, TRUE); UpdateWindow(hWnd);
    }
    break;

case ID_THMLIGHT:
    if (g_darkMode) {
        g_darkMode = false; T = LIGHT; RebuildBrushes();
        ListView_SetBkColor(hList, T.bg);
        ListView_SetTextBkColor(hList, T.bg);
        ListView_SetTextColor(hList, T.text);
        InvalidateRect(hWnd, nullptr, TRUE); UpdateWindow(hWnd);
    }
    break;
```

**Step 4: Update `DrawButton()` to draw active segmented state**

Add special case for theme buttons:
```cpp
bool isThemeBtn = (GetDlgCtrlID(hw)==ID_THMDARK || GetDlgCtrlID(hw)==ID_THMLIGHT);
bool isActive = (GetDlgCtrlID(hw)==ID_THMDARK && g_darkMode)
             || (GetDlgCtrlID(hw)==ID_THMLIGHT && !g_darkMode);
if (isThemeBtn)
    fill = isActive ? T.accent : (hov ? T.surfaceHov : T.surface);
```

**Step 5: Build and test**

Go to Settings, click Light — entire app repaints light. Click Dark — returns to dark. Sidebar toggle also works.

**Step 6: Commit**
```bash
git commit -am "feat: Settings page theme segmented buttons (Dark/Light)"
```

---

## Task 9: Update add/edit dialog to use active theme

**Files:**
- Modify: `power_calculator.cpp` — `DlgProc` color message handlers

**Step 1: Update `DlgProc` `WM_ERASEBKGND`**

```cpp
case WM_ERASEBKGND: {
    HDC hdc = (HDC)wp;
    RECT rc; GetClientRect(hWnd, &rc);
    HBRUSH br = CreateSolidBrush(T.bg);
    FillRect(hdc, &rc, br);
    DeleteObject(br);
    RECT top = { rc.left, rc.top, rc.right, rc.top + 3 };
    HBRUSH brA = CreateSolidBrush(T.accent);
    FillRect(hdc, &top, brA);
    DeleteObject(brA);
    return 1;
}
```

**Step 2: Update `WM_CTLCOLORSTATIC` and `WM_CTLCOLOREDIT` in `DlgProc`**

```cpp
case WM_CTLCOLORSTATIC: {
    HDC hdc = (HDC)wp;
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, T.subtext);
    // Return a brush matching current bg — recreate per call
    // (Store a local static and rebuild when theme changes, or just create/delete)
    static HBRUSH dlgBr = nullptr;
    if (dlgBr) DeleteObject(dlgBr);
    dlgBr = CreateSolidBrush(T.bg);
    return (LRESULT)dlgBr;
}

case WM_CTLCOLOREDIT: {
    HDC hdc = (HDC)wp;
    SetTextColor(hdc, T.text);
    SetBkColor(hdc, T.editBg);
    static HBRUSH dlgEditBr = nullptr;
    if (dlgEditBr) DeleteObject(dlgEditBr);
    dlgEditBr = CreateSolidBrush(T.editBg);
    return (LRESULT)dlgEditBr;
}
```

**Step 3: Build and verify**

Open Add dialog in dark mode — dark background. Switch to light, open dialog — light background.

**Step 4: Commit**
```bash
git commit -am "feat: add/edit dialog picks up active theme colors"
```

---

## Task 10: Update ListView header and row custom-draw colors

**Files:**
- Modify: `power_calculator.cpp` — `InitListCols()`, `WM_NOTIFY` custom-draw handlers

**Step 1: Update `InitListCols()` to set ListView colors from theme**

```cpp
ListView_SetBkColor(hList,     T.bg);
ListView_SetTextBkColor(hList, T.bg);
ListView_SetTextColor(hList,   T.text);
```

**Step 2: Header custom-draw already uses `brSurface` — verify it picks up theme**

`brSurface` is rebuilt by `RebuildBrushes()` so no code change needed; just confirm visually.

**Step 3: Row custom-draw — update to use `T.*`**

```cpp
case CDDS_ITEMPREPAINT:
    cd->clrTextBk = (cd->nmcd.uItemState & CDIS_SELECTED) ? T.rowSel
                  : (cd->nmcd.dwItemSpec % 2 == 0) ? T.rowEven : T.rowOdd;
    cd->clrText = T.text;
    return CDRF_NEWFONT;
```

**Step 4: Build, switch themes, verify ListView updates correctly**

**Step 5: Final commit**
```bash
git commit -am "feat: ListView row/header colors respond to theme switch"
```

---

## Task 11: Window minimum size + final polish

**Files:**
- Modify: `power_calculator.cpp` — `WM_GETMINMAXINFO`, `WinMain` initial size

**Step 1: Update min size**

```cpp
case WM_GETMINMAXINFO: {
    auto* mm = (MINMAXINFO*)lp;
    mm->ptMinTrackSize = {920, 580};
    return 0;
}
```

**Step 2: Update `CreateWindowExW` initial size in `WinMain`**

```cpp
hMain = CreateWindowExW(0, CLS_MAIN, L"⚡ Energy Calculator",
    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
    1024, 640, nullptr, nullptr, hInst, nullptr);
```

**Step 3: Remove now-unused `hSum` create/reference** (`hSum` is hidden permanently in Task 5).

**Step 4: Remove old status-bar-area guard** in DoLayout if it references `hSum`.

**Step 5: Final build**

```bash
"/c/Program Files/CodeBlocks/MinGW/bin/g++.exe" -std=c++17 -O2 power_calculator.cpp \
  -o power_calculator.exe -mwindows -lcomctl32 -luxtheme -ldwmapi 2>&1 | grep -v "pragma comment"
```
Expected: zero warnings, zero errors.

**Step 6: Final commit**
```bash
git commit -am "feat: final polish — min size, initial window size, remove dead hSum refs"
```
