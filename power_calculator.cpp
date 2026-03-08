// ┌─────────────────────────────────────────────────────────────────────────┐
// │  Energy Consumption Calculator — Sleek Dark UI                          │
// └─────────────────────────────────────────────────────────────────────────┘
#define UNICODE
#define _UNICODE
#define WINVER       0x0601
#define _WIN32_WINNT 0x0601
#define _WIN32_IE    0x0600

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <map>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;

// ── colour palette ────────────────────────────────────────────────────────
#define CLR_BG          RGB( 18,  22,  36)   // deep navy background
#define CLR_SURFACE     RGB( 28,  33,  52)   // card/panel surface
#define CLR_SURFACE2    RGB( 36,  42,  64)   // slightly lighter surface
#define CLR_ACCENT      RGB( 99, 102, 241)   // indigo accent
#define CLR_ACCENT_HOV  RGB(124, 108, 255)   // purple hover
#define CLR_ACCENT_PRE  RGB( 79,  70, 229)   // pressed
#define CLR_DANGER      RGB(239,  68,  68)   // red for Remove
#define CLR_DANGER_HOV  RGB(248, 113, 113)
#define CLR_SUCCESS     RGB( 34, 197,  94)   // green (not currently used)
#define CLR_TEXT        RGB(226, 232, 240)   // primary text
#define CLR_SUBTEXT     RGB(148, 163, 184)   // secondary / label text
#define CLR_DIMTEXT     RGB( 71,  85, 105)   // very dim text
#define CLR_ROW_EVEN    RGB( 18,  22,  36)   // same as BG
#define CLR_ROW_ODD     RGB( 24,  29,  48)   // slightly lighter row
#define CLR_ROW_SEL     RGB( 55,  58, 128)   // selected row
#define CLR_BORDER      RGB( 51,  65, 100)   // border/divider
#define CLR_EDIT_BG     RGB( 22,  27,  44)   // edit box background
#define CLR_HDR_BG      RGB( 23,  28,  46)   // list header bg
#define CLR_TOPBAR      RGB( 99, 102, 241)   // 4px accent stripe at top

// ── constants ─────────────────────────────────────────────────────────────
const double    CO2_FACTOR = 0.386;
const wchar_t*  SAVE_FILE  = L"appliances.dat";
const wchar_t*  CLS_MAIN   = L"PowerCalcMain";
const wchar_t*  CLS_DLG    = L"ApplianceDlg";

// ── control IDs ───────────────────────────────────────────────────────────
enum {
    ID_LIST = 101,
    ID_ADD, ID_EDIT, ID_DEL, ID_EXP,
    ID_SUM, ID_RATE, ID_RATEBTN, ID_RATELBL, ID_STAT
};
enum { D_NAME=201, D_WATTS, D_HOURS, D_RATE, D_DFLT };

// ── data model ────────────────────────────────────────────────────────────
struct Appliance {
    wstring name;
    double  watts = 100.0;
    double  hours = 1.0;
    double  rate  = 0.12;

    double kWh()    const { return watts * hours / 1000.0; }
    double cDay()   const { return kWh() * rate; }
    double cMonth() const { return cDay() * 30.0; }
    double cYear()  const { return cDay() * 365.0; }
    double co2Yr()  const { return kWh() * 365.0 * CO2_FACTOR; }
};

static vector<Appliance> g_apps;
static double             g_rate = 0.12;

// ── globals ───────────────────────────────────────────────────────────────
static HWND  hMain, hList, hSum, hRate, hStat;
static HFONT fUI  = nullptr;
static HFONT fBold= nullptr;
static HFONT fSm  = nullptr;

// GDI resource cache
static HBRUSH brBg      = nullptr;
static HBRUSH brSurface = nullptr;
static HBRUSH brSurface2= nullptr;
static HBRUSH brAccent  = nullptr;
static HBRUSH brEditBg  = nullptr;
static HBRUSH brBorder  = nullptr;

// Hover tracking for owner-draw buttons (maps HWND → bool)
static map<HWND,bool> g_hover;
static map<HWND,bool> g_press;

// ── string helpers ────────────────────────────────────────────────────────
static wstring wfmt(double v, int dp = 2) {
    wostringstream s;
    s << fixed << setprecision(dp) << v;
    return s.str();
}

static string toUTF8(const wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    string r(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &r[0], n, nullptr, nullptr);
    while (!r.empty() && r.back() == '\0') r.pop_back();
    return r;
}

static wstring fromUTF8(const char* s) {
    if (!s || !*s) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    wstring r(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &r[0], n);
    while (!r.empty() && r.back() == L'\0') r.pop_back();
    return r;
}

// ── save / load ───────────────────────────────────────────────────────────
static void SaveData() {
    FILE* f = _wfopen(SAVE_FILE, L"w");
    if (!f) return;
    fprintf(f, "%.6f\n", g_rate);
    for (auto& a : g_apps)
        fprintf(f, "%s|%.4f|%.4f|%.4f\n",
                toUTF8(a.name).c_str(), a.watts, a.hours, a.rate);
    fclose(f);
}

static void LoadData() {
    FILE* f = _wfopen(SAVE_FILE, L"r");
    if (!f) return;
    char ln[512];
    if (fgets(ln, sizeof(ln), f)) {
        g_rate = atof(ln);
        if (g_rate <= 0) g_rate = 0.12;
    }
    g_apps.clear();
    while (fgets(ln, sizeof(ln), f)) {
        size_t l = strlen(ln);
        while (l && (ln[l-1] == '\n' || ln[l-1] == '\r')) ln[--l] = '\0';
        if (!l) continue;
        char *p1 = strchr(ln, '|');
        char *p2 = p1 ? strchr(p1+1, '|') : nullptr;
        char *p3 = p2 ? strchr(p2+1, '|') : nullptr;
        if (!p3) continue;
        *p1 = *p2 = *p3 = '\0';
        Appliance a;
        a.name  = fromUTF8(ln);
        a.watts = atof(p1 + 1);
        a.hours = atof(p2 + 1);
        a.rate  = atof(p3 + 1);
        if (a.watts > 0 && a.hours >= 0 && a.hours <= 24 && a.rate > 0)
            g_apps.push_back(a);
    }
    fclose(f);
}

// ── GDI helpers ───────────────────────────────────────────────────────────
static void RoundRect2(HDC hdc, RECT r, int rx, COLORREF fill, COLORREF border = CLR_BORDER) {
    HBRUSH hFill = CreateSolidBrush(fill);
    HPEN   hPen  = (border == (COLORREF)-1) ? (HPEN)GetStockObject(NULL_PEN) : CreatePen(PS_SOLID, 1, border);
    SelectObject(hdc, hFill); SelectObject(hdc, hPen);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rx, rx);
    DeleteObject(hFill); DeleteObject(hPen);
}

static void DrawAccentLine(HDC hdc, int x, int y, int w, int h) {
    // Horizontal gradient-ish line using GDI gradient fill
    TRIVERTEX tv[2] = {
        {x,   y,   (COLOR16)(GetRValue(CLR_ACCENT)   << 8), (COLOR16)(GetGValue(CLR_ACCENT)   << 8), (COLOR16)(GetBValue(CLR_ACCENT)   << 8), 0},
        {x+w, y+h, (COLOR16)(GetRValue(CLR_ACCENT_HOV)<< 8), (COLOR16)(GetGValue(CLR_ACCENT_HOV)<< 8), (COLOR16)(GetBValue(CLR_ACCENT_HOV)<< 8), 0},
    };
    GRADIENT_RECT gr = {0, 1};
    GdiGradientFill(hdc, tv, 2, &gr, 1, GRADIENT_FILL_RECT_H);
}

// ── UI refresh ────────────────────────────────────────────────────────────
static void RefreshList() {
    ListView_DeleteAllItems(hList);
    for (int i = 0; i < (int)g_apps.size(); i++) {
        auto& a = g_apps[i];
        LVITEMW li = {};
        li.mask  = LVIF_TEXT;
        li.iItem = i;
        wstring idx = to_wstring(i + 1);
        li.pszText = (LPWSTR)idx.c_str();
        ListView_InsertItem(hList, &li);
        ListView_SetItemText(hList, i, 1, (LPWSTR)a.name.c_str());
        wstring w = wfmt(a.watts, 0);          ListView_SetItemText(hList, i, 2, (LPWSTR)w.c_str());
        wstring h = wfmt(a.hours, 1);          ListView_SetItemText(hList, i, 3, (LPWSTR)h.c_str());
        wstring k = wfmt(a.kWh(),  3);         ListView_SetItemText(hList, i, 4, (LPWSTR)k.c_str());
        wstring c = L"$" + wfmt(a.cDay(), 4); ListView_SetItemText(hList, i, 5, (LPWSTR)c.c_str());
    }
}

static void RefreshSummary() {
    int n = (int)g_apps.size();
    if (hStat) {
        wstring s = to_wstring(n) + L" appliance" + (n != 1 ? L"s" : L"");
        SetWindowTextW(hStat, s.c_str());
    }
    if (!hSum) return;
    if (!n) {
        SetWindowTextW(hSum,
            L"  No appliances yet.\r\n"
            L"  Click '+ Add' to begin tracking your energy use.");
        return;
    }
    double k = 0, c = 0;
    for (auto& a : g_apps) { k += a.kWh(); c += a.cDay(); }
    wostringstream ss;
    ss << L"  Daily      " << wfmt(k)       << L" kWh   $" << wfmt(c)       << L"\r\n"
       << L"  Monthly    " << wfmt(k * 30)  << L" kWh   $" << wfmt(c * 30)  << L"\r\n"
       << L"  Yearly     " << wfmt(k * 365) << L" kWh   $" << wfmt(c * 365) << L"\r\n"
       << L"  CO\u2082 / yr    " << wfmt(k * 365 * CO2_FACTOR) << L" kg";
    SetWindowTextW(hSum, ss.str().c_str());
}

// ── layout ────────────────────────────────────────────────────────────────
static void DoLayout(HWND hWnd) {
    RECT rc; GetClientRect(hWnd, &rc);
    int W = rc.right, H = rc.bottom;

    // Status bar
    if (hStat) SendMessage(hStat, WM_SIZE, 0, 0);
    int SH = 24;
    if (hStat) { RECT sr; GetClientRect(hStat, &sr); SH = (sr.bottom > 0) ? sr.bottom : 24; }

    int UH = H - SH;
    int PAD = 14;

    // ListView: top section, leaves 172px for controls below
    int listH = UH - 172;
    if (listH < 120) listH = 120;
    if (hList) SetWindowPos(hList, nullptr, PAD, 28, W - PAD*2, listH, SWP_NOZORDER | SWP_NOACTIVATE);

    // Button row
    int btnY = listH + 38;
    int BW = 96, BH = 34;
    struct { int id, x; } btns[] = {{ID_ADD,PAD},{ID_EDIT,PAD+BW+8},{ID_DEL,PAD+BW*2+16},{ID_EXP,PAD+BW*3+24}};
    for (auto& b : btns) {
        HWND h = GetDlgItem(hWnd, b.id);
        if (h) SetWindowPos(h, nullptr, b.x, btnY, BW, BH, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Summary and rate panel
    int sumY  = btnY + BH + 14;
    int sumW  = (W - PAD*3) * 3 / 5;
    int sumH  = UH - sumY - 8;
    if (sumH < 68) sumH = 68;
    if (hSum) SetWindowPos(hSum, nullptr, PAD, sumY, sumW, sumH, SWP_NOZORDER | SWP_NOACTIVATE);

    // Rate panel (right of summary)
    int rx = PAD + sumW + PAD;
    int rw = W - rx - PAD;
    if (rw < 90) rw = 90;
    HWND hrl = GetDlgItem(hWnd, ID_RATELBL);
    HWND hrb = GetDlgItem(hWnd, ID_RATEBTN);
    if (hrl)   SetWindowPos(hrl,   nullptr, rx, sumY,      rw,      20, SWP_NOZORDER | SWP_NOACTIVATE);
    if (hRate) SetWindowPos(hRate, nullptr, rx, sumY + 28, rw,      30, SWP_NOZORDER | SWP_NOACTIVATE);
    if (hrb)   SetWindowPos(hrb,   nullptr, rx, sumY + 68, rw,      34, SWP_NOZORDER | SWP_NOACTIVATE);
}

// ── owner-draw button helper ──────────────────────────────────────────────
static void DrawButton(DRAWITEMSTRUCT* di) {
    HDC     hdc  = di->hDC;
    RECT    rc   = di->rcItem;
    HWND    hw   = di->hwndItem;
    bool    hov  = g_hover.count(hw) && g_hover[hw];
    bool    pre  = (di->itemState & ODS_SELECTED) != 0;

    // Determine colours
    wchar_t txt[128] = {};
    GetWindowTextW(hw, txt, 128);
    bool isDanger = (GetDlgCtrlID(hw) == ID_DEL);

    COLORREF fill, textClr = CLR_TEXT;
    if (isDanger)
        fill = pre ? CLR_DANGER : (hov ? CLR_DANGER_HOV : CLR_DANGER);
    else
        fill = pre ? CLR_ACCENT_PRE : (hov ? CLR_ACCENT_HOV : CLR_ACCENT);

    // Rounded rect background
    SetBkMode(hdc, TRANSPARENT);
    RoundRect2(hdc, rc, 8, fill, (COLORREF)-1);

    // Text
    int ox = pre ? 1 : 0, oy = pre ? 1 : 0;
    RECT tr = {rc.left+ox, rc.top+oy, rc.right+ox, rc.bottom+oy};
    HFONT old = (HFONT)SelectObject(hdc, fBold);
    SetTextColor(hdc, textClr);
    DrawTextW(hdc, txt, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);

    // Focus rect (skip the dotted Windows default)
    if (di->itemState & ODS_FOCUS) {
        RECT fr = { rc.left+3, rc.top+3, rc.right-3, rc.bottom-3 };
        HPEN pen = CreatePen(PS_DOT, 1, RGB(200,200,255));
        HPEN old2 = (HPEN)SelectObject(hdc, pen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc, fr.left, fr.top, fr.right, fr.bottom, 6, 6);
        SelectObject(hdc, old2);
        DeleteObject(pen);
    }
}

// ── subclass for hover tracking ───────────────────────────────────────────
static LRESULT CALLBACK BtnSubclass(HWND hw, UINT msg, WPARAM wp, LPARAM lp,
                                     UINT_PTR, DWORD_PTR) {
    switch (msg) {
    case WM_MOUSEMOVE:
        if (!g_hover[hw]) {
            g_hover[hw] = true;
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hw, 0 };
            TrackMouseEvent(&tme);
            InvalidateRect(hw, nullptr, FALSE);
        }
        break;
    case WM_MOUSELEAVE:
        g_hover[hw] = false;
        InvalidateRect(hw, nullptr, FALSE);
        break;
    }
    return DefSubclassProc(hw, msg, wp, lp);
}

// ── add/edit dialog ───────────────────────────────────────────────────────
struct DlgData { Appliance* target; bool saved; };
static HWND     dDlg = nullptr;
static HWND     dName, dWatts, dHours, dRate, dDflt;
static DlgData* dData = nullptr;

// Subclass for dialog edit boxes → dark background
static WNDPROC g_origEditProc = nullptr;

static LRESULT CALLBACK DlgEditSubclass(HWND hw, UINT msg, WPARAM wp, LPARAM lp,
                                        UINT_PTR, DWORD_PTR) {
    return DefSubclassProc(hw, msg, wp, lp);
}

static HWND dMakeLabel(HWND p, const wchar_t* t, int x, int y, int w = 110) {
    HWND h = CreateWindowExW(0, L"STATIC", t, WS_CHILD | WS_VISIBLE | SS_LEFT,
                             x, y, w, 20, p, nullptr,
                             (HINSTANCE)GetWindowLongPtrW(p, GWLP_HINSTANCE), nullptr);
    if (fSm) SendMessage(h, WM_SETFONT, (WPARAM)fSm, TRUE);
    return h;
}

static HWND dMakeEdit(HWND p, int id, const wchar_t* v, int x, int y, int w = 200) {
    HWND h = CreateWindowExW(0, L"EDIT", v,
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                             x, y, w, 30, p, (HMENU)(INT_PTR)id,
                             (HINSTANCE)GetWindowLongPtrW(p, GWLP_HINSTANCE), nullptr);
    if (fUI) SendMessage(h, WM_SETFONT, (WPARAM)fUI, TRUE);
    return h;
}

static HWND dMakeBtn(HWND p, int id, const wchar_t* t, DWORD sty, int x, int y, int w, int h) {
    HWND hw = CreateWindowExW(0, L"BUTTON", t, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | sty,
                              x, y, w, h, p, (HMENU)(INT_PTR)id,
                              (HINSTANCE)GetWindowLongPtrW(p, GWLP_HINSTANCE), nullptr);
    if (fBold) SendMessage(hw, WM_SETFONT, (WPARAM)fBold, TRUE);
    SetWindowSubclass(hw, BtnSubclass, 1, 0);
    return hw;
}

static HWND dMakeCheck(HWND p, int id, const wchar_t* t, int x, int y, int w) {
    HWND hw = CreateWindowExW(0, L"BUTTON", t,
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                              x, y, w, 22, p, (HMENU)(INT_PTR)id,
                              (HINSTANCE)GetWindowLongPtrW(p, GWLP_HINSTANCE), nullptr);
    if (fUI) SendMessage(hw, WM_SETFONT, (WPARAM)fUI, TRUE);
    return hw;
}

LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        dData = (DlgData*)((CREATESTRUCT*)lp)->lpCreateParams;
        int lx = 20, ex = 138, row = 20, ew = 190;

        dMakeLabel(hWnd, L"Name",         lx, row + 5);
        dName  = dMakeEdit(hWnd, D_NAME,  L"", ex, row, ew); row += 50;

        dMakeLabel(hWnd, L"Power (W)",    lx, row + 5);
        dWatts = dMakeEdit(hWnd, D_WATTS, L"", ex, row, ew); row += 50;

        dMakeLabel(hWnd, L"Hrs / Day",    lx, row + 5);
        dHours = dMakeEdit(hWnd, D_HOURS, L"", ex, row, ew); row += 50;

        dMakeLabel(hWnd, L"Rate ($/kWh)", lx, row + 5);
        dRate  = dMakeEdit(hWnd, D_RATE,  wfmt(g_rate, 4).c_str(), ex, row, ew); row += 46;

        dDflt  = dMakeCheck(hWnd, D_DFLT, L"Use default rate", ex, row, ew); row += 34;
        SendMessage(dDflt, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(dRate, FALSE);

        dMakeBtn(hWnd, IDOK,     L"Save",   BS_DEFPUSHBUTTON, ex,       row, 92, 34);
        dMakeBtn(hWnd, IDCANCEL, L"Cancel", BS_PUSHBUTTON,    ex + 100, row, 92, 34);

        if (dData && dData->target) {
            Appliance* a = dData->target;
            SetWindowTextW(dName,  a->name.c_str());
            SetWindowTextW(dWatts, wfmt(a->watts, 0).c_str());
            SetWindowTextW(dHours, wfmt(a->hours, 1).c_str());
            SetWindowTextW(dRate,  wfmt(a->rate,  4).c_str());
            SendMessage(dDflt, BM_SETCHECK, BST_UNCHECKED, 0);
            EnableWindow(dRate, TRUE);
        }
        SetFocus(dName);
        return 0;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wp;
        RECT rc; GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, brBg);
        // Thin top accent line
        RECT top = { rc.left, rc.top, rc.right, rc.top + 3 };
        FillRect(hdc, &top, brAccent);
        return 1;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        SetBkMode(hdc, TRANSPARENT);
        // Checkbox
        if ((HWND)lp == dDflt) {
            SetTextColor(hdc, CLR_SUBTEXT);
            return (LRESULT)brBg;
        }
        SetTextColor(hdc, CLR_SUBTEXT);
        return (LRESULT)brBg;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, CLR_TEXT);
        SetBkColor(hdc, (COLORREF)CLR_EDIT_BG);
        return (LRESULT)brEditBg;
    }

    case WM_DRAWITEM: {
        DrawButton((DRAWITEMSTRUCT*)lp);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case D_DFLT: {
            bool chk = SendMessage(dDflt, BM_GETCHECK, 0, 0) == BST_CHECKED;
            EnableWindow(dRate, !chk);
            if (chk) SetWindowTextW(dRate, wfmt(g_rate, 4).c_str());
            break;
        }
        case IDOK: {
            wchar_t buf[256];
            GetWindowTextW(dName, buf, 256);
            if (!buf[0]) {
                MessageBoxW(hWnd, L"Name cannot be empty.", L"Validation", MB_OK | MB_ICONWARNING);
                SetFocus(dName); break;
            }
            wstring name(buf);
            GetWindowTextW(dWatts, buf, 256);
            double watts = _wtof(buf);
            if (watts <= 0) {
                MessageBoxW(hWnd, L"Power must be > 0 W.", L"Validation", MB_OK | MB_ICONWARNING);
                SetFocus(dWatts); break;
            }
            GetWindowTextW(dHours, buf, 256);
            double hours = _wtof(buf);
            if (hours < 0 || hours > 24) {
                MessageBoxW(hWnd, L"Hours must be between 0 and 24.", L"Validation", MB_OK | MB_ICONWARNING);
                SetFocus(dHours); break;
            }
            bool useDefault = SendMessage(dDflt, BM_GETCHECK, 0, 0) == BST_CHECKED;
            double rate = g_rate;
            if (!useDefault) {
                GetWindowTextW(dRate, buf, 64);
                rate = _wtof(buf);
                if (rate <= 0) {
                    MessageBoxW(hWnd, L"Rate must be > 0.", L"Validation", MB_OK | MB_ICONWARNING);
                    SetFocus(dRate); break;
                }
            }
            if (dData->target) {
                dData->target->name  = name;
                dData->target->watts = watts;
                dData->target->hours = hours;
                dData->target->rate  = rate;
            } else {
                Appliance a;
                a.name = name; a.watts = watts; a.hours = hours; a.rate = rate;
                g_apps.push_back(a);
            }
            dData->saved = true;
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
            break;
        }
        case IDCANCEL:
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
            break;
        }
        return 0;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        dDlg = nullptr;
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}

static void ShowAppDlg(Appliance* target) {
    if (dDlg) { SetForegroundWindow(dDlg); return; }

    static DlgData data;
    data.target = target;
    data.saved  = false;

    RECT rc; GetWindowRect(hMain, &rc);
    int W = 352, H = 348;
    int dx = (rc.left + rc.right  - W) / 2;
    int dy = (rc.top  + rc.bottom - H) / 2;

    dDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        CLS_DLG, target ? L"Edit Appliance" : L"Add Appliance",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        dx, dy, W, H,
        hMain, nullptr, GetModuleHandleW(nullptr), &data);

    EnableWindow(hMain, FALSE);
    ShowWindow(dDlg, SW_SHOW);
    UpdateWindow(dDlg);

    MSG msg;
    while (dDlg && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(dDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(hMain, TRUE);
    SetForegroundWindow(hMain);

    if (data.saved) {
        RefreshList();
        RefreshSummary();
        SaveData();
    }
}

// ── main window ───────────────────────────────────────────────────────────
static void InitListCols() {
    struct { const wchar_t* name; int w; int fmt; } cols[] = {
        {L"#",        40, LVCFMT_CENTER},
        {L"Name",    188, LVCFMT_LEFT  },
        {L"Watts",    72, LVCFMT_RIGHT },
        {L"Hrs/Day",  72, LVCFMT_RIGHT },
        {L"kWh/Day",  80, LVCFMT_RIGHT },
        {L"$/Day",    88, LVCFMT_RIGHT },
    };
    for (int i = 0; i < 6; i++) {
        LVCOLUMNW c = {};
        c.mask    = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        c.pszText = (LPWSTR)cols[i].name;
        c.cx      = cols[i].w;
        c.fmt     = cols[i].fmt;
        ListView_InsertColumn(hList, i, &c);
    }
    ListView_SetExtendedListViewStyle(hList,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    ListView_SetBkColor(hList,     CLR_BG);
    ListView_SetTextBkColor(hList, CLR_BG);
    ListView_SetTextColor(hList,   CLR_TEXT);

    // Remove theme so we can custom-draw header
    SetWindowTheme(hList, L"", L"");
    HWND hHdr = ListView_GetHeader(hList);
    if (hHdr) SetWindowTheme(hHdr, L"", L"");
}

// ── main window fonts ─────────────────────────────────────────────────────
static void CreateFonts() {
    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    // UI font  – Segoe UI 10
    fUI = CreateFontW(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    // Bold font – Segoe UI 10 Bold
    fBold = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    // Small font – Segoe UI 9
    fSm = CreateFontW(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    if (!fUI)   fUI   = CreateFontIndirectW(&ncm.lfMessageFont);
    if (!fBold) fBold = fUI;
    if (!fSm)   fSm   = fUI;
}

static void CreateBrushes() {
    brBg       = CreateSolidBrush(CLR_BG);
    brSurface  = CreateSolidBrush(CLR_SURFACE);
    brSurface2 = CreateSolidBrush(CLR_SURFACE2);
    brAccent   = CreateSolidBrush(CLR_ACCENT);
    brEditBg   = CreateSolidBrush(CLR_EDIT_BG);
    brBorder   = CreateSolidBrush(CLR_BORDER);
}

static void DestroyBrushes() {
    DeleteObject(brBg);      DeleteObject(brSurface);
    DeleteObject(brSurface2);DeleteObject(brAccent);
    DeleteObject(brEditBg);  DeleteObject(brBorder);
}

// ── paint section label ───────────────────────────────────────────────────
static void PaintLabel(HDC hdc, HFONT font, const wchar_t* txt, int x, int y) {
    HFONT old = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, CLR_SUBTEXT);
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, x, y, txt, (int)wcslen(txt));
    SelectObject(hdc, old);
}

// ── WndProc ───────────────────────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    case WM_CREATE: {
        CreateFonts();
        CreateBrushes();

        HINSTANCE hi = ((CREATESTRUCT*)lp)->hInstance;

        // ListView (no client-edge so we paint our own border)
        hList = CreateWindowExW(0, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
            14, 28, 740, 280, hWnd, (HMENU)ID_LIST, hi, nullptr);
        SendMessage(hList, WM_SETFONT, (WPARAM)fUI, TRUE);
        InitListCols();

        // Owner-draw action buttons
        struct { const wchar_t* lbl; int id; } btnDefs[] = {
            {L"+ Add", ID_ADD}, {L"✎  Edit", ID_EDIT}, {L"Remove", ID_DEL}, {L"Export CSV", ID_EXP}
        };
        for (int i = 0; i < 4; i++) {
            HWND h = CreateWindowExW(0, L"BUTTON", btnDefs[i].lbl,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                14 + 104*i, 320, 96, 34, hWnd, (HMENU)(INT_PTR)btnDefs[i].id, hi, nullptr);
            SendMessage(h, WM_SETFONT, (WPARAM)fBold, TRUE);
            SetWindowSubclass(h, BtnSubclass, 1, 0);
        }

        // Summary (read-only multiline edit, no border — styled via WM_CTLCOLOREDIT)
        hSum = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY,
            14, 370, 440, 88, hWnd, (HMENU)ID_SUM, hi, nullptr);
        SendMessage(hSum, WM_SETFONT, (WPARAM)fUI, TRUE);

        // Rate label
        HWND hrl = CreateWindowExW(0, L"STATIC", L"Default rate ($/kWh)",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            468, 370, 200, 20, hWnd, (HMENU)ID_RATELBL, hi, nullptr);
        SendMessage(hrl, WM_SETFONT, (WPARAM)fSm, TRUE);

        // Rate edit
        hRate = CreateWindowExW(0, L"EDIT", L"0.1200",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            468, 398, 150, 30, hWnd, (HMENU)ID_RATE, hi, nullptr);
        SendMessage(hRate, WM_SETFONT, (WPARAM)fUI, TRUE);

        // Rate update button (owner-draw)
        HWND hrb = CreateWindowExW(0, L"BUTTON", L"Update",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            468, 436, 150, 34, hWnd, (HMENU)ID_RATEBTN, hi, nullptr);
        SendMessage(hrb, WM_SETFONT, (WPARAM)fBold, TRUE);
        SetWindowSubclass(hrb, BtnSubclass, 1, 0);

        // Status bar
        hStat = CreateWindowExW(0, L"msctls_statusbar32", L"",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hWnd, (HMENU)ID_STAT, hi, nullptr);
        // Theme-strip status bar so background can be controlled
        SetWindowTheme(hStat, L"", L"");

        LoadData();
        SetWindowTextW(hRate, wfmt(g_rate, 4).c_str());
        RefreshList();
        RefreshSummary();
        return 0;
    }

    case WM_SIZE:
        DoLayout(hWnd);
        InvalidateRect(hWnd, nullptr, TRUE);
        return 0;

    case WM_GETMINMAXINFO: {
        auto* mm = (MINMAXINFO*)lp;
        mm->ptMinTrackSize = {580, 480};
        return 0;
    }

    case WM_ERASEBKGND: {
        // Paint entire background dark
        HDC hdc = (HDC)wp;
        RECT rc; GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, brBg);
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc; GetClientRect(hWnd, &rc);
        int W = rc.right;

        // Top accent stripe
        DrawAccentLine(hdc, 0, 0, W, 4);

        // Section labels
        PaintLabel(hdc, fSm, L"APPLIANCES",      14, 10);
        PaintLabel(hdc, fSm, L"SUMMARY",         14,
                   /* sumY label */ [&] {
                       RECT lr; GetWindowRect(hSum, &lr);
                       POINT p = {lr.left, lr.top};
                       ScreenToClient(hWnd, &p);
                       return p.y - 16;
                   }());
        PaintLabel(hdc, fSm, L"RATE",
                   [&] {
                       RECT lr; GetWindowRect(hRate, &lr);
                       POINT p = {lr.left, lr.top};
                       ScreenToClient(hWnd, &p);
                       return p.x;
                   }(),
                   [&] {
                       RECT lr; GetWindowRect(hSum, &lr);
                       POINT p = {lr.left, lr.top};
                       ScreenToClient(hWnd, &p);
                       return p.y - 16;
                   }());

        // Paint summary panel background
        if (hSum) {
            RECT sr; GetWindowRect(hSum, &sr);
            POINT p0 = {sr.left, sr.top}, p1 = {sr.right, sr.bottom};
            ScreenToClient(hWnd, &p0); ScreenToClient(hWnd, &p1);
            RECT sumRc = { p0.x - 2, p0.y - 2, p1.x + 2, p1.y + 2 };
            // Rounded surface card
            RoundRect2(hdc, sumRc, 8, CLR_SURFACE, CLR_BORDER);
        }

        // Paint rate card
        if (hRate) {
            RECT rr; GetWindowRect(hRate, &rr);
            POINT p0 = {rr.left, rr.top}, p1 = {rr.right, rr.bottom};
            ScreenToClient(hWnd, &p0); ScreenToClient(hWnd, &p1);
            // Rate edit box border
            RECT erRc = { p0.x - 2, p0.y - 2, p1.x + 2, p1.y + 2 };
            RoundRect2(hdc, erRc, 6, CLR_EDIT_BG, CLR_BORDER);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    // Route colour messages for static labels and edits
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, CLR_SUBTEXT);
        return (LRESULT)brBg;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, CLR_TEXT);
        SetBkColor(hdc, CLR_EDIT_BG);
        return (LRESULT)brEditBg;
    }

    case WM_DRAWITEM: {
        DrawButton((DRAWITEMSTRUCT*)lp);
        return TRUE;
    }

    case WM_NOTIFY: {
        auto* nm = (NMHDR*)lp;
        if (nm->idFrom == ID_LIST) {
            // Double-click to edit
            if (nm->code == NM_DBLCLK) {
                int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                if (i >= 0) ShowAppDlg(&g_apps[i]);
                return 0;
            }
            // Custom-draw rows
            if (nm->code == NM_CUSTOMDRAW) {
                auto* cd = (NMLVCUSTOMDRAW*)lp;
                switch (cd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT:
                    if (cd->nmcd.uItemState & CDIS_SELECTED) {
                        cd->clrTextBk = CLR_ROW_SEL;
                        cd->clrText   = CLR_TEXT;
                    } else {
                        cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0)
                            ? CLR_ROW_EVEN : CLR_ROW_ODD;
                        cd->clrText   = CLR_TEXT;
                    }
                    return CDRF_NEWFONT;
                }
                return CDRF_DODEFAULT;
            }
            // Custom-draw header
            if (nm->hwndFrom == ListView_GetHeader(hList)) {
                if (nm->code == NM_CUSTOMDRAW) {
                    auto* cd = (NMCUSTOMDRAW*)lp;
                    switch (cd->dwDrawStage) {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;
                    case CDDS_ITEMPREPAINT: {
                        // Fill header item background
                        FillRect(cd->hdc, &cd->rc, brSurface);
                        // Get header text
                        HDITEMW hdi = {};
                        wchar_t htxt[64] = {};
                        hdi.mask       = HDI_TEXT | HDI_FORMAT;
                        hdi.pszText    = htxt;
                        hdi.cchTextMax = 64;
                        Header_GetItem(nm->hwndFrom, (int)cd->dwItemSpec, &hdi);
                        // Draw text
                        SetTextColor(cd->hdc, CLR_SUBTEXT);
                        SetBkMode(cd->hdc, TRANSPARENT);
                        HFONT oldf = (HFONT)SelectObject(cd->hdc, fSm);
                        RECT tr = cd->rc;
                        InflateRect(&tr, -6, 0);
                        UINT dtFmt = DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
                        if (hdi.fmt & HDF_RIGHT)       dtFmt |= DT_RIGHT;
                        else if (hdi.fmt & HDF_CENTER) dtFmt |= DT_CENTER;
                        else                            dtFmt |= DT_LEFT;
                        DrawTextW(cd->hdc, htxt, -1, &tr, dtFmt);
                        SelectObject(cd->hdc, oldf);
                        // Bottom divider
                        HPEN pen = CreatePen(PS_SOLID, 1, CLR_BORDER);
                        HPEN op  = (HPEN)SelectObject(cd->hdc, pen);
                        MoveToEx(cd->hdc, cd->rc.left,  cd->rc.bottom - 1, nullptr);
                        LineTo  (cd->hdc, cd->rc.right, cd->rc.bottom - 1);
                        SelectObject(cd->hdc, op);
                        DeleteObject(pen);
                        return CDRF_SKIPDEFAULT;
                    }}
                    return CDRF_DODEFAULT;
                }
            }
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {

        case ID_ADD:
            ShowAppDlg(nullptr);
            break;

        case ID_EDIT: {
            int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (i < 0) {
                MessageBoxW(hWnd, L"Select an appliance to edit.", L"Edit",
                            MB_OK | MB_ICONINFORMATION);
                break;
            }
            ShowAppDlg(&g_apps[i]);
            break;
        }

        case ID_DEL: {
            int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (i < 0) {
                MessageBoxW(hWnd, L"Select an appliance to remove.", L"Remove",
                            MB_OK | MB_ICONINFORMATION);
                break;
            }
            wstring m = L"Remove \"" + g_apps[i].name + L"\"?";
            if (MessageBoxW(hWnd, m.c_str(), L"Confirm", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                g_apps.erase(g_apps.begin() + i);
                RefreshList();
                RefreshSummary();
                SaveData();
            }
            break;
        }

        case ID_EXP: {
            if (g_apps.empty()) {
                MessageBoxW(hWnd, L"No appliances to export.", L"Export",
                            MB_OK | MB_ICONINFORMATION);
                break;
            }
            time_t now = time(nullptr);
            char buf[64];
            strftime(buf, sizeof(buf), "energy_report_%Y%m%d_%H%M%S.csv", localtime(&now));
            FILE* f = fopen(buf, "w");
            if (!f) {
                MessageBoxW(hWnd, L"Could not create file.", L"Export Error",
                            MB_OK | MB_ICONERROR);
                break;
            }
            fprintf(f, "name,watts,hrs_per_day,rate_usd_per_kwh,"
                       "daily_kwh,daily_usd,monthly_usd,yearly_usd,yearly_co2_kg\n");
            for (auto& a : g_apps)
                fprintf(f, "%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
                        toUTF8(a.name).c_str(),
                        a.watts, a.hours, a.rate,
                        a.kWh(), a.cDay(), a.cMonth(), a.cYear(), a.co2Yr());
            fclose(f);
            wchar_t wbuf[64];
            MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, 64);
            MessageBoxW(hWnd, (wstring(L"Saved: ") + wbuf).c_str(),
                        L"Export Complete", MB_OK | MB_ICONINFORMATION);
            break;
        }

        case ID_RATEBTN: {
            wchar_t buf[64];
            GetWindowTextW(hRate, buf, 64);
            double r = _wtof(buf);
            if (r <= 0) {
                MessageBoxW(hWnd, L"Rate must be greater than 0.", L"Invalid Rate",
                            MB_OK | MB_ICONWARNING);
                break;
            }
            g_rate = r;
            SaveData();
            MessageBoxW(hWnd, L"Default rate updated.", L"Updated",
                        MB_OK | MB_ICONINFORMATION);
            break;
        }
        }
        return 0;

    case WM_KEYDOWN:
        if (wp == VK_DELETE) {
            int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (i >= 0) SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_DEL, 0), 0);
        }
        return 0;

    case WM_DESTROY:
        SaveData();
        if (fUI)   DeleteObject(fUI);
        if (fBold && fBold != fUI) DeleteObject(fBold);
        if (fSm   && fSm   != fUI) DeleteObject(fSm);
        DestroyBrushes();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}

// ── entry point ───────────────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    // Enable modern common controls (comctl32 v6)
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    // Register dialog window class
    {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = DlgProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = CLS_DLG;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);
    }

    // Register main window class — use NULL background (we paint in WM_ERASEBKGND)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = CLS_MAIN;
        wc.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;   // we paint in WM_ERASEBKGND
        RegisterClassExW(&wc);
    }

    hMain = CreateWindowExW(
        0, CLS_MAIN, L"⚡ Energy Calculator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 840, 560,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(hMain, nShow);
    UpdateWindow(hMain);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
