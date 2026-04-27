#include "ble_connection.h"
#include "display.h"
#include "scoring.h"
#include <bluefruit.h>

// Custom 128-bit UUIDs for PeakPoint Data service and characteristics
#define PP_SERVICE_UUID       "12345678-1234-1234-1234-1234567890AB"
#define PP_DAYS_HISTORY_UUID  "12345678-1234-1234-1234-1234567890AC"
#define PP_STATUS_UUID        "12345678-1234-1234-1234-1234567890AD"

static BLEService        ppService(PP_SERVICE_UUID);
static BLECharacteristic daysHistoryChar(PP_DAYS_HISTORY_UUID);
static BLECharacteristic statusChar(PP_STATUS_UUID);

static unsigned long bleStartTime = 0;
static bool          bleDataSent  = false;

// Serialize daysHistory[] into a JSON string
static void serializeDaysHistory(char* buf, size_t bufSize) {
  size_t pos = 0;
  pos += snprintf(buf + pos, bufSize - pos, "[");

  for (uint16_t d = 0; d < daysHistoryCount && pos < bufSize - 2; d++) {
    if (d > 0) pos += snprintf(buf + pos, bufSize - pos, ",");
    pos += snprintf(buf + pos, bufSize - pos,
      "{\"date\":\"%s\",\"score\":%u,\"history\":{",
      daysHistory[d].date,
      daysHistory[d].score);

    for (uint16_t h = 0; h < daysHistory[d].historyCount && pos < bufSize - 2; h++) {
      if (h > 0) pos += snprintf(buf + pos, bufSize - pos, ",");
      pos += snprintf(buf + pos, bufSize - pos,
        "\"%s\":\"%u\"",
        daysHistory[d].history[h].timestamp,
        daysHistory[d].history[h].level);
    }

    pos += snprintf(buf + pos, bufSize - pos, "}}");
  }

  snprintf(buf + pos, bufSize - pos, "]");
}

static void writeStatus(const char* value) {
  statusChar.write(value, strlen(value));
}

void bleInit() {
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("PeakPoint");

  ppService.begin();

  // Characteristic_1: daysHistory - readable, max 512 bytes
  daysHistoryChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  daysHistoryChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  daysHistoryChar.setMaxLen(512);
  daysHistoryChar.begin();

  // Characteristic_2: status - readable short string
  statusChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  statusChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  statusChar.setMaxLen(32);
  statusChar.begin();
  writeStatus("idle");

  // BLE radio stays off until bleStart() is called
  Bluefruit.Advertising.stop();
}

void bleStart() {
  bleStartTime = millis();
  bleDataSent  = false;

  writeStatus("advertising");

  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(ppService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(false);
  Bluefruit.Advertising.setInterval(160, 160); // 100ms intervals
  Bluefruit.Advertising.setFastTimeout(BLE_ADVERTISE_TIMEOUT_MS / 1000);
  Bluefruit.Advertising.start(BLE_ADVERTISE_TIMEOUT_MS / 1000);

  displayBleConnecting();
}

void bleStop() {
  Bluefruit.Advertising.stop();
  if (Bluefruit.connected()) {
    Bluefruit.disconnect(Bluefruit.connHandle());
  }
  writeStatus("idle");
}

// Returns true when the BLE session is fully complete (success or timeout)
bool bleUpdate() {
  unsigned long now = millis();

  if (Bluefruit.connected()) {
    if (!bleDataSent) {
      bleDataSent = true;

      // Show connected message briefly
      displayBleConnected();
      writeStatus("connected");
      delay(800);

      // Serialize and transmit
      displayBleSyncing();
      writeStatus("syncing");

      char jsonBuf[512];
      serializeDaysHistory(jsonBuf, sizeof(jsonBuf));
      bool ok = daysHistoryChar.write(jsonBuf, strlen(jsonBuf));

      if (ok) {
        writeStatus("synced");
        displayBleSyncDone();
      } else {
        writeStatus("error");
        displayBleError();
      }

      delay(2000); // keep result visible briefly
      bleStop();
      return true;
    }
  } else {
    // Not connected - check for timeout
    bool timedOut = (now - bleStartTime >= (unsigned long)BLE_ADVERTISE_TIMEOUT_MS + 2000UL);
    bool advStopped = !Bluefruit.Advertising.isRunning();

    if (timedOut || (advStopped && !bleDataSent)) {
      displayBleError();
      delay(2000);
      bleStop();
      return true;
    }
  }

  return false;
}
