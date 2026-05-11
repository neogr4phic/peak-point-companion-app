# Plan: Restructured History Navigation


## Summary

"Go to history" opens the history list from the long-press menu. In the list,
long-press goes directly back to the same long-press menu (no sub-menu).
"Finish day" opens a "Sync via BLE / [cancel]" sub-menu; [cancel] returns to
the long-press menu. No modes, no flags — STATE_FINISHED is just a history
browser.


## Complete New Flow

```
STATE_NORMAL
  long-press → MENU_CTX_NORMAL

MENU_CTX_NORMAL  [history empty: 2 items; history non-empty: 4 items]
  "Finish day"    → MENU_CTX_FINISHED
  "Go to history" → STATE_FINISHED
  "Delete last"   → deleteHistoryEntry(last) → STATE_NORMAL
  "[cancel]"      → STATE_NORMAL

MENU_CTX_FINISHED
  "Sync via BLE"  → STATE_BLE
  "[cancel]"      → MENU_CTX_NORMAL   ← back to long-press menu

STATE_FINISHED  (history browser — no modes)
  encoder         → scroll list
  short press     → MENU_CTX_DELETE
  long-press      → MENU_CTX_NORMAL   ← direct back, no sub-menu

MENU_CTX_DELETE
  "Delete"        → deleteHistoryEntry(offset) → clamp → STATE_FINISHED
  "[cancel]"      → STATE_FINISHED
```


## Steps

### Phase 1 — Display  (src/display.cpp)

1. `displayMenu()`: add 4-item case.
   Add `rowY4[] = {0, 8, 16, 24}` branch alongside existing `rowY2`/`rowY3`.
   textSize(1) = 8 px tall → 4 × 8 = 32 px, fills the display exactly, no overlap.

### Phase 2 — Main  (src/main.cpp)  — depends on Phase 1

2. Replace `menuItemsNormal3[]` with:
   `menuItemsNormal4[] = {"Finish day", "Go to history", "Delete last", "[cancel]"}`

3. `itemCount` for `MENU_CTX_NORMAL` non-empty: 4 (was 3).

4. `MENU_CTX_NORMAL` action handler — remap all 4 cursors:
   - cursor 0 → "Finish day"    → menuContext=MENU_CTX_FINISHED; menuCursor=0; appState=STATE_MENU
   - cursor 1 → "Go to history" → historyScrollOffset=0; appState=STATE_FINISHED
   - cursor 2 → "Delete last"   → deleteHistoryEntry(dayScoreHistoryCount-1); appState=STATE_NORMAL
   - cursor 3 → "[cancel]"      → appState=STATE_NORMAL

5. `MENU_CTX_FINISHED` [cancel] (cursor 1):
   menuContext=MENU_CTX_NORMAL; menuCursor=0; appState=STATE_MENU
   (was: appState=STATE_FINISHED)

6. `STATE_FINISHED` long-press:
   menuContext=MENU_CTX_NORMAL; menuCursor=0; appState=STATE_MENU
   (was: menuContext=MENU_CTX_FINISHED)

### Phase 3 — Spec  (AGENTS.md)  — parallel with Phases 1 & 2

7. Sections 6, 7.1, 8 updated for new flow, 4-item MENU_CTX_NORMAL,
   STATE_FINISHED as pure history browser, MENU_CTX_FINISHED [cancel] → back.


## Relevant Files

- src/display.cpp   displayMenu() ~line 41
- src/main.cpp      menuItemsNormal3 ~line 30, itemCount ~line 104,
                    MENU_CTX_NORMAL handler ~line 116,
                    MENU_CTX_FINISHED [cancel] ~line 128,
                    STATE_FINISHED long-press ~line 165
- AGENTS.md         sections 6, 7.1, 8


## Verification

1. `pio run` → SUCCESS
2. Long-press (non-empty history) → 4-item menu; all labels fit 128 px at textSize(1)
3. "Go to history" → history list; encoder scrolls; short press → delete confirm;
   long-press → back to 4-item menu
4. "Finish day" → 2-item sync menu; "[cancel]" → back to 4-item menu;
   "Sync via BLE" → BLE flow → resetDay() → STATE_NORMAL
5. "Delete last" → last entry removed, back in STATE_NORMAL immediately
6. Empty history → 2-item menu (only "Finish day" / "[cancel]")


## Decisions

- Long-press in STATE_FINISHED goes directly back to MENU_CTX_NORMAL —
  no confirmation sub-menu; a single gesture, not a destructive action.
- historyScrollOffset reset to 0 on every entry into STATE_FINISHED.
- No historyReviewMode flag — STATE_FINISHED is always a pure browser;
  context lives entirely in menuContext.
- "Finish day" lives only in MENU_CTX_NORMAL — history browser has no
  path to sync or day-reset.
- MENU_CTX_DELETE [cancel] stays in STATE_FINISHED — correct in all paths.
