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

// Serialize dayScoreHistory[] into {"n":<count>,"s":[level,...]}
static void serializeDayScoreHistory(char* buf, size_t bufSize) {
  size_t pos = 0;
  pos += snprintf(buf + pos, bufSize - pos, "{\"n\":%u,\"s\":[", dayScoreHistoryCount);
  for (uint8_t i = 0; i < dayScoreHistoryCount && pos < bufSize - 2; i++) {
    if (i > 0) pos += snprintf(buf + pos, bufSize - pos, ",");
    pos += snprintf(buf + pos, bufSize - pos, "%u", dayScoreHistory[i].level);
  }
  snprintf(buf + pos, bufSize - pos, "]}");
}

static void writeStatus(const char* value) {
  statusChar.write(value, strlen(value));
}

void bleInit() {
  Bluefruit.begin();
  Bluefruit.setTxPower(BLE_TX_POWER_DBM);
  Bluefruit.setName(BLE_DEVICE_NAME);

  // Disable the library's automatic connection-LED management.
  // On XIAO nRF52840 the LED does not turn off reliably after disconnect
  // via the auto handler; the OLED display provides all status feedback.
  Bluefruit.autoConnLed(false);

  ppService.begin();

  // Characteristic_1: daysHistory - readable, max 512 bytes
  daysHistoryChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  daysHistoryChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  daysHistoryChar.setMaxLen(BLE_JSON_BUF_SIZE);
  daysHistoryChar.begin();

  // Characteristic_2: status - readable short string
  statusChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  statusChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  statusChar.setMaxLen(BLE_STATUS_MAX_LEN);
  statusChar.begin();
  writeStatus(BLE_STATUS_IDLE);

  // BLE radio stays off until bleStart() is called
  Bluefruit.Advertising.stop();
}

void bleStart() {
  bleStartTime = millis();
  bleDataSent  = false;

  writeStatus(BLE_STATUS_ADVERTISING);

  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(ppService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(false);
  Bluefruit.Advertising.setInterval(BLE_ADV_INTERVAL, BLE_ADV_INTERVAL);
  Bluefruit.Advertising.setFastTimeout(BLE_ADVERTISE_TIMEOUT_MS / 1000);
  Bluefruit.Advertising.start(BLE_ADVERTISE_TIMEOUT_MS / 1000);

  displayBleConnecting();
}

void bleStop() {
  Bluefruit.Advertising.stop();
  if (Bluefruit.connected()) {
    Bluefruit.disconnect(Bluefruit.connHandle());
  }
  writeStatus(BLE_STATUS_IDLE);
}

bool bleIsConnected() {
  return Bluefruit.connected();
}

// Returns true when the BLE session is fully complete (success or timeout)
bool bleUpdate() {
  unsigned long now = millis();

  if (Bluefruit.connected()) {
    if (!bleDataSent) {
      bleDataSent = true;

      // Show connected message and wait until the phone has enabled CCCD
      // (subscribed to notifications) before writing, so the notification
      // is not lost due to a discovery/setup race condition on the central.
      displayBleConnected();
      writeStatus(BLE_STATUS_CONNECTED);

      unsigned long cccdWait = millis();
      while (!daysHistoryChar.notifyEnabled()) {
        if (millis() - cccdWait > 5000) break; // safety timeout
        delay(20);
      }

      // Ensure the "connected" message was visible for at least the configured time
      unsigned long elapsed = millis() - cccdWait;
      if (elapsed < BLE_CONNECTED_DISPLAY_MS) {
        delay(BLE_CONNECTED_DISPLAY_MS - elapsed);
      }

      // Serialize and transmit
      displayBleSyncing();
      writeStatus(BLE_STATUS_SYNCING);

      char jsonBuf[BLE_JSON_BUF_SIZE];
      serializeDayScoreHistory(jsonBuf, sizeof(jsonBuf));

      // write() updates the GATT attribute value (for fallback READ access).
      // notify() sends the actual BLE notification packet to the subscribed
      // central — without this call the phone's monitor callback never fires.
      daysHistoryChar.write(jsonBuf, strlen(jsonBuf));
      bool ok = daysHistoryChar.notify(jsonBuf, strlen(jsonBuf)) > 0;

      if (ok) {
        writeStatus(BLE_STATUS_SYNCED);
        displayBleSyncDone();
      } else {
        writeStatus(BLE_STATUS_ERROR);
        displayBleError();
      }

      delay(BLE_RESULT_DISPLAY_MS); // keep result visible briefly
      bleStop();
      return true;
    }
  } else {
    // Not connected - check for timeout
    bool timedOut = (now - bleStartTime >= (unsigned long)BLE_ADVERTISE_TIMEOUT_MS + (unsigned long)BLE_RESULT_DISPLAY_MS);
    bool advStopped = !Bluefruit.Advertising.isRunning();

    if (timedOut || (advStopped && !bleDataSent)) {
      displayBleError();
      delay(BLE_RESULT_DISPLAY_MS);
      bleStop();
      return true;
    }
  }

  return false;
}
