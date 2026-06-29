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

## Hardware — verified LAFVIN ESP32-S3 AI Chatbot pin map

Pins below are taken from LAFVIN's own board source (the display is **ST7789**,
240×320, colour-inverted — *not* ILI9341). They're already set in the sketch.

| Signal              | GPIO | Notes                              |
|---------------------|------|------------------------------------|
| TFT MOSI            | 40   | ST7789 SPI                         |
| TFT SCLK            | 41   |                                    |
| TFT CS              | 47   |                                    |
| TFT DC              | 39   |                                    |
| TFT RST             | 4    |                                    |
| TFT Backlight       | 42   | driven HIGH = on                   |
| Speaker amp enable  | 48   | **PA enable, NOT an LED** (ES8311) |
| Codec I2C SDA/SCL   | 1/2  | ES8311 @ 0x18                      |
| I2S MCLK/BCLK/WS/DOUT| 38/14/13/45 | speaker audio               |
| GPS (optional)      | 16/17| UART, 9600 baud                    |
| microSD (optional)  | 5    | not in base kit                    |

> ⚠ GPIO 48 on this board is the **speaker amplifier enable**, not a status LED.
> The sketch drives it for audio. If you want a discrete blink LED, wire one to a
> free GPIO and set `ALERT_LED_PIN` to it (default `-1` = none).

> If the screen is rotated/upside-down, change `tft.setRotation(3)` to `1`. If
> colours look inverted, toggle `tft.invertDisplay(true/false)` in `setup()`.

## Software

1. Arduino IDE with the **ESP32 board package** (core **3.x**), board = **ESP32-S3**,
   **PSRAM enabled** (module is N16R8 = 16 MB flash / 8 MB PSRAM).
2. Install libraries: **TinyGPSPlus**, **Adafruit GFX**, **Adafruit ST7789**.
3. Open `surveillance_detector.ino`, upload.
4. Open Serial Monitor at **115200 baud**.

> Set `ENABLE_TFT 0` to build headless (serial only) if you haven't installed the
> Adafruit libraries yet. The watchdog auto-adapts to core 2.x vs 3.x.

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
  - `VENDOR` — matches a known camera/tracker brand by **MAC vendor (OUI)** *or*
    by **SSID/BLE-name keyword** (e.g. an SSID containing `reolink`, `ipcam`,
    `tapo`, or a BLE name containing `airtag`, `tile`). Keyword + OUI together
    catch far more than OUI alone.
  - `FOLLOWING` — *(needs GPS)* moved with you (> `MOVE_METERS`), sustained over
    several scan cycles and `APPROACH_MIN_MS`.
  - `APPROACHING` — *(no GPS needed)* its **smoothed** signal climbed ≥
    `APPROACH_DB` dB across ≥ `APPROACH_MIN_CYC` cycles over ≥ `APPROACH_MIN_MS` —
    i.e. it's been closing on you for a while, not a one-off spike. This is how
    "something is following me" works on the bare kit.

### Alerts (screen + speaker)

When a device is **first flagged**, you're warned three ways at once — no need to
read the SD card:

1. **Full-screen TFT alert** that spells out the threat in big text —
   `! ALERT` + `FOLLOWING YOU` / `APPROACHING YOU` / `CAMERA / TRACKER`, with the
   device label, MAC, signal, and time.
2. **Speaker tones** through the kit's ES8311 codec — distinct patterns per
   threat (urgent triple = following, rising couplet = approaching, double-beep =
   camera/tracker). A startup chime on boot confirms the speaker works.
3. Optional discrete **LED** if you wired one (`ALERT_LED_PIN`).

Each device alerts **once** so you aren't spammed on every re-sighting.
`testalert` (serial) fires the screen + speaker so you can verify audio.

> **Audio needs no extra libraries** — it uses the ESP32 core's built-in I2S
> driver, so the sketch compiles as-is. If you hear nothing, type `testalert`
> and watch Serial: `[AUDIO] ES8311 not found` means the codec didn't ACK on
> I2C (check the board), otherwise the ES8311 register init may need a tweak.
> Set `ENABLE_AUDIO 0` to build silent (screen + serial only).

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
| `testalert` | Fire the screen alert + speaker (verify audio)  |
| `help`      | Show commands                                   |

---

## Tuning

Edit the `#define`s near the top of the sketch. **Higher thresholds = fewer
false alarms; lower = more sensitive.**

- `SCAN_PERIOD` — ms between scan cycles (default 12 s).
- `BLE_SCAN_SECS` — length of each BLE listen window.
- `APPROACH_DB` — dB the smoothed signal must rise to count as approaching (10).
- `APPROACH_MIN_CYC` — min scan cycles a device must persist before it can be
  flagged approaching/following (4) — the main transient-killer.
- `APPROACH_MIN_MS` — min time a device must be around first (60 s).
- `EMA_ALPHA` — RSSI smoothing (0.4). Lower = smoother/slower, fewer spikes.
- `MOVE_METERS` — GPS distance that counts as "following you."
- `MAX_DEVICES` — RAM tracking-table size (oldest **unflagged** device is
  recycled when full, so flagged threats are never forgotten on a long walk).

### Extending detection (two ways)

1. **`OUI_TABLE[]`** — MAC vendor prefixes (first 3 bytes). Add rows from the
   IEEE OUI registry / maclookup.app for more camera brands.
2. **`SSID_KEYWORDS[]` / `BLE_KEYWORDS[]`** — substrings matched against WiFi
   SSIDs and BLE names. Often catches devices whose MAC is randomized or whose
   vendor isn't in the OUI table. Add brand words you care about.

---

## Roadmap ideas (all still receive-only)

- Speaker tones through the kit's I2S audio codec (richer than a buzzer).
- On-screen detail view: tap a button to see a device's history/RSSI trend.
- WiFi probe-request sniffing (promiscuous RX) to see what *your own* devices leak.
- Auto-expiring stale devices from the table so it doesn't fill up on long walks.
