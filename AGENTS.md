# PeakPoint Companion — Project Specification

---

## 1. Project Overview

PeakPoint Companion is a wearable firmware for the Seeed Studio XIAO nRF52840. It tracks a daily score built from user-submitted activity levels, stores a session history, and syncs the data to a smartphone via Bluetooth Low Energy (BLE).

---

## 2. Hardware

| Component | Details |
|---|---|
| Microcontroller | Seeed Studio XIAO nRF52840 |
| Display | 0.91" 128×32 OLED, single color (white), I2C (VCC, GND, SDA, SCL) |
| Input | Rotary Encoder KY-040 breakout board (GND, +, SW, DT, CLK) |

**Pin Assignments**

| Signal | Pin |
|---|---|
| CLK | D0 |
| DT | D1 |
| SW | D2 |

---

## 3. Development Environment

- **Language:** C/C++, Arduino-compatible
- **Framework:** PlatformIO, Arduino framework
- **Board:** `seeed-xiao-afruitnrf52-nrf52840`
- **Platform:** https://github.com/Seeed-Studio/platform-seeedboards.git
- **Programming guideline:** https://docs.arduino.cc/programming/
- **Upload port:** `/dev/ttyACM0`

**Libraries (`lib_deps`)**

| Library | Purpose |
|---|---|
| `mathertel/RotaryEncoder @ ^1.5.3` | Encoder reading (debounced, direction-safe, acceleration support) |
| `adafruit/Adafruit SSD1306 @ ^2.5.7` | OLED driver |
| `adafruit/Adafruit GFX Library @ ^1.11.5` | Graphics primitives |
| `adafruit/RTClib @ ^2.1.4` | Software RTC (`RTC_Millis`) |
| `bluefruit.h` | BLE stack (bundled with Adafruit nRF52 framework, no extra entry needed) |

---

## 4. Software Architecture

- Modular design: each feature area is a dedicated `.h` / `.cpp` pair
- Modules are linked via header files only
- `main.cpp` is the orchestrator (state machine + setup/loop)

**Module overview**

| Module | Header | Source | Responsibility |
|---|---|---|---|
| Display | `include/display.h` | `src/display.cpp` | All OLED rendering |
| Encoder | `include/encoder.h` | `src/encoder.cpp` | Rotation + button reading |
| Scoring | `include/scoring.h` | `src/scoring.cpp` | Score data + submit/reset logic |
| RTC | `include/rtc_time.h` | `src/rtc_time.cpp` | Timestamp + date string |
| BLE | `include/ble_connection.h` | `src/ble_connection.cpp` | BLE GATT + data sync |
| Main | — | `src/main.cpp` | State machine, wiring |

---

## 5. Data Model

### 5.1 `selectedLevel` — Virtual Scroll Wheel

| Property | Value |
|---|---|
| Variable | `selectedLevel` |
| Type | `uint8_t` |
| Init | `1` |
| Min | `1` |
| Max | `9` |
| Display font | `setTextSize(3)` |
| Display position | Float left, vertically centered |

**Level → Points mapping**

| Level | Points |
|---|---|
| 1 | 1 |
| 2 | 3 |
| 3 | 6 |
| 4 | 15 |
| 5 | 25 |
| 6 | 45 |
| 7 | 80 |
| 8 | 130 |
| 9 | 200 |

### 5.2 `dayScoreCounter` — Day Score

| Property | Value |
|---|---|
| Variable | `dayScoreCounter` |
| Type | `uint16_t` |
| Init | `0` |
| Min | `0` |
| Max | `9999` |
| Display font | `setTextSize(4)` (24×32 px, full line height) |
| Display position | Float right, vertically centered |

### 5.3 `dayScoreHistory` — Session History

| Property | Value |
|---|---|
| Variable | `dayScoreHistory` |
| Type | Array of `DayScoreEntry` structs |
| Max entries | `100` |
| Init | Empty |

**Structure (`DayScoreEntry`)**
```
timestamp[9]  // "HH:MM:SS" (ODBC format)
level         // uint8_t
```

**Example (JSON representation)**
```json
{
  "09:38:12": "3",
  "09:39:22": "4",
  "09:42:54": "7"
}
```

---

## 6. State Machine

```
STATE_NORMAL
    │  long-press (≥500ms) → countdown starts
    ▼
STATE_FINISHING
    │  3s countdown completes → display "Day finished! Good job!"
    ▼
STATE_FINISHED
    │  long-press (≥500ms) → BLE countdown starts
    ▼
STATE_BLE_PENDING
    │  3s countdown completes → BLE starts advertising
    ▼
STATE_BLE
    │  sync complete or timeout → resetDay(), return to NORMAL
    ▼
STATE_NORMAL
```

**State descriptions**

| State | Description |
|---|---|
| `STATE_NORMAL` | Default working state. Encoder adjusts level; short press submits. |
| `STATE_FINISHING` | Long-press countdown (3→0). Release cancels back to NORMAL. |
| `STATE_FINISHED` | Scrollable history list. Long-press starts BLE countdown. |
| `STATE_BLE_PENDING` | BLE countdown (3→0). Release cancels back to FINISHED. |
| `STATE_BLE` | BLE advertising, connecting, syncing. No encoder input. |

---

## 7. Functional Requirements

### 7.1 Function 1 — DayScore Creation

1. **Level selection:** Rotating the encoder clockwise increments `selectedLevel` (max 9); counter-clockwise decrements it (min 1). The level is not reset after submitting.
2. **Submit (short press):** Adds `levelToPoints[selectedLevel]` to `dayScoreCounter` (clamped to 9999). Appends a timestamped entry to `dayScoreHistory`.
3. **Finish day (long-press in NORMAL):**
   - After 500 ms hold, transitions to `STATE_FINISHING`
   - Display shows `"finishing day in <N>"` with a 3-second countdown
   - Releasing the button before countdown ends cancels back to `STATE_NORMAL`
   - When countdown reaches 0: display shows `"Day finished! Good job!"`, then transitions to `STATE_FINISHED`
4. **History review (STATE_FINISHED):**
   - Displays `dayScoreHistory` as a vertically scrollable list
   - Two rows visible at a time; each row: `"HH:MM:SS  L<N>"`
   - Encoder scrolls the list
   - ▲ triangle in top-right corner: visible when scrolling up is possible; hidden at the top
   - ▼ triangle in bottom-right corner: visible when scrolling down is possible; hidden at the bottom
5. **Day reset:** `dayScoreCounter`, `dayScoreHistory`, and `selectedLevel` reset to initial values after BLE sync completes.

### 7.2 Function 2 — Bluetooth Connection

**Protocol:** BLE GATT

```
Device (XIAO nRF52840)
  └── Service: "PeakPoint Data"
        ├── Characteristic_1: "dayScoreHistory"  (Read + Notify, max 512 bytes)
        └── Characteristic_2: "status"           (Read + Notify, max 32 bytes)
```

**UUIDs**
```
Service:          12345678-1234-1234-1234-1234567890AB
Characteristic_1: 12345678-1234-1234-1234-1234567890AC
Characteristic_2: 12345678-1234-1234-1234-1234567890AD
```

**Behavior**

| Step | Trigger | Display |
|---|---|---|
| BLE countdown | Long-press (≥500ms) in `STATE_FINISHED` | `"BLE transmission in <N>"` |
| Advertising | Countdown reaches 0 | `"Connecting to Smartphone..."` |
| Connected | Smartphone connects | `"Smartphone connected!"` |
| Syncing | Transmitting JSON to Characteristic_1 | `"Syncing data..."` |
| Sync done | Write acknowledged | `"Sync finished!"` |
| Error | Any BLE or sync failure | `"BLE Error!"` |
| Timeout | No connection within 30 seconds | `"BLE Error!"` → back to `STATE_NORMAL` |

**Rules**
- BLE stays off in `STATE_NORMAL` — power consumption must be minimal
- Releasing the button during the BLE countdown cancels back to `STATE_FINISHED`
- After sync completes (success or error), `resetDay()` is called and device returns to `STATE_NORMAL`
- Display messages wider than 128 px are automatically split across two lines at a word boundary

**Data format (Characteristic_1 payload)**
```json
{
  "09:38:12": "3",
  "09:39:22": "4",
  "09:42:54": "7"
}
```

---

## 8. Display Reference

| Context | Font size | Notes |
|---|---|---|
| `selectedLevel` | `setTextSize(3)` | Float left, vertically centered |
| `dayScoreCounter` | `setTextSize(4)` | Float right, vertically centered |
| Countdown number | `setTextSize(2)` | Centered |
| All other text | `setTextSize(1)` | Centered; auto-split if too wide |

Long messages that exceed the 128 px screen width are split at the last fitting word boundary and rendered on two lines (y=6, y=18).

---

## 9. Known Defects

| # | Status | Description |
|---|---|---|
| 1 | ✅ Solved | Fast encoder scrolling caused `selectedLevel` to randomly jump back. Fixed by switching to `mathertel/RotaryEncoder` with `TWO03` latch mode. |
| 2 | ✅ Solved | Countdown continued after releasing the knob in `STATE_FINISHING`. Fixed by checking button state each loop iteration. |

---

## 10. Future Features — DO NOT IMPLEMENT

### Extended Bluetooth
- Multi-packet transmission when negotiated MTU is smaller than the payload
- Detailed error messages (e.g. "Connection lost", "No device found", "Sync aborted")
- Globally unique UUIDs generated with `uuidgen` (for published product)

### Real Time Clock (RTC)
- Get time signal from smartphone via BLE on sync
- Use received time to calibrate `RTC_Millis` drift (track millis offset)
- Use Adafruit RTClib for battery-backed RTC hardware

### Boot Screen
- PeakPoint logo/graphic
- Text: `"PeakPoint Companion"`
- Version number: `<Major.Minor.Patch>`

### Screen Rotation
- Portrait mode
- Use `display.width()` / `display.height()` for rotated coordinate system

### Days-History
Persist each finished day's score and history across sessions.

| Property | Value |
|---|---|
| Variable | `daysHistory` |
| Type | Array of `DayEntry` structs |
| Max entries | `30` |
| Init | Empty |

**Structure (`DayEntry`)**
```
date[11]      // "dd.mm.yyyy"
score         // uint16_t
history[]     // DayScoreEntry[100]
historyCount  // uint16_t
```

**Example (JSON representation)**
```json
{
  "21.04.2026": { "score": 101, "history": { "09:38:12": "3", "09:39:22": "4" } },
  "22.04.2026": { "score": 98,  "history": { "11:08:22": "2", "11:10:02": "4" } }
}
```
