# GUI Overhaul Design — 2026-03-08

## Goal
Replace the current single-panel dark Win32/GDI UI with a Windows 11 / Fluent-inspired
owner-drawn interface featuring a left sidebar, three pages, and light/dark theme toggle.
No external dependencies added — pure GDI (Option A).

---

## Layout & Structure

- **Window minimum:** 920 × 580 px
- **Sidebar:** 220 px wide, owner-drawn, fixed left strip
  - App icon + title at top (⚡ Energy Calc)
  - Nav items: Dashboard · Appliances · Settings (36 px tall each)
  - Active item: 3 × 18 px accent pill, vertically centred on left edge
  - Hover: subtle tinted fill (Surface hover color)
  - Theme toggle pinned at bottom
- **Content area:** remaining width, swaps Win32 child controls per active page
  - Page title bar: 48 px tall, 20 pt semibold page name
  - Child controls shown/hidden on nav switch (not destroyed/recreated)

---

## Color System

Two `Theme` structs; `g_dark` bool selects active one. All brushes rebuilt on switch.

| Token          | Dark       | Light      |
|----------------|------------|------------|
| Background     | `#202020`  | `#F3F3F3`  |
| Surface        | `#2B2B2B`  | `#FFFFFF`  |
| Surface hover  | `#303030`  | `#F5F5F5`  |
| Accent         | `#60CDFF`  | `#0078D4`  |
| Accent pressed | `#4DB8E8`  | `#006CBE`  |
| Danger         | `#FF6666`  | `#C42B1C`  |
| Text primary   | `#FFFFFF`  | `#1A1A1A`  |
| Text secondary | `#9D9D9D`  | `#605E5C`  |
| Border         | `#3D3D3D`  | `#E0E0E0`  |
| Sidebar bg     | `#181818`  | `#E8E8E8`  |

### Fluent Details
- Corner radius: 8 px (cards/panels), 6 px (buttons), 4 px (inputs) — via `RoundRect`
- Buttons: filled accent for primary (Add), Surface fill for secondary (Edit, Export),
  Danger fill for Remove
- Stat card left bar: 4 px wide — blue (kWh), green (cost), amber (CO₂)
- Font: Segoe UI Variable → fallback Segoe UI
  - 13 px regular (body), 13 px semibold (labels), 22 px bold (stat numbers), 11 px small

---

## Pages

### Dashboard
- 2 × 2 grid of stat cards (~200 × 90 px each):
  Daily kWh · Daily Cost · Monthly Cost · Yearly CO₂
- Each card: colored 4 px left bar, large value (22 pt bold), label (11 pt secondary)
- Below cards: read-only top-5 appliances table (name · kWh/day · $/day), no actions

### Appliances
- Full-width ListView (same 6 columns as current: # · Name · Watts · Hrs/Day · kWh/Day · $/Day)
- Toolbar row **above** the list: `[+ Add]  [✎ Edit]  [Remove]  [Export CSV]` right-aligned
- Keyboard shortcuts preserved: Delete, F2, Ctrl+N, double-click to edit

### Settings
Two labelled sections separated by a divider:
- **Electricity** — rate edit field + Update button
- **Appearance** — Light / Dark segmented toggle (two owner-drawn buttons, one active)

---

## Implementation Notes

- `enum Page { PAGE_DASHBOARD, PAGE_APPLIANCES, PAGE_SETTINGS }` + `g_page` global
- `struct Theme { COLORREF bg, surface, surfaceHov, accent, ... }` with `g_dark`/`g_light` instances
- `RebuildBrushes()` deletes and recreates all `HBRUSH` globals from active theme
- `DoLayout()` repositions all child HWNDs; sidebar is purely painted (no child HWND)
- Sidebar hit-testing done in `WM_LBUTTONDOWN` by checking x < SIDEBAR_W and y bucket
- `WM_MOUSEMOVE` + `TrackMouseEvent` for sidebar hover redraws
- Add/edit dialog styling updated to match new theme colors
