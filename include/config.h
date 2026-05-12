#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// Level → Points Mapping
// =============================================================================
// Points awarded per level when the knob is pressed.
// Index 0 is unused (levels start at 1).
#define LEVEL_POINTS_0    0
#define LEVEL_POINTS_1    1
#define LEVEL_POINTS_2    3
#define LEVEL_POINTS_3    6
#define LEVEL_POINTS_4    15
#define LEVEL_POINTS_5    25
#define LEVEL_POINTS_6    45
#define LEVEL_POINTS_7    80
#define LEVEL_POINTS_8    130
#define LEVEL_POINTS_9    200

// =============================================================================
// Score & Level Limits
// =============================================================================
#define LEVEL_MIN             1     // Minimum selectable level
#define LEVEL_INIT            1     // Level on startup / after reset
#define LEVEL_MAX             9     // Maximum selectable level
#define DAY_SCORE_MAX         9999  // dayScoreCounter upper clamp
#define MAX_HISTORY_ENTRIES   100   // Max entries in dayScoreHistory[]

// =============================================================================
// Timing — Button  (all values in milliseconds)
// =============================================================================
#define BTN_DEBOUNCE_MS           50    // Min press duration to count as a short press
#define BTN_LONG_PRESS_MS         500   // Hold duration before long-press activates

// =============================================================================
// Timing — Display Refresh
// =============================================================================
#define DISPLAY_UPDATE_INTERVAL_MS  50  // Loop throttle for OLED redraws (~20 FPS)

// =============================================================================
// Timing — Flash Message
// =============================================================================
#define FLASH_DURATION_MS          1500   // Auto-dismiss duration for feedback messages

// =============================================================================
// Timing — BLE
// =============================================================================
#define BLE_ADVERTISE_TIMEOUT_MS   30000  // Advertising timeout before "BLE Error!"
#define BLE_CONNECTED_DISPLAY_MS   800    // "Smartphone connected!" display duration
#define BLE_RESULT_DISPLAY_MS      2000   // "Sync finished!" / "BLE Error!" display duration

// =============================================================================
// BLE Hardware Parameters
// =============================================================================
#define BLE_TX_POWER_DBM    4    // Transmit power in dBm
#define BLE_ADV_INTERVAL    160  // Advertising interval in 0.625 ms units (= 100 ms)
#define BLE_JSON_BUF_SIZE   512  // Max JSON payload for dayScoreHistory characteristic
#define BLE_STATUS_MAX_LEN  32   // Max bytes for status characteristic

// =============================================================================
// Display Layout
// =============================================================================
#define OLED_I2C_ADDR        0x3C  // I2C address of the SSD1306 OLED
#define HISTORY_VISIBLE_ROWS 2     // Number of history rows visible at once

#endif
