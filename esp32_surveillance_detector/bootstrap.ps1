# =============================================================================
# Pred_Ai Guardian — one-shot setup (Windows / PowerShell)
# Installs Arduino CLI + the exact ESP32-S3 core + all libraries, then compiles.
#
#   powershell -ExecutionPolicy Bypass -File .\bootstrap.ps1
#   powershell -ExecutionPolicy Bypass -File .\bootstrap.ps1 COM5   # also flash
#
# Find your port in Device Manager (Ports) or:  arduino-cli board list
# =============================================================================
param([string]$Port = "")

$ErrorActionPreference = "Stop"

# N16R8 = 16 MB flash + OPI PSRAM. CDCOnBoot=cdc => Serial over native USB-C.
$FQBN     = "esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc,PartitionScheme=default"
$Esp32Url = "https://espressif.github.io/arduino-esp32/package_esp32_index.json"
$SketchDir = $PSScriptRoot

Write-Host "==> Pred_Ai bootstrap (sketch: $SketchDir)"

# 1) Arduino CLI
$cli = Get-Command arduino-cli -ErrorAction SilentlyContinue
if (-not $cli) {
  Write-Host "==> Installing arduino-cli into .\bin ..."
  $bin = Join-Path $SketchDir "bin"
  New-Item -ItemType Directory -Force -Path $bin | Out-Null
  $zip = Join-Path $bin "arduino-cli.zip"
  Invoke-WebRequest "https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip" -OutFile $zip
  Expand-Archive -Path $zip -DestinationPath $bin -Force
  $env:Path = "$bin;$env:Path"
}
arduino-cli version

# 2) Board manager + index
cmd /c "arduino-cli config init --overwrite" 2>$null | Out-Null
arduino-cli config set board_manager.additional_urls $Esp32Url
arduino-cli core update-index

# 3) ESP32 core
Write-Host "==> Installing ESP32 core (big download) ..."
arduino-cli core install esp32:esp32

# 4) Libraries
Write-Host "==> Installing libraries ..."
arduino-cli lib update-index
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit BusIO"
arduino-cli lib install "Adafruit ST7735 and ST7789 Library"
arduino-cli lib install "ESP8266Audio"
arduino-cli lib install "ESP8266SAM"
# Only if you set ENABLE_GPS 1:
# arduino-cli lib install "TinyGPSPlus"

# 5) Compile
Write-Host "==> Compiling ..."
arduino-cli compile --fqbn $FQBN $SketchDir
Write-Host "==> BUILD OK"

# 6) Flash (optional)
if ($Port -ne "") {
  Write-Host "==> Flashing to $Port ..."
  arduino-cli upload --fqbn $FQBN -p $Port $SketchDir
  Write-Host "==> Flashed. Serial monitor: arduino-cli monitor -p $Port -c baudrate=115200"
} else {
  Write-Host ""
  Write-Host "Build succeeded. To flash: plug in the board, then run:"
  Write-Host "    arduino-cli board list"
  Write-Host "    .\bootstrap.ps1 <COMx>"
}
