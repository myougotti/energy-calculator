# CLAUDE.md

Guidance for AI assistants working in this repository.

## Working principles

- **Think before coding:** state assumptions, ask when unsure, never guess.
- **Simplicity first:** write the minimum code that solves the problem, nothing extra.
- **Surgical changes:** every changed line must trace back to the request.
- **Goal-driven:** turn vague instructions into verifiable success criteria before starting.

## What this project is

A **Windows desktop GUI application** that calculates and tracks household
appliance power consumption and energy cost. It is a single-file Win32 / GDI
program written in C++ (`power_calculator.cpp`, ~1130 lines). There is no
external UI framework — all rendering is hand-drawn with GDI and standard
Win32 common controls (ListView, status bar, edit boxes, owner-draw buttons).

> **README is stale.** `README.md` still describes the *original* console
> application (menus, `cout`-based interaction). That version no longer exists
> — the codebase is now a windowed GUI app. Trust the source code over the
> README. Update the README if you touch user-facing behavior.

## Build & run

The build targets **Windows only**. The `Makefile` links Win32 libraries
(`-mwindows -lcomctl32 -lgdi32 -lcomdlg32 -luxtheme -ldwmapi -lmsimg32`) and
produces `power_calculator.exe`.

```bash
make          # build power_calculator.exe
make run      # build + run (Windows only)
make clean    # remove build artifacts
make rebuild  # clean + build
```

Compilers: MinGW `g++` or MSVC. The code uses C++17 features (the Makefile
specifies `-std=c++11`, but the source relies on C++17 — prefer C++17 when
compiling manually).

**This Linux dev environment cannot build or run the app** — it has native
`g++`/`gcc` but no Windows headers (`windows.h`, `commctrl.h`, …) and no MinGW
cross-compiler. Do not attempt `make` here; it will fail on missing headers.
Verify changes by careful code review, not by building, unless a Windows
toolchain is available.

## Architecture (single file: `power_calculator.cpp`)

Message-driven Win32 app. Key pieces, roughly in file order:

- **Theme system** (`struct Theme`, `DARK`, `LIGHT`, `g_darkMode`, active
  `Theme T`): all colors come from `T.<field>`. Brushes are cached globals
  rebuilt via `RebuildBrushes()` whenever the theme changes.
- **Data model** (`struct Appliance`): `name/watts/hours/rate` plus derived
  `kWh()`, `cDay()`, `cMonth()`, `cYear()`, `co2Yr()`. State lives in globals
  `g_apps` (vector) and `g_rate` (default rate).
- **Persistence**: `SaveData()`/`LoadData()` read/write `appliances.dat`
  (see format below). Auto-saves on every add/edit/remove/rate-change and on
  `WM_DESTROY`. CSV export (`ID_EXP`) writes a timestamped `energy_report_*.csv`.
- **Two window classes**: main window (`WndProc`, class `PowerCalcMain`) and
  the add/edit modal (`DlgProc`, class `ApplianceDlg`, driven by `ShowAppDlg`).
- **Owner-draw buttons**: created with `BS_OWNERDRAW`, painted by
  `DrawButton()`, with hover/press tracked through `BtnSubclass` +
  `g_hover`/`g_press` maps.
- **ListView**: custom-drawn header and rows (`WM_NOTIFY` / `NM_CUSTOMDRAW`)
  for themed colors. Columns set in `InitListCols()`, rows filled by
  `RefreshList()`.
- **Sidebar**: painted (not a child window) by `DrawSidebar()`; nav items in
  the `NAV[]` table.

### Energy formulas
```
kWh/day   = watts * hours / 1000
$/day     = kWh/day * rate
$/month   = $/day * 30
$/year    = $/day * 365
CO2 kg/yr = kWh/day * 365 * CO2_FACTOR   (CO2_FACTOR = 0.386)
```

### Save file format (`appliances.dat`)
Line 1: default rate (`%.6f`). Each subsequent line is one appliance,
pipe-delimited, name UTF-8 encoded:
```
name|watts|hours|rate
```
`LoadData()` validates ranges (watts > 0, 0 ≤ hours ≤ 24, rate > 0).

## Current state: GUI overhaul IN PROGRESS

There is a multi-task plan to convert the single-panel UI into a Fluent /
Windows-11-style layout (left sidebar + Dashboard / Appliances / Settings
pages + live light/dark toggle):

- Design: `docs/plans/2026-03-08-gui-overhaul-design.md`
- Implementation plan (11 tasks): `docs/plans/2026-03-09-gui-overhaul-impl.md`

**Only Tasks 1–3 are done** (theme struct, `Page` enum / `SIDEBAR_W` /
`g_page`, and `DrawSidebar()` painting). Tasks 4–11 are **not yet
implemented**, so the code is in a transitional state. Notably:

- The sidebar is **painted but not interactive** — there is no
  `WM_LBUTTONDOWN` / `WM_MOUSEMOVE` nav hit-testing yet, so clicking nav items
  and the theme toggle does nothing (Task 4).
- `DoLayout()` still uses the **old single-panel layout** (ListView + button
  row + summary box + rate panel), not the per-page show/hide design (Task 5).
- `WM_PAINT` still paints the old `APPLIANCES`/`SUMMARY`/`RATE` labels via
  `PaintLabel()` — no page title bar, dashboard stat cards, or top-5 table yet
  (Task 6).
- No Settings theme toggle buttons; window min size is still `580×480` and
  `hSum` is still in use (Tasks 8, 11).

When continuing the overhaul, follow the impl-plan task order and the existing
patterns; build/verify steps in the plan assume a Windows toolchain.

## Conventions

- **Single translation unit.** Keep everything in `power_calculator.cpp`
  unless there's a strong reason to split.
- **Unicode throughout.** `UNICODE`/`_UNICODE` defined; use wide strings
  (`wstring`, `L"..."`, `*W` APIs). Convert at the file boundary with
  `toUTF8()` / `fromUTF8()`.
- **Colors via `T.*`** — never hardcode `RGB(...)` for UI chrome; add a field
  to `Theme` (and both `DARK`/`LIGHT`) instead.
- **GDI hygiene.** Every `CreateSolidBrush`/`CreatePen`/`CreateFont` must be
  paired with `DeleteObject`; cached brushes live in globals and are freed in
  `DestroyBrushes()`.
- **Control IDs** are enum constants (`ID_*` for main window, `D_*` for the
  dialog). Add new ones to the existing enums.
- Commit messages follow Conventional Commits (`feat:`, `refactor:`,
  `docs:`, `chore:`).

## Repo layout
```
power_calculator.cpp   # entire application
Makefile               # Windows build (MinGW/MSVC)
appliances.dat         # saved data (rate + appliances)
README.md              # STALE — describes old console version
TODO.md                # roadmap / wishlist
docs/plans/            # GUI overhaul design + impl plan
.vscode/               # Windows VS Code C/C++ config
power_calculator.exe   # committed build artifact (Windows binary)
```
