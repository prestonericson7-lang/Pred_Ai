# Pred_Ai — Wireless Surveillance Awareness Logger (ESP32)

A **passive, receive-only** ESP32 tool that listens for nearby WiFi and BLE
devices, tags likely **cameras / IoT / trackers**, stamps each sighting with
**GPS location + UTC time**, logs it to an SD card, and **flags devices that
follow you** across distinct locations.

It only **listens**. It does not transmit, jam, spoof, or broadcast anything —
there is no beacon flooding, no fake access points, no BLE advertising. That is
deliberate: those behaviors are illegal spectrum interference and would put
*you* on the map, not hide you.

---

## What it can and cannot detect

**Can detect** (anything powered on and transmitting on 2.4 GHz):
- WiFi cameras (Hikvision, Dahua, Wyze, Nest, Ring, Reolink, Tuya, Axis, …)
  identified by their MAC vendor prefix (OUI).
- BLE trackers — AirTag / Find My, Tile, Samsung SmartTag — by their BLE
  advertising signature.
- Any other WiFi AP or BLE device nearby (logged, not necessarily flagged).

**Cannot detect** (be honest with yourself about the blind spots):
- Wired or analog hidden cameras with no radio.
- Cameras recording locally with WiFi turned off.
- Anything not transmitting at the moment you pass by.
- Devices using randomized MACs are logged but cannot be vendor-identified.

A flag means **"worth a closer look,"** never "confirmed surveillance."

---

## Hardware

| Part            | Connection                          |
|-----------------|-------------------------------------|
| ESP32 dev board | —                                   |
| microSD (SPI)   | CS → GPIO 5 (MOSI/MISO/SCK = default SPI) |
| GPS (UART)      | GPS TX → GPIO 16, GPS RX → GPIO 17, 9600 baud |

Pins are `#define`d at the top of the sketch — change them to match your wiring.

## Software

1. Arduino IDE with the **ESP32 board package** installed (targets **core 3.x**;
   see the note below if you are on 2.x).
2. Install the **TinyGPSPlus** library (Library Manager).
3. Open `surveillance_detector.ino`, select your ESP32 board, upload.
4. Open Serial Monitor at **115200 baud**.

> **Core-version note:** on ESP32 Arduino core **3.x**, `BLEScan::start()` returns
> a `BLEScanResults*` (pointer) and BLE string getters return `String` — that is
> what this sketch uses. On older **2.x** cores `start()` returns a value: change
> `BLEScanResults* results = pBLEScan->start(...)` to
> `BLEScanResults results = pBLEScan->start(...)` and use `results.` instead of
> `results->`.

---

## Using it

Sightings stream to the serial monitor and to `/sightings.csv` on the SD card:

```
timestamp,lat,lon,type,mac,label,rssi,hits,flags
2026-06-28T14:03:11Z,40.712776,-74.005974,WIFI,2C:AA:8E:11:22:33,Wyze,-58,3,VENDOR
2026-06-28T14:05:02Z,40.713900,-74.004100,BLE,D1:A2:B3:C4:D5:E6,AirTag/FindMy?,-71,9,VENDOR FOLLOWING
```

- `rssi` is signal strength — closer to 0 means physically closer to you.
- `hits` is how many times that device has been seen.
- `flags`:
  - `VENDOR` — MAC/BLE signature matches a known camera or tracker brand.
  - `FOLLOWING` — the same device was seen at two locations more than
    `MOVE_METERS` (default 75 m) apart, i.e. it is moving *with* you. For a
    tracker, this is the signal that actually matters.

### Serial review commands

| Command   | Action                                            |
|-----------|---------------------------------------------------|
| `list`    | All devices currently tracked in RAM              |
| `flagged` | Only vendor-matched or following devices          |
| `gps`     | Current GPS fix / coordinates / satellite count   |
| `clear`   | Reset the in-RAM table (SD log is kept)           |
| `help`    | Show commands                                     |

---

## Tuning

Edit the `#define`s near the top of the sketch:

- `SCAN_PERIOD` — ms between scan cycles (default 15 s).
- `BLE_SCAN_SECS` — length of each BLE listen window.
- `MOVE_METERS` — distance that counts as "following you."
- `MAX_DEVICES` — RAM tracking-table size.

### Extending vendor detection

`OUI_TABLE[]` is a **starter** list. MAC vendor prefixes (the first 3 bytes)
are public — look them up at the IEEE OUI registry or maclookup.app and add
rows for any cameras/brands you want to catch. More entries = better coverage.

---

## Roadmap ideas (all still receive-only)

- Buzzer/LED alert the moment a `FOLLOWING` device is confirmed.
- Persist the device table to SD so "following" survives a reboot.
- WiFi probe-request sniffing (promiscuous RX) to see what *your own* devices
  leak.
- Optional AES-encrypted log (mbedTLS is already on the ESP32) instead of plain
  CSV, if the log itself is sensitive.
