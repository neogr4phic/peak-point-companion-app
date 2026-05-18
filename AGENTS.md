# PeakPoint Companion — Project Specification


## 1. Project Overview

PeakPoint Companion is a wearable firmware for the Seeed Studio XIAO nRF52840.
It tracks a daily score built from user-submitted activity levels, stores a
session history, and syncs the data to a smartphone via Bluetooth Low Energy (BLE).


## 2. Hardware

Components:
  Microcontroller  Seeed Studio XIAO nRF52840
  Display          0.91" 128×32 OLED, single color (white), I2C (VCC, GND, SDA, SCL)
  Input            Rotary Encoder KY-040 breakout board (GND, +, SW, DT, CLK)

Pin assignments:
  CLK  D0
  DT   D1
  SW   D2


## 3. Development Environment

  Language           C/C++, Arduino-compatible
  Framework          PlatformIO, Arduino framework
  Board              seeed-xiao-afruitnrf52-nrf52840
  Platform           https://github.com/Seeed-Studio/platform-seeedboards.git
  Programming guide  https://docs.arduino.cc/programming/
  Upload port        /dev/ttyACM0

Libraries (lib_deps):
  mathertel/RotaryEncoder @ ^1.5.3      Encoder reading (debounced, direction-safe, acceleration)
  adafruit/Adafruit SSD1306 @ ^2.5.7    OLED driver
  adafruit/Adafruit GFX Library @ ^1.11.5  Graphics primitives
  adafruit/RTClib @ ^2.1.4              Software RTC (RTC_Millis)
  bluefruit.h                           BLE stack (bundled with Adafruit nRF52, no lib_deps entry needed)


## 4. Software Architecture

- Modular design: each feature area is a dedicated .h / .cpp pair
- Modules are linked via header files only
- main.cpp is the orchestrator (state machine + setup/loop)

Modules:
  Display   include/display.h       src/display.cpp         All OLED rendering
  Encoder   include/encoder.h       src/encoder.cpp         Rotation + button reading
  Scoring   include/scoring.h       src/scoring.cpp         Score data + submit/reset logic
  RTC       include/rtc_time.h      src/rtc_time.cpp        Timestamp
  BLE       include/ble_connection.h  src/ble_connection.cpp  BLE GATT + data sync
  Main      —                       src/main.cpp            State machine, wiring


## 5. Data Model

### 5.1 selectedLevel — Virtual Scroll Wheel

  Variable          selectedLevel
  Type              uint8_t
  Init              1
  Min               1
  Max               9
  Display font      setTextSize(3)
  Display position  Float left, vertically centered

Level → Points mapping:
  1 →   1
  2 →   3
  3 →   6
  4 →  15
  5 →  25
  6 →  45
  7 →  80
  8 → 130
  9 → 200

### 5.2 dayScoreCounter — Day Score

  Variable          dayScoreCounter
  Type              uint16_t
  Init              0
  Min               0
  Max               9999
  Display font      setTextSize(4)  (24×32 px, full line height)
  Display position  Float right, vertically centered

### 5.3 dayScoreHistory — Session History

  Variable     dayScoreHistory
  Type         Array of DayScoreEntry structs
  Max entries  100
  Init         Empty

Structure (DayScoreEntry):
  timestamp[9]  // "HH:MM:SS" (ODBC format)
  level         // uint8_t

Example (JSON):
```json
{
  "09:38:12": "3",
  "09:39:22": "4",
  "09:42:54": "7"
}
```


## 6. State Machine

Flow:
  STATE_NORMAL
    → long-press (≥500ms) → STATE_MENU (context: NORMAL)
  STATE_MENU (context: NORMAL)                    [history empty: 2 items; non-empty: 4 items]
    → select "Finish day"      → STATE_SCORE
    → select "Delete last"    → delete most recent entry → STATE_NORMAL
    → select "Edit scores"    → STATE_HISTORY
    → select "Back"           → STATE_NORMAL
  STATE_HISTORY
    → short press (history not empty) → STATE_MENU (context: ENTRY)
  STATE_MENU (context: ENTRY)
    → select "Delete selection" → delete top visible entry → flash("Score deleted") → STATE_HISTORY
    → select "Delete all"       → clearHistory() → flash("List deleted") → STATE_MENU (context: NORMAL)
    → select "Scroll list"      → STATE_HISTORY
    → select "Back"             → STATE_MENU (context: NORMAL)
  STATE_SCORE
    → short press → STATE_MENU (context: SCORE)
  STATE_MENU (context: SCORE)
    → select "Transmit score" → STATE_BLE
    → select "Dismiss score"  → resetDay() → STATE_NORMAL
  STATE_FLASH
    → auto-dismiss after 1500 ms → stored return state
  STATE_BLE
    → sync complete or timeout → resetDay() → STATE_NORMAL

State descriptions:
  STATE_NORMAL    Default working state. Encoder adjusts level; short press submits.
  STATE_MENU      Context-sensitive menu. Encoder moves cursor; short press selects.
  STATE_HISTORY   Scrollable history list. Short press opens entry menu.
  STATE_SCORE     Full-screen final score with confetti animation. Short press opens score menu.
  STATE_FLASH     Auto-dismissing feedback message. No input accepted.
  STATE_BLE       BLE advertising, connecting, syncing. No encoder input.

Menu contents by context:
  MENU_CTX_NORMAL (history empty)     item 0: "Finish day"       item 1: "Back"
  MENU_CTX_NORMAL (history non-empty) item 0: "Finish day"       item 1: "Delete last"
                                      item 2: "Edit scores"      item 3: "Back"
  MENU_CTX_ENTRY                      item 0: "Delete selection" item 1: "Delete all"
                                      item 2: "Scroll list"      item 3: "Back"
  MENU_CTX_SCORE                      item 0: "Transmit score"   item 1: "Dismiss score"


## 7. Functional Requirements

### 7.1 Function 1 — DayScore Creation

1. Level selection
   Rotating encoder clockwise increments selectedLevel (max 9).
   Counter-clockwise decrements it (min 1).
   The level is not reset after submitting.

2. Submit (short press in STATE_NORMAL)
   Adds levelToPoints[selectedLevel] to dayScoreCounter (clamped to 9999).
   Appends a timestamped entry to dayScoreHistory.

3. Options menu (long-press in STATE_NORMAL)
   - After 500 ms hold, opens STATE_MENU (context: NORMAL)
   - When history is empty:     2-item menu: "> Finish day" / "  Back"
   - When history non-empty:    4-item menu: "> Finish day" / "  Delete last" /
                                             "  Edit scores" / "  Back"
   - Encoder moves cursor; short press selects
   - "Finish day"   → STATE_SCORE (full-screen achievement display)
   - "Delete last"  → deletes most recent history entry, subtracts its points
                       from dayScoreCounter (floor 0), returns to STATE_NORMAL
   - "Edit scores"  → historyScrollOffset=0; enters STATE_HISTORY
   - "Back"         → STATE_NORMAL

4. History browser (STATE_HISTORY)
   - Displays dayScoreHistory as a vertically scrollable list
   - Two rows visible at a time
   - Top row: "> HH:MM:SS  L<N>" — marked as selected; short press targets this entry
   - Bottom row: "  HH:MM:SS  L<N>"
   - Encoder scrolls the list
   - Triangle up (top-right): visible when scrolling up is possible
   - Triangle down (bottom-right): visible when scrolling down is possible
   - No long-press; navigation back is via "Back" in the entry menu

5. Entry menu (short press in STATE_HISTORY)
   - Short press opens STATE_MENU (context: ENTRY) targeting the top visible entry
   - 4-item menu: "> Delete selection" / "  Delete all" / "  Scroll list" / "  Back"
   - "Delete selection": removes the targeted entry, subtracts its points from
     dayScoreCounter (floor 0), clamps historyScrollOffset, shows flash("Score deleted"),
     returns to STATE_HISTORY
   - "Delete all": calls clearHistory() (zeros counter and history), shows
     flash("List deleted"), returns to STATE_MENU (context: NORMAL)
   - "Scroll list": returns to STATE_HISTORY
   - "Back": returns to STATE_MENU (context: NORMAL)
   - Short press does nothing if dayScoreHistory is empty

6. Final score (STATE_SCORE)
   - Displays "Great job!" label + framed dayScoreCounter at textSize(2) with a confetti animation
   - After SCORE_AUTO_MENU_MS (3000 ms) opens STATE_MENU (context: SCORE) automatically
   - Short press opens STATE_MENU (context: SCORE) immediately
   - 2-item menu: "> Transmit score" / "  Dismiss score"
   - "Transmit score": starts BLE advertising → STATE_BLE
   - "Dismiss score": calls resetDay() → STATE_NORMAL

7. Flash feedback (STATE_FLASH)
   - Displays a centered message for FLASH_DURATION_MS (1500 ms)
   - No encoder or button input accepted during this time
   - Automatically transitions to the stored return state

8. Day reset
   dayScoreCounter, dayScoreHistory, and selectedLevel reset to initial
   values after BLE sync completes or when "Dismiss score" is selected.
   clearHistory() resets counter and history only (selectedLevel preserved).

### 7.2 Function 2 — Bluetooth Connection

Protocol: BLE GATT

```
Device (XIAO nRF52840)
  └── Service: "PeakPoint Data"
        ├── Characteristic_1: "dayScoreHistory"  (Read + Notify, max 512 bytes)
        └── Characteristic_2: "status"           (Read + Notify, max 32 bytes)
```

UUIDs:
  Service:          12345678-1234-1234-1234-1234567890AB
  Characteristic_1: 12345678-1234-1234-1234-1234567890AC
  Characteristic_2: 12345678-1234-1234-1234-1234567890AD

BLE flow and display messages:
  Select "Transmit score"       →  "Connecting to Smartphone..."
  Smartphone connects           →  "Smartphone connected!"
  Transmitting JSON             →  "Syncing data..."
  Write acknowledged            →  "Sync finished!"
  Any BLE or sync failure       →  "BLE Error!"
  No connection within 30s      →  "BLE Error!" → back to STATE_NORMAL

Rules:
  - BLE stays off in STATE_NORMAL — power consumption must be minimal
  - Selecting "Dismiss score" in the menu returns to STATE_NORMAL without BLE
  - After sync completes (success or error), resetDay() is called and device
    returns to STATE_NORMAL
  - Display messages wider than 128 px are automatically split across two lines
    at a word boundary

Data format (Characteristic_1 payload):
```json
{
  "09:38:12": "3",
  "09:39:22": "4",
  "09:42:54": "7"
}
```


## 8. Display Reference

Font sizes:
  selectedLevel     setTextSize(3)  Float left, vertically centered
  dayScoreCounter   setTextSize(4)  Float right, vertically centered
  Final score       setTextSize(3)  Centered horizontally, y=4
  Menu items        setTextSize(1)  Left-aligned; ">" prefix on selected item
  All other text    setTextSize(1)  Centered; auto-split if too wide

Menu layout (STATE_MENU):
  Always 2 rows visible regardless of total item count (2-item scroll window).
  Row 0  y=4   "> " + items[cursor]      — selected item
  Row 1  y=20  "  " + items[cursor+1]    — next item preview (hidden when cursor is last)
  Top-right triangle:    visible when cursor > 0 (items exist above)
  Bottom-right triangle: visible when cursor+2 < count (items exist below preview row)

History list layout (STATE_HISTORY):
  Row 0  y=4   "> HH:MM:SS  L<N>"  — top entry, targeted by short press
  Row 1  y=20  "  HH:MM:SS  L<N>"
  Top-right triangle: visible when scrollOffset > 0
  Bottom-right triangle: visible when more entries exist below

Final score layout (STATE_SCORE):
  Background: 12 confetti particles animated downward (deterministic LCG)
  Label:       "Great job!" textSize(1), centered horizontally at y=2
  Score:       textSize(2), centered horizontally at y=14; rect frame with 3 px padding

Long messages that exceed 128 px are split at the last fitting word boundary
and rendered on two lines (y=6, y=18).


## 9. Future Features — DO NOT IMPLEMENT

Gamification:
- instead of points, you will gain meters in altitude (like when climbing a mountain). The unit should be "m" for meters.

Translation:
- all visible texts should have a german and englisch version
- the language should be set in the options menu (not yet implemented)

Battery indicator:
- a visible battery indicator
- a warning message when battery is in critical level (<=5 %)

Settings menu:
- a menu to control general settings for the device
  -> language
  -> screen brightness
  -> version info
  -> firmware update

Extended Bluetooth:
  - Multi-packet transmission when negotiated MTU is smaller than the payload
  - Detailed error messages (e.g. "Connection lost", "No device found", "Sync aborted")
  - Globally unique UUIDs generated with uuidgen (for published product)

[ in work ] Real Time Clock (RTC):
  - Get time signal from smartphone via BLE on sync
  - Use received time to calibrate RTC_Millis drift (track millis offset)
  - Use Adafruit RTClib for battery-backed RTC hardware

Boot Screen:
  - PeakPoint logo/graphic
  - Text: "PeakPoint Companion"
  - Version number: <Major.Minor.Patch>

Screen Rotation:
  - Portrait mode
  - Use display.width() / display.height() for rotated coordinate system

Days-History — persist each finished day's score and history across sessions:
  Variable     daysHistory
  Type         Array of DayEntry structs
  Max entries  30
  Init         Empty

  Structure (DayEntry):
    date[11]      // "dd.mm.yyyy"
    score         // uint16_t
    history[]     // DayScoreEntry[100]
    historyCount  // uint16_t

  Example (JSON):
```json
{
  "21.04.2026": { "score": 101, "history": { "09:38:12": "3", "09:39:22": "4" } },
  "22.04.2026": { "score": 98,  "history": { "11:08:22": "2", "11:10:02": "4" } }
}
```
