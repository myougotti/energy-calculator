#define UNICODE
#define _UNICODE
#define WINVER       0x0601
#define _WIN32_WINNT 0x0601
#define _WIN32_IE    0x0600

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <ctime>

using namespace std;

// ── constants ────────────────────────────────────────────
const double    CO2_FACTOR = 0.386;
const wchar_t*  SAVE_FILE  = L"appliances.dat";
const wchar_t*  CLS_MAIN   = L"PowerCalcMain";
const wchar_t*  CLS_DLG    = L"ApplianceDlg";

// ── control IDs ──────────────────────────────────────────
enum {
    ID_LIST = 101,
    ID_ADD, ID_EDIT, ID_DEL, ID_EXP,
    ID_SUM, ID_RATE, ID_RATEBTN, ID_RATELBL, ID_STAT
};
enum { D_NAME=201, D_WATTS, D_HOURS, D_RATE, D_DFLT };

// ── data model ───────────────────────────────────────────
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

// ── globals ───────────────────────────────────────────────
static HWND  hMain, hList, hSum, hRate, hStat;
static HFONT fUI = nullptr;

// ── string helpers ────────────────────────────────────────
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

// ── save / load ───────────────────────────────────────────
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

// ── UI refresh ────────────────────────────────────────────
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
        wstring c = L"$" + wfmt(a.cDay(), 4);  ListView_SetItemText(hList, i, 5, (LPWSTR)c.c_str());
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
       << L"  CO2 / yr   " << wfmt(k * 365 * CO2_FACTOR) << L" kg";
    SetWindowTextW(hSum, ss.str().c_str());
}

// ── layout ────────────────────────────────────────────────
static void DoLayout(HWND hWnd) {
    RECT rc; GetClientRect(hWnd, &rc);
    int W = rc.right, H = rc.bottom;

    // Status bar: send WM_SIZE to let it auto-size to bottom
    if (hStat) SendMessage(hStat, WM_SIZE, 0, 0);
    int SH = 22;
    if (hStat) { RECT sr; GetClientRect(hStat, &sr); SH = (sr.bottom > 0) ? sr.bottom : 22; }

    int UH = H - SH;

    // ListView: top section, leaves 162px for controls below
    int listH = UH - 162;
    if (listH < 120) listH = 120;
    if (hList) SetWindowPos(hList, nullptr, 10, 10, W - 20, listH, SWP_NOZORDER | SWP_NOACTIVATE);

    // Button row
    int btnY = listH + 16;
    struct { int id, x; } btns[] = {{ID_ADD,10},{ID_EDIT,108},{ID_DEL,206},{ID_EXP,304}};
    for (auto& b : btns) {
        HWND h = GetDlgItem(hWnd, b.id);
        if (h) SetWindowPos(h, nullptr, b.x, btnY, 90, 30, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Summary and rate panel
    int sumY  = btnY + 42;
    int sumW  = (W - 30) * 3 / 5;
    int sumH  = UH - sumY - 6;
    if (sumH < 68) sumH = 68;
    if (hSum) SetWindowPos(hSum, nullptr, 10, sumY, sumW, sumH, SWP_NOZORDER | SWP_NOACTIVATE);

    // Rate panel (right of summary)
    int rx = sumW + 22;
    int rw = W - rx - 12;
    if (rw < 90) rw = 90;
    HWND hrl = GetDlgItem(hWnd, ID_RATELBL);
    HWND hrb = GetDlgItem(hWnd, ID_RATEBTN);
    if (hrl)   SetWindowPos(hrl,   nullptr, rx, sumY,      rw,      20, SWP_NOZORDER | SWP_NOACTIVATE);
    if (hRate) SetWindowPos(hRate, nullptr, rx, sumY + 28, rw - 84, 26, SWP_NOZORDER | SWP_NOACTIVATE);
    if (hrb)   SetWindowPos(hrb,   nullptr, rx + rw - 80, sumY + 26, 80, 30, SWP_NOZORDER | SWP_NOACTIVATE);
}

// ── add/edit dialog ───────────────────────────────────────
struct DlgData { Appliance* target; bool saved; };
static HWND     dDlg = nullptr;
static HWND     dName, dWatts, dHours, dRate, dDflt;
static DlgData* dData = nullptr;

static void dLabel(HWND p, const wchar_t* t, int x, int y) {
    HWND h = CreateWindowExW(0, L"STATIC", t, WS_CHILD | WS_VISIBLE | SS_RIGHT,
                             x, y, 104, 20, p, nullptr,
                             (HINSTANCE)GetWindowLongPtrW(p, GWLP_HINSTANCE), nullptr);
    if (fUI) SendMessage(h, WM_SETFONT, (WPARAM)fUI, TRUE);
}

static HWND dEdit(HWND p, int id, const wchar_t* v, int x, int y) {
    HWND h = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", v,
                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                             x, y, 164, 26, p, (HMENU)(INT_PTR)id,
                             (HINSTANCE)GetWindowLongPtrW(p, GWLP_HINSTANCE), nullptr);
    if (fUI) SendMessage(h, WM_SETFONT, (WPARAM)fUI, TRUE);
    return h;
}

static HWND dBtn(HWND p, int id, const wchar_t* t, DWORD sty, int x, int y, int w, int h) {
    HWND hw = CreateWindowExW(0, L"BUTTON", t, WS_CHILD | WS_VISIBLE | WS_TABSTOP | sty,
                              x, y, w, h, p, (HMENU)(INT_PTR)id,
                              (HINSTANCE)GetWindowLongPtrW(p, GWLP_HINSTANCE), nullptr);
    if (fUI) SendMessage(hw, WM_SETFONT, (WPARAM)fUI, TRUE);
    return hw;
}

LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        dData = (DlgData*)((CREATESTRUCT*)lp)->lpCreateParams;
        int lx = 12, ex = 122, row = 18;

        dLabel(hWnd, L"Name:",         lx, row); dName  = dEdit(hWnd, D_NAME,  L"",                  ex, row - 3); row += 40;
        dLabel(hWnd, L"Power (W):",    lx, row); dWatts = dEdit(hWnd, D_WATTS, L"",                  ex, row - 3); row += 40;
        dLabel(hWnd, L"Hrs / Day:",    lx, row); dHours = dEdit(hWnd, D_HOURS, L"",                  ex, row - 3); row += 40;
        dLabel(hWnd, L"Rate ($/kWh):", lx, row); dRate  = dEdit(hWnd, D_RATE,  wfmt(g_rate, 4).c_str(), ex, row - 3); row += 40;

        dDflt = dBtn(hWnd, D_DFLT, L"Use default rate", BS_AUTOCHECKBOX, ex, row, 164, 22);
        SendMessage(dDflt, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(dRate, FALSE);
        row += 36;

        dBtn(hWnd, IDOK,     L"Save",   BS_DEFPUSHBUTTON, ex,      row, 76, 28);
        dBtn(hWnd, IDCANCEL, L"Cancel", BS_PUSHBUTTON,    ex + 84, row, 76, 28);

        // Populate for edit
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
    int dx = (rc.left + rc.right  - 308) / 2;
    int dy = (rc.top  + rc.bottom - 310) / 2;

    dDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        CLS_DLG, target ? L"Edit Appliance" : L"Add Appliance",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        dx, dy, 308, 310,
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

// ── main window procedure ─────────────────────────────────
static void InitListCols() {
    struct { const wchar_t* name; int w; int fmt; } cols[] = {
        {L"#",        36, LVCFMT_CENTER},
        {L"Name",    182, LVCFMT_LEFT  },
        {L"Watts",    70, LVCFMT_RIGHT },
        {L"Hrs/Day",  68, LVCFMT_RIGHT },
        {L"kWh/Day",  78, LVCFMT_RIGHT },
        {L"$/Day",    86, LVCFMT_RIGHT },
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
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    case WM_CREATE: {
        // System UI font (Segoe UI on Win7+)
        NONCLIENTMETRICSW ncm = {};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        fUI = CreateFontIndirectW(&ncm.lfMessageFont);

        HINSTANCE hi = ((CREATESTRUCT*)lp)->hInstance;

        // ListView
        hList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
            10, 10, 740, 280, hWnd, (HMENU)ID_LIST, hi, nullptr);
        SendMessage(hList, WM_SETFONT, (WPARAM)fUI, TRUE);
        InitListCols();

        // Action buttons
        struct { const wchar_t* lbl; int id; } btnDefs[] = {
            {L"+ Add", ID_ADD}, {L"Edit", ID_EDIT}, {L"Remove", ID_DEL}, {L"Export CSV", ID_EXP}
        };
        for (int i = 0; i < 4; i++) {
            HWND h = CreateWindowExW(0, L"BUTTON", btnDefs[i].lbl,
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10 + 98 * i, 300, 90, 30, hWnd, (HMENU)(INT_PTR)btnDefs[i].id, hi, nullptr);
            SendMessage(h, WM_SETFONT, (WPARAM)fUI, TRUE);
        }

        // Summary (read-only multiline edit)
        hSum = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY,
            10, 344, 440, 88, hWnd, (HMENU)ID_SUM, hi, nullptr);
        SendMessage(hSum, WM_SETFONT, (WPARAM)fUI, TRUE);

        // Rate label
        HWND hrl = CreateWindowExW(0, L"STATIC", L"Default rate ($/kWh):",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            464, 344, 200, 20, hWnd, (HMENU)ID_RATELBL, hi, nullptr);
        SendMessage(hrl, WM_SETFONT, (WPARAM)fUI, TRUE);

        // Rate edit
        hRate = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0.1200",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            464, 370, 120, 26, hWnd, (HMENU)ID_RATE, hi, nullptr);
        SendMessage(hRate, WM_SETFONT, (WPARAM)fUI, TRUE);

        // Rate update button
        HWND hrb = CreateWindowExW(0, L"BUTTON", L"Update",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            592, 368, 80, 30, hWnd, (HMENU)ID_RATEBTN, hi, nullptr);
        SendMessage(hrb, WM_SETFONT, (WPARAM)fUI, TRUE);

        // Status bar
        hStat = CreateWindowExW(0, L"msctls_statusbar32", L"",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hWnd, (HMENU)ID_STAT, hi, nullptr);

        // Load saved data
        LoadData();
        SetWindowTextW(hRate, wfmt(g_rate, 4).c_str());
        RefreshList();
        RefreshSummary();
        return 0;
    }

    case WM_SIZE:
        DoLayout(hWnd);
        return 0;

    case WM_GETMINMAXINFO: {
        auto* mm = (MINMAXINFO*)lp;
        mm->ptMinTrackSize = {500, 420};
        return 0;
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
            // Alternating row colors
            if (nm->code == NM_CUSTOMDRAW) {
                auto* cd = (NMLVCUSTOMDRAW*)lp;
                switch (cd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT:
                    if (!(cd->nmcd.uItemState & CDIS_SELECTED)) {
                        cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0)
                            ? RGB(255, 255, 255)
                            : RGB(245, 247, 251);
                        cd->clrText = RGB(20, 20, 20);
                    }
                    return CDRF_NEWFONT;
                }
                return CDRF_DODEFAULT;
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

    // Keyboard shortcuts
    case WM_KEYDOWN:
        if (wp == VK_DELETE) {
            int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (i >= 0) SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_DEL, 0), 0);
        }
        return 0;

    case WM_DESTROY:
        SaveData();
        if (fUI) DeleteObject(fUI);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}

// ── entry point ───────────────────────────────────────────
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

    // Register main window class
    {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = CLS_MAIN;
        wc.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassExW(&wc);
    }

    hMain = CreateWindowExW(
        0, CLS_MAIN, L"Power Consumption Calculator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 760, 520,
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
