/*
 * Pred_Ai — Wireless Surveillance Awareness Logger
 * --------------------------------------------------
 * A PASSIVE, RECEIVE-ONLY ESP32 tool.
 *
 * Target board: ESP32-S3 (e.g. LAFVIN AI kit). Runs on the kit alone (live TFT
 * screen + LED/buzzer alerts); GPS and microSD are OPTIONAL and light up
 * automatically if you wire them later.
 *
 * What it does:
 *   - Scans for nearby WiFi access points and BLE devices (listen only).
 *   - Tags likely cameras / IoT / trackers by MAC vendor (OUI) and BLE signature.
 *   - Flags devices from known camera/tracker vendors, and devices that appear
 *     to be FOLLOWING you:
 *       * with GPS:  seen at locations far apart while staying with you.
 *       * without GPS: signal strength keeps RISING (it's closing on you) — so
 *         "following" detection still works on the bare kit.
 *   - ALERTS you the moment a device is flagged: TFT screen + LED + optional
 *     buzzer (and serial).
 *   - If a GPS module is present, stamps sightings with location + UTC time.
 *   - If an SD card is present, logs sightings to CSV and remembers flagged
 *     devices across reboots.
 *
 * What it does NOT do:
 *   - It does NOT transmit, jam, spoof, or spam anything. There is no beacon
 *     flood, no fake APs, no BLE advertising. It only listens.
 *
 * Honest limitations (please read so you trust the device correctly):
 *   - It can only see devices that are POWERED ON and TRANSMITTING on 2.4 GHz.
 *   - It CANNOT detect wired/analog cameras, cameras with their radio off, or
 *     anything recording locally to SD with WiFi disabled.
 *   - It CANNOT hear 5 GHz WiFi. The ESP32-S3 radio is 2.4 GHz only (see the
 *     "2.4G 802.11 b/g/n" marking on the module). 5 GHz needs an ESP32-C5; a
 *     5 GHz antenna does NOT add the band — the chip's radio decides it.
 *   - Vendor (OUI) matching is a strong hint, not proof. MACs can be randomized
 *     or spoofed, and the OUI table here is a small starter set — expand it.
 *
 * Hardware:
 *   - ESP32-S3 dev board (LAFVIN AI kit)
 *   - 2.0" SPI TFT (ILI9341-class) from the kit  [ENABLE_TFT]
 *   - LED on ALERT_LED_PIN. Optional passive buzzer (ENABLE_BUZZER).
 *   - OPTIONAL later: UART GPS (GPIO 16/17), microSD on SPI (CS GPIO 5).
 *
 *   !! PIN NUMBERS ARE PLACEHOLDERS !! Set the TFT/SD/GPS pins below to match
 *   your LAFVIN shield's silkscreen (MOSI/SCK/CS/DC/RES/BLK etc.). The S3 uses
 *   different default pins than a classic ESP32.
 *
 * Libraries: TinyGPSPlus, Adafruit_GFX + Adafruit_ILI9341 (if ENABLE_TFT),
 * plus the ESP32 Arduino core (WiFi, BLE, SD, SPI).
 */

#include <WiFi.h>
#include "esp_wifi.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <SD.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <math.h>

// ---- Optional 2.0" TFT screen (kit display, ILI9341-class) ----
#define ENABLE_TFT    1          // set to 0 to build without the screen
#if ENABLE_TFT
  #include <Adafruit_GFX.h>
  #include <Adafruit_ILI9341.h>  // if your screen is ST7789, swap this library
  // !! Set these to your LAFVIN shield's TFT pins (silkscreen CS/DC/RES). !!
  #define TFT_CS      10
  #define TFT_DC      9
  #define TFT_RST     8
  Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
#endif

// ---------------- Pin / config ----------------
#define SD_CS_PIN     5          // optional microSD (set to your shield's CS)
#define GPS_RX_PIN    16         // optional GPS
#define GPS_TX_PIN    17
#define GPS_BAUD      9600

#define ALERT_LED_PIN 2          // LED for alerts (wire one if board has none)

#define LOG_PATH      "/sightings.csv"
#define DB_PATH       "/devices.dat"   // persistent tracking table
#define DB_MAGIC      0x50414932UL     // 'PAI2' — db format/version guard
#define BLE_SCAN_SECS 4          // seconds per BLE scan window
#define SCAN_PERIOD   15000UL    // ms between full scan cycles
#define MAX_DEVICES   200        // in-RAM tracking table size
#define MOVE_METERS   75.0       // GPS: seen this far apart => following you
#define APPROACH_DB   12         // no-GPS: signal this many dB stronger than
                                 // first sight => it's closing on you
#define APPROACH_MIN_HITS 3      // need a few sightings before trusting RSSI

// ---- Optional buzzer alert ----
#define ENABLE_BUZZER 0          // set to 1 if a passive buzzer is on BUZZER_PIN
#define BUZZER_PIN    4

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

// ---------------- Known vendor OUIs ----------------
// First 3 bytes of a MAC identify the manufacturer. This is a STARTER set of
// camera / IoT / tracker vendors — extend it from https://maclookup.app or the
// IEEE OUI registry. risk: 2 = camera-likely, 1 = IoT/tracker, 0 = info only.
struct OuiEntry { uint8_t oui[3]; const char* name; uint8_t risk; };

const OuiEntry OUI_TABLE[] = {
  {{0x00,0x40,0x8C}, "Axis (camera)",        2},
  {{0xAC,0xCC,0x8E}, "Axis (camera)",        2},
  {{0x00,0x12,0x12}, "Hikvision",            2},
  {{0x44,0x19,0xB6}, "Hikvision",            2},
  {{0xBC,0xAD,0x28}, "Hikvision",            2},
  {{0x00,0x18,0xAE}, "Dahua",                2},
  {{0x3C,0xEF,0x8C}, "Dahua",                2},
  {{0x2C,0xAA,0x8E}, "Wyze",                 2},
  {{0x7C,0x78,0xB2}, "Wyze",                 2},
  {{0x18,0xB4,0x30}, "Nest/Google",          2},
  {{0x64,0x16,0x66}, "Nest/Google",          2},
  {{0x00,0x7E,0x56}, "Ring/Amazon",          2},
  {{0x34,0xD2,0x70}, "Ring/Amazon",          2},
  {{0xD8,0x6C,0x63}, "Google IoT",           1},
  {{0x10,0x5A,0x17}, "Tuya (smart cam/IoT)", 2},
  {{0x50,0x02,0x91}, "Tuya (smart cam/IoT)", 2},
  {{0x68,0x57,0x2D}, "Tuya (smart cam/IoT)", 2},
  {{0xD4,0xA6,0x51}, "Reolink",              2},
  {{0xEC,0x71,0xDB}, "Reolink/Shenzhen",     2},
};
const int OUI_COUNT = sizeof(OUI_TABLE) / sizeof(OUI_TABLE[0]);

// ---------------- Device tracking table ----------------
struct Sighting {
  uint8_t mac[6];
  bool    isBle;
  int8_t  firstRssi;      // signal at first sighting (for approach detection)
  int8_t  bestRssi;
  uint16_t hits;
  double  firstLat, firstLon;
  double  lastLat,  lastLon;
  bool    mobileFlag;     // GPS: seen at >MOVE_METERS apart => following
  bool    approachFlag;   // no-GPS: signal rose sharply => closing on you
  bool    vendorFlag;     // matched a camera/tracker vendor
  bool    alerted;        // we already raised an alert for this device
  char    label[24];      // vendor or BLE label
};

Sighting devices[MAX_DEVICES];
int deviceCount = 0;

double curLat = 0.0, curLon = 0.0;
bool   gpsFix = false;
unsigned long lastScan = 0;
bool   sdReady = false;

BLEScan* pBLEScan = nullptr;

// ---------------- Helpers ----------------
double haversineMeters(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0;
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);
  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(radians(lat1)) * cos(radians(lat2)) * sin(dLon / 2) * sin(dLon / 2);
  return R * 2 * atan2(sqrt(a), sqrt(1 - a));
}

String macToStr(const uint8_t* m) {
  char buf[18];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  return String(buf);
}

String timestamp() {
  if (gps.date.isValid() && gps.time.isValid()) {
    char buf[24];
    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ",
            gps.date.year(), gps.date.month(), gps.date.day(),
            gps.time.hour(), gps.time.minute(), gps.time.second());
    return String(buf);
  }
  return String("uptime+") + String(millis() / 1000) + "s";
}

// Locally administered MAC (randomized) — bit 1 of first octet set.
bool isRandomMac(const uint8_t* m) { return (m[0] & 0x02) != 0; }

// Returns OUI index or -1. Skips randomized MACs (vendor bits are meaningless).
int lookupOui(const uint8_t* m) {
  if (isRandomMac(m)) return -1;
  for (int i = 0; i < OUI_COUNT; i++) {
    if (m[0] == OUI_TABLE[i].oui[0] && m[1] == OUI_TABLE[i].oui[1] &&
        m[2] == OUI_TABLE[i].oui[2]) return i;
  }
  return -1;
}

// ---------------- Persistence (survive reboot) ----------------
void saveTable() {
  if (!sdReady) return;
  File f = SD.open(DB_PATH, FILE_WRITE);   // truncates/overwrites
  if (!f) return;
  uint32_t magic = DB_MAGIC;
  f.write((const uint8_t*)&magic, sizeof(magic));
  f.write((const uint8_t*)&deviceCount, sizeof(deviceCount));
  if (deviceCount > 0)
    f.write((const uint8_t*)devices, sizeof(Sighting) * deviceCount);
  f.close();
}

void loadTable() {
  if (!sdReady || !SD.exists(DB_PATH)) return;
  File f = SD.open(DB_PATH, FILE_READ);
  if (!f) return;
  uint32_t magic = 0;
  int count = 0;
  if (f.read((uint8_t*)&magic, sizeof(magic)) == sizeof(magic) && magic == DB_MAGIC &&
      f.read((uint8_t*)&count, sizeof(count)) == sizeof(count) &&
      count >= 0 && count <= MAX_DEVICES) {
    int want = sizeof(Sighting) * count;
    if (f.read((uint8_t*)devices, want) == want) deviceCount = count;
  } else {
    Serial.println("Ignoring incompatible/old device DB (will be rewritten).");
  }
  f.close();
  Serial.printf("Loaded %d remembered devices from SD.\n", deviceCount);
}

// ---------------- TFT screen ----------------
#if ENABLE_TFT
bool isFollowing(const Sighting* s) { return s->mobileFlag || s->approachFlag; }

void tftBanner(const char* text, uint16_t color) {
  tft.fillRect(0, 0, tft.width(), 22, color);
  tft.setTextColor(ILI9341_BLACK, color);
  tft.setTextSize(2);
  tft.setCursor(4, 4);
  tft.print(text);
}

// Live "radar": flagged devices first, colour-coded, with a status footer.
void drawScreen() {
  int flagged = 0;
  for (int i = 0; i < deviceCount; i++)
    if (devices[i].vendorFlag || isFollowing(&devices[i])) flagged++;

  tft.fillScreen(ILI9341_BLACK);
  char hdr[40];
  snprintf(hdr, sizeof(hdr), "Pred_Ai  %d flagged", flagged);
  tftBanner(hdr, flagged ? ILI9341_RED : ILI9341_DARKGREEN);

  tft.setTextSize(1);
  int y = 28;
  for (int i = 0; i < deviceCount && y < tft.height() - 12; i++) {
    Sighting* s = &devices[i];
    bool follow = isFollowing(s);
    if (!(s->vendorFlag || follow)) continue;
    tft.setTextColor(follow ? ILI9341_RED : ILI9341_YELLOW, ILI9341_BLACK);
    tft.setCursor(2, y);
    tft.printf("%-6s %-16s %ddB", follow ? "FOLLOW" : "VENDOR", s->label, s->bestRssi);
    y += 12;
  }

  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(2, tft.height() - 10);
  tft.printf("seen:%d  GPS:%s  SD:%s", deviceCount, gpsFix ? "Y" : "N", sdReady ? "Y" : "N");
}

void tftAlertFlash() { tft.fillScreen(ILI9341_RED); delay(150); }
#endif

// ---------------- Alerts ----------------
void localAlert() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(ALERT_LED_PIN, HIGH);
#if ENABLE_BUZZER
    tone(BUZZER_PIN, 2200, 80);
#endif
    delay(90);
    digitalWrite(ALERT_LED_PIN, LOW);
    delay(90);
  }
}

void raiseAlert(const Sighting* s, const char* reason) {
  String msg = String("ALERT: ") + reason + " " +
               (s->isBle ? "BLE " : "WIFI ") + macToStr(s->mac) +
               " [" + s->label + "] rssi=" + s->bestRssi +
               " @ " + String(curLat, 6) + "," + String(curLon, 6) +
               " " + timestamp();
  Serial.print("*** "); Serial.println(msg);
#if ENABLE_TFT
  tftAlertFlash();
#endif
  localAlert();
}

// ---------------- Core: record a sighting ----------------
void recordDevice(const uint8_t* mac, bool isBle, int rssi, const char* label, bool vendorHit) {
  // Find existing entry.
  Sighting* s = nullptr;
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].isBle == isBle && memcmp(devices[i].mac, mac, 6) == 0) {
      s = &devices[i];
      break;
    }
  }

  if (!s) {
    if (deviceCount >= MAX_DEVICES) return;  // table full; oldest stays
    s = &devices[deviceCount++];
    memcpy(s->mac, mac, 6);
    s->isBle = isBle;
    s->firstRssi = rssi;
    s->bestRssi = rssi;
    s->hits = 0;
    s->firstLat = curLat; s->firstLon = curLon;
    s->mobileFlag = false;
    s->approachFlag = false;
    s->vendorFlag = vendorHit;
    s->alerted = false;
    strncpy(s->label, label, sizeof(s->label) - 1);
    s->label[sizeof(s->label) - 1] = '\0';
  }

  s->hits++;
  if (rssi > s->bestRssi) s->bestRssi = rssi;
  s->lastLat = curLat; s->lastLon = curLon;
  if (vendorHit) s->vendorFlag = true;

  // Following detection #1 (needs GPS): seen far from where we first saw it.
  if (gpsFix && (s->firstLat != 0.0 || s->firstLon != 0.0)) {
    double d = haversineMeters(s->firstLat, s->firstLon, curLat, curLon);
    if (d > MOVE_METERS) s->mobileFlag = true;
  }
  // Following detection #2 (no GPS needed): signal climbed sharply over several
  // sightings => the device is closing the distance on you.
  if (s->hits >= APPROACH_MIN_HITS && (rssi - s->firstRssi) >= APPROACH_DB)
    s->approachFlag = true;

  logSighting(s, rssi, label);

  // Alert once, the first time a device becomes noteworthy. A "following"
  // signal is stronger than a vendor match, so prefer that wording.
  bool follow = s->mobileFlag || s->approachFlag;
  if ((s->vendorFlag || follow) && !s->alerted) {
    raiseAlert(s, follow ? "FOLLOWING device" : "known vendor device");
    s->alerted = true;
  }
}

void logSighting(Sighting* s, int rssi, const char* label) {
  String flag = "";
  if (s->vendorFlag)   flag += "VENDOR ";
  if (s->mobileFlag)   flag += "FOLLOWING ";
  if (s->approachFlag) flag += "APPROACHING ";
  if (flag == "")      flag = "-";

  String line = timestamp() + "," +
                String(curLat, 6) + "," + String(curLon, 6) + "," +
                (s->isBle ? "BLE" : "WIFI") + "," +
                macToStr(s->mac) + "," +
                String(label) + "," +
                String(rssi) + "," +
                String(s->hits) + "," +
                flag;

  Serial.println(line);

  if (sdReady) {
    File f = SD.open(LOG_PATH, FILE_APPEND);
    if (f) { f.println(line); f.close(); }
  }
}

// ---------------- WiFi scan (passive) ----------------
void scanWifi() {
  int n = WiFi.scanNetworks(false /*sync*/, true /*show hidden*/);
  for (int i = 0; i < n; i++) {
    uint8_t* bssid = WiFi.BSSID(i);
    if (!bssid) continue;
    int oui = lookupOui(bssid);
    bool vendorHit = (oui >= 0 && OUI_TABLE[oui].risk >= 1);
    String ssid = WiFi.SSID(i);
    char label[24];
    if (oui >= 0) {
      strncpy(label, OUI_TABLE[oui].name, sizeof(label) - 1);
      label[sizeof(label) - 1] = '\0';
    } else if (ssid.length() > 0) {
      snprintf(label, sizeof(label), "wifi:%s", ssid.c_str());
    } else {
      strncpy(label, "wifi:<hidden>", sizeof(label) - 1);
      label[sizeof(label) - 1] = '\0';
    }
    recordDevice(bssid, false, WiFi.RSSI(i), label, vendorHit);
  }
  WiFi.scanDelete();
}

// ---------------- BLE scan (passive) ----------------
void scanBle() {
  BLEScanResults* results = pBLEScan->start(BLE_SCAN_SECS, false);
  int n = results->getCount();
  for (int i = 0; i < n; i++) {
    BLEAdvertisedDevice d = results->getDevice(i);
    BLEAddress addr = d.getAddress();
    const uint8_t* native = *addr.getNative();  // 6-byte MAC

    char label[24] = "ble";
    bool vendorHit = false;

    // BLE manufacturer-data signatures for common trackers.
    if (d.haveManufacturerData()) {
      String md = d.getManufacturerData();
      if (md.length() >= 2) {
        uint16_t company = (uint8_t)md[0] | ((uint8_t)md[1] << 8);
        if (company == 0x004C) {            // Apple
          // 0x12 0x19 = Find My / offline finding (AirTag-class).
          if (md.length() >= 4 && (uint8_t)md[2] == 0x12) {
            strncpy(label, "AirTag/FindMy?", sizeof(label) - 1);
            vendorHit = true;
          } else {
            strncpy(label, "Apple BLE", sizeof(label) - 1);
          }
        } else if (company == 0x0075) {
          strncpy(label, "Samsung BLE", sizeof(label) - 1);
        }
        label[sizeof(label) - 1] = '\0';
      }
    }

    // Service UUIDs used by trackers.
    if (d.haveServiceUUID()) {
      String u = d.getServiceUUID().toString().c_str();
      if (u.indexOf("feed") >= 0 || u.indexOf("feec") >= 0) {
        strncpy(label, "Tile tracker", sizeof(label) - 1); label[sizeof(label)-1]='\0';
        vendorHit = true;
      } else if (u.indexOf("fd5a") >= 0) {
        strncpy(label, "Samsung SmartTag", sizeof(label) - 1); label[sizeof(label)-1]='\0';
        vendorHit = true;
      }
    }

    // Fall back to advertised name if we still have nothing useful.
    if (!vendorHit && d.haveName() && d.getName().length() > 0) {
      snprintf(label, sizeof(label), "ble:%s", d.getName().c_str());
    }

    recordDevice(native, true, d.getRSSI(), label, vendorHit);
  }
  pBLEScan->clearResults();  // free heap between scans
}

// ---------------- Serial review commands ----------------
void printDevice(const Sighting* s) {
  bool flagged = s->vendorFlag || s->mobileFlag || s->approachFlag;
  Serial.printf("  %s %s rssi=%d hits=%u %s%s%s%s  [%s]\n",
                s->isBle ? "BLE " : "WIFI",
                macToStr(s->mac).c_str(),
                s->bestRssi, s->hits,
                s->vendorFlag ? "VENDOR " : "",
                s->mobileFlag ? "FOLLOWING " : "",
                s->approachFlag ? "APPROACHING " : "",
                flagged ? "" : "-",
                s->label);
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "help" || cmd == "?") {
    Serial.println("Commands: list | flagged | gps | save | clear | testalert | help");
  } else if (cmd == "list") {
    Serial.printf("== %d devices tracked ==\n", deviceCount);
    for (int i = 0; i < deviceCount; i++) printDevice(&devices[i]);
  } else if (cmd == "flagged") {
    Serial.println("== flagged (vendor / following / approaching) ==");
    int c = 0;
    for (int i = 0; i < deviceCount; i++)
      if (devices[i].vendorFlag || devices[i].mobileFlag || devices[i].approachFlag) {
        printDevice(&devices[i]); c++;
      }
    Serial.printf("(%d flagged)\n", c);
  } else if (cmd == "gps") {
    Serial.printf("GPS fix=%s lat=%.6f lon=%.6f sats=%d\n",
                  gpsFix ? "yes" : "no", curLat, curLon,
                  gps.satellites.isValid() ? gps.satellites.value() : 0);
  } else if (cmd == "save") {
    saveTable();
    Serial.println("Table saved to SD.");
  } else if (cmd == "clear") {
    deviceCount = 0;
    saveTable();
    Serial.println("Table cleared (SD CSV log kept).");
  } else if (cmd == "testalert") {
    Serial.println("Firing test alert...");
    localAlert();
  } else if (cmd.length() > 0) {
    Serial.println("Unknown. Type 'help'.");
  }
}

void serialEvent() {
  handleCommand(Serial.readStringUntil('\n'));
}

// ---------------- Setup / loop ----------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nPred_Ai Surveillance Awareness Logger (passive / receive-only)");

  pinMode(ALERT_LED_PIN, OUTPUT);
  digitalWrite(ALERT_LED_PIN, LOW);

#if ENABLE_TFT
  tft.begin();
  tft.setRotation(1);              // landscape, 320x240
  tft.fillScreen(ILI9341_BLACK);
  tftBanner("Pred_Ai booting...", ILI9341_DARKGREEN);
#endif

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  sdReady = SD.begin(SD_CS_PIN);
  if (sdReady) {
    Serial.println("SD: ready");
    File f = SD.open(LOG_PATH, FILE_APPEND);
    if (f) {
      if (f.size() == 0)
        f.println("timestamp,lat,lon,type,mac,label,rssi,hits,flags");
      f.close();
    }
    loadTable();   // restore remembered devices from a previous run
  } else {
    Serial.println("SD: NOT found (logging to serial only)");
  }

  // WiFi in station mode = scan/listen only. No AP, no transmit beacons.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  BLEDevice::init("");                 // init once, never advertise
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  Serial.println("Ready. Type 'help' for review commands.");
}

void loop() {
  // Keep GPS parser fed continuously.
  while (gpsSerial.available() > 0) gps.encode(gpsSerial.read());
  if (gps.location.isUpdated() && gps.location.isValid()) {
    curLat = gps.location.lat();
    curLon = gps.location.lng();
    gpsFix = true;
  }

  if (millis() - lastScan > SCAN_PERIOD) {
    lastScan = millis();
    scanWifi();    // WiFi and BLE share one radio; run sequentially.
    scanBle();
    saveTable();   // persist so flagged devices survive a reboot (if SD present)
#if ENABLE_TFT
    drawScreen();  // refresh the live radar
#endif
  }
}
