#!/usr/bin/env bash
# =============================================================================
# Pred_Ai Guardian — one-shot setup (macOS / Linux)
# Installs Arduino CLI + the exact ESP32-S3 core + all libraries, then compiles.
#
#   ./bootstrap.sh                 install everything and compile (proves it builds)
#   ./bootstrap.sh /dev/ttyACM0    ...and flash to that serial port
#
# Find your port with:  arduino-cli board list   (after this runs once)
# =============================================================================
set -euo pipefail

# ---- settings you can tweak ----
# N16R8 module = 16 MB flash + OCTAL (OPI) PSRAM. CDCOnBoot=cdc => Serial over the
# native USB-C port. If Serial Monitor is blank, try the other USB port or set
# CDCOnBoot=default below.
FQBN="esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc,PartitionScheme=default"
ESP32_URL="https://espressif.github.io/arduino-esp32/package_esp32_index.json"

SKETCH_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${1:-}"

echo "==> Pred_Ai bootstrap (sketch: $SKETCH_DIR)"

# 1) Arduino CLI -------------------------------------------------------------
if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "==> Installing arduino-cli into ~/bin ..."
  mkdir -p "$HOME/bin"
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$HOME/bin" sh
  export PATH="$HOME/bin:$PATH"
  echo "    (add ~/bin to your PATH to use 'arduino-cli' directly later)"
fi
echo "    $(arduino-cli version)"

# 2) Board manager + index ---------------------------------------------------
arduino-cli config init --overwrite >/dev/null 2>&1 || true
arduino-cli config set board_manager.additional_urls "$ESP32_URL"
arduino-cli core update-index

# 3) ESP32 core (3.x — big download, be patient) -----------------------------
echo "==> Installing ESP32 core ..."
arduino-cli core install esp32:esp32

# 4) Libraries (exact set the sketch needs) ----------------------------------
echo "==> Installing libraries ..."
arduino-cli lib update-index
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit BusIO"
arduino-cli lib install "Adafruit ST7735 and ST7789 Library"
arduino-cli lib install "ESP8266Audio"     # dependency of ESP8266SAM
arduino-cli lib install "ESP8266SAM"        # spoken-voice alerts
# Only needed if you set ENABLE_GPS 1 in the sketch:
# arduino-cli lib install "TinyGPSPlus"

# 5) Compile (proves the toolchain + libs are correct) -----------------------
echo "==> Compiling ..."
arduino-cli compile --fqbn "$FQBN" "$SKETCH_DIR"
echo "==> BUILD OK"

# 6) Flash (optional) --------------------------------------------------------
if [ -n "$PORT" ]; then
  echo "==> Flashing to $PORT ..."
  arduino-cli upload --fqbn "$FQBN" -p "$PORT" "$SKETCH_DIR"
  echo "==> Flashed. Open the serial monitor with:"
  echo "    arduino-cli monitor -p $PORT -c baudrate=115200"
else
  echo
  echo "Build succeeded. To flash, plug in the board and run:"
  echo "    arduino-cli board list          # find the port (e.g. /dev/ttyACM0)"
  echo "    ./bootstrap.sh <that-port>"
fi
