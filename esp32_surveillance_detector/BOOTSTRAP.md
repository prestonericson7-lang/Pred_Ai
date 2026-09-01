# Bootstrap — get it building/flashing without dependency headaches

Two ways: the **one-command script** (recommended), or the **manual IDE** list.

## Option A — one command (Arduino CLI)

The script installs Arduino CLI, the exact ESP32-S3 core, and every library, then
compiles to prove it all works. Run it from inside the
`esp32_surveillance_detector` folder.

**macOS / Linux**
```bash
chmod +x bootstrap.sh
./bootstrap.sh                 # install + compile
./bootstrap.sh /dev/ttyACM0    # ...and flash to that port
```

**Windows (PowerShell)**
```powershell
powershell -ExecutionPolicy Bypass -File .\bootstrap.ps1          # install + compile
powershell -ExecutionPolicy Bypass -File .\bootstrap.ps1 COM5     # ...and flash
```

Find the port after the first run with `arduino-cli board list` (it shows
`/dev/ttyACM0`, `/dev/cu.usbmodem…`, or `COM5`). Then open the serial monitor:
```
arduino-cli monitor -p <PORT> -c baudrate=115200
```

## Option B — manual (Arduino IDE)

1. **Boards Manager URL** — *File ▸ Preferences ▸ Additional boards manager URLs*:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. **Boards Manager** — install **esp32 by Espressif Systems** (3.x).
3. **Library Manager** — install these exact libraries:
   - Adafruit GFX Library
   - Adafruit BusIO
   - Adafruit ST7735 and ST7789 Library
   - ESP8266Audio
   - ESP8266SAM
   - *(TinyGPSPlus — only if you set `ENABLE_GPS 1`)*
4. **Board settings** — *Tools*:
   - Board: **ESP32S3 Dev Module**
   - PSRAM: **OPI PSRAM**
   - Flash Size: **16MB (128Mb)**
   - USB CDC On Boot: **Enabled**  *(so the Serial Monitor works over USB-C)*
5. Open `esp32_surveillance_detector.ino`, pick the port, **Upload**.

## What "good" looks like

- A two-note **boot chime** + `Pred_Ai booting...` on the screen.
- Serial @115200 prints `[AUDIO] ES8311 + I2S ready` and `Guardian Detector ready`.
- Type `testalert` → screen flashes, it beeps/speaks. `say hello` → it talks.
- Press **BOOT** → "Shield activated," then it speaks the count.

## If something's off

- **Won't compile / library error** → re-run the script; if only voice fails, set
  `ENABLE_VOICE 0` (falls back to tones, no ESP8266SAM needed).
- **Serial Monitor blank** → try the other USB-C port, or set
  `CDCOnBoot=default` (IDE: *USB CDC On Boot: Disabled*).
- **`[AUDIO] ES8311 not found`** → the codec didn't answer on I2C; send me that
  serial line and I'll adjust.
- **Screen blank/garbled** → flip `tft.setRotation(3)`→`1`, or toggle
  `tft.invertDisplay(...)`.
