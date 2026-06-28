# Pred_Ai — Wireless Surveillance Awareness Logger (ESP32-S3)

A **passive, receive-only** tool that listens for nearby WiFi and BLE devices,
tags likely **cameras / IoT / trackers**, and **alerts you on a screen** the
moment something looks like it's a known camera vendor or is **following you**.

Built for the **LAFVIN ESP32-S3 AI kit** — it runs on the kit alone (live TFT
screen + LED alert). **GPS and microSD are optional** and light up automatically
if you wire them later.

It only **listens**. It does not transmit, jam, spoof, or broadcast anything —
no beacon flooding, no fake access points, no BLE advertising. Those behaviors
are illegal spectrum interference and would put *you* on the map, not hide you.

## About "5G" — important hardware truth

The band a radio can hear is fixed in the **transceiver chip**, not the antenna
or a plug-in card:

- Your **ESP32-S3 is 2.4 GHz only** — it says so right on the module
  (`2.4G 802.11 b/g/n`). A 5 GHz antenna will *not* add 5 GHz; it only detunes
  the 2.4 GHz reception. To hear **5 GHz WiFi cameras** you'd need a different
  chip (**ESP32-C5**) or a Raspberry Pi + 5 GHz USB adapter.
- A laptop **Intel WiFi card** speaks PCIe and needs an OS driver; the ESP32
  cannot drive it.
- **"5G" cellular** is a separate radio that needs a modem module + SIM. We
  removed that path — this build uses **no SIM and no cellular**.

The good news: most WiFi cameras and **all** BLE trackers (AirTags, Tile,
SmartTags) live on 2.4 GHz, so the S3 still catches a lot.

---

## What it can and cannot detect

**Can detect** (anything powered on and transmitting on 2.4 GHz):
- WiFi cameras (Hikvision, Dahua, Wyze, Nest, Ring, Reolink, Tuya, Axis, …) by
  their MAC vendor prefix (OUI).
- BLE trackers — AirTag / Find My, Tile, Samsung SmartTag — by their BLE
  advertising signature.
- Any other WiFi AP or BLE device nearby (logged, not necessarily flagged).

**Cannot detect** (know the blind spots so you trust it correctly):
- Wired/analog hidden cameras with no radio.
- Cameras recording locally with WiFi turned off.
- Anything on **5 GHz** (see above), or not transmitting as you pass.
- Devices using randomized MACs are logged but can't be vendor-identified.

A flag means **"worth a closer look,"** never "confirmed surveillance."

---

## Hardware

| Part                  | Connection                                   |
|-----------------------|----------------------------------------------|
| ESP32-S3 board (kit)  | —                                            |
| 2.0" TFT (kit)        | SPI; set `TFT_CS / TFT_DC / TFT_RST` to your shield's pins |
| Alert LED             | `ALERT_LED_PIN` (wire one to a free GPIO)    |
| Buzzer (optional)     | `BUZZER_PIN` — set `ENABLE_BUZZER 1`         |
| GPS (optional, later) | UART → `GPS_RX_PIN` / `GPS_TX_PIN`, 9600 baud |
| microSD (optional)    | SPI → `SD_CS_PIN`                            |

> **Pins are placeholders.** The ESP32-S3 + LAFVIN shield use different GPIOs
> than a classic ESP32. Set the `#define`s at the top of the sketch to match the
> shield's silkscreen (look for `CS DC RES MOSI SCK BLK` next to the TFT header).

## Software

1. Arduino IDE with the **ESP32 board package** (core **3.x**), board set to your
   **ESP32-S3** (enable PSRAM; this module is N16R8 = 16 MB flash / 8 MB PSRAM).
2. Install libraries: **TinyGPSPlus**, **Adafruit GFX**, **Adafruit ILI9341**.
3. Open `surveillance_detector.ino`, set the TFT pins, upload.
4. Open Serial Monitor at **115200 baud**.

> **If the screen is blank or garbled**, your panel is probably an **ST7789**, not
> ILI9341 — swap `#include <Adafruit_ILI9341.h>` for `Adafruit_ST7789` and the
> constructor accordingly. Set `ENABLE_TFT 0` to build headless (serial only).
>
> **Core note:** on core **3.x** `BLEScan::start()` returns a `BLEScanResults*`
> (what this sketch uses). On older **2.x** cores it returns a value — change
> `BLEScanResults* results = ...` to `BLEScanResults results = ...` and use `.`
> instead of `->`.

---

## Using it

The TFT shows a **live radar**: flagged devices first (red = following/approaching,
yellow = known vendor), with a footer showing how many devices are tracked and
whether GPS/SD are present. New flags flash the screen red and blink the LED.

If an SD card is present, sightings also stream to `/sightings.csv`:

```
timestamp,lat,lon,type,mac,label,rssi,hits,flags
2026-06-28T14:03:11Z,40.712776,-74.005974,WIFI,2C:AA:8E:11:22:33,Wyze,-58,3,VENDOR
2026-06-28T14:05:02Z,40.713900,-74.004100,BLE,D1:A2:B3:C4:D5:E6,AirTag/FindMy?,-71,9,VENDOR APPROACHING
```

- `rssi` — signal strength; closer to 0 = physically closer to you.
- `flags`:
  - `VENDOR` — matches a known camera/tracker brand.
  - `FOLLOWING` — *(needs GPS)* seen at two spots > `MOVE_METERS` apart while
    staying with you.
  - `APPROACHING` — *(no GPS needed)* its signal climbed ≥ `APPROACH_DB` dB over
    several sightings, i.e. it's closing the distance on you. This is how
    "something is following me" works on the bare kit.

### Alerts

When a device is **first flagged**, you're warned immediately — no need to read
the SD card:

- **TFT** flashes red and lists the device; **LED** blinks (+ optional buzzer).
- Each device alerts **once** so you aren't spammed on every re-sighting.
- `testalert` (serial) fires the LED/buzzer to verify wiring.

### Reboot-persistent memory *(needs SD)*

With an SD card, the tracking table is saved to `/devices.dat` and reloaded on
boot, so a device already flagged stays flagged across power cycles. Without SD,
the device still works fully — it just starts fresh each boot.

### Serial review commands

| Command     | Action                                          |
|-------------|-------------------------------------------------|
| `list`      | All devices currently tracked                   |
| `flagged`   | Vendor / following / approaching devices        |
| `gps`       | Current GPS fix / coordinates / satellites      |
| `save`      | Force-save the tracking table (if SD present)   |
| `clear`     | Reset the table (SD CSV log is kept)            |
| `testalert` | Fire the LED/buzzer                             |
| `help`      | Show commands                                   |

---

## Tuning

Edit the `#define`s near the top of the sketch:

- `SCAN_PERIOD` — ms between scan cycles (default 15 s).
- `BLE_SCAN_SECS` — length of each BLE listen window.
- `APPROACH_DB` / `APPROACH_MIN_HITS` — how aggressively to call something
  "approaching" without GPS. Lower dB = more sensitive (more false alarms).
- `MOVE_METERS` — GPS distance that counts as "following you."
- `MAX_DEVICES` — RAM tracking-table size.

### Extending vendor detection

`OUI_TABLE[]` is a **starter** list. MAC vendor prefixes (first 3 bytes) are
public — look them up at the IEEE OUI registry or maclookup.app and add rows for
any camera brands you want to catch. More entries = better coverage.

---

## Roadmap ideas (all still receive-only)

- Speaker tones through the kit's I2S audio codec (richer than a buzzer).
- On-screen detail view: tap a button to see a device's history/RSSI trend.
- WiFi probe-request sniffing (promiscuous RX) to see what *your own* devices leak.
- Auto-expiring stale devices from the table so it doesn't fill up on long walks.
