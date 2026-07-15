# VPX-OS

A custom smartwatch firmware for the **[Waveshare ESP32-S3-Touch-AMOLED-2.06](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-2.06)**, built on
[ESP-Brookesia](https://github.com/espressif/esp-brookesia) and LVGL. Forked from Espressif's
ESP-Brookesia phone demo and extended with a real settings experience, Wi-Fi provisioning,
persistent user preferences, and a custom dark theme.

> Fork of the Waveshare / ESP-Brookesia demo — repo: `vpx-code/esp32-s3-touch-amoled-2.06`

---

## ⚠️ Status

Early and actively evolving. Wi-Fi provisioning, auto-connect, brightness persistence, and the
clock are working. The **BLE** section of Settings is a placeholder. Expect rough edges and
in-progress `TODO`s throughout.

## ✨ Features

- **⚙️ Settings app** — a native settings screen with Wi-Fi, BLE, and Display sections.
- **📶 Wi-Fi provisioning** — phone-driven SoftAP setup: the watch brings up an access point
  (`VPX_WATCH`), you join it from your phone, and enter your network credentials in the
  captive-portal page that pops up. No on-device keyboard needed.
- **🔁 Auto-connect on boot** — the last successfully connected access point is stored in NVS
  and reconnected automatically on the next power-up.
- **💡 Persistent brightness** — the display brightness you set survives reboots. It's written
  to NVS when you finish adjusting the slider and re-applied on boot.
- **🕐 Clock app** — a simple time display, wired into the status bar clock.
- **🎨 Custom dark theme** — a bespoke `410×502` dark stylesheet with tuned colors, fonts, and
  a cleaned-up settings menu header.

## 🧩 Hardware

| | |
|---|---|
| **Board** | Waveshare ESP32-S3-Touch-AMOLED-2.06 |
| **SoC** | ESP32-S3 (dual-core, octal PSRAM, 16 MB flash) |
| **Display** | 2.06" AMOLED, 410×502, SH8601 over QSPI, RGB565 |
| **Touch** | FT5x06 capacitive (I²C) |

## 🏗️ Built on

- **[ESP-IDF](https://github.com/espressif/esp-idf)** 5.5.x
- **[ESP-Brookesia](https://github.com/espressif/esp-brookesia)** — the phone UI system (apps,
  launcher, status bar, app lifecycle)
- **[LVGL](https://lvgl.io/)** v9 — the rendering/widget layer
- **Brookesia service framework** — the `ServiceManager` / helper pattern used for Wi-Fi and NVS

## 📁 Project layout

```
vpx-os/
├── main/                          # Entry point + app wiring
│   ├── main.cpp                   # app_main: display, Wi-Fi service, boot-time settings load
│   └── dark/                      # Custom dark stylesheet + theme constants
├── components/
│   ├── settings_app/              # Settings app (Wi-Fi / BLE / Display) + brightness persistence
│   ├── clock_app/                 # Clock app
│   └── espressif__brookesia_service_wifi/   # Vendored Wi-Fi service
├── docs/                          # Engineering write-ups (see below)
├── sdkconfig.defaults             # Project configuration (target, PSRAM, LVGL, fonts…)
└── partitions.csv                 # Flash partition table
```

## 🚀 Build & flash

Requires an ESP-IDF 5.5.x environment.

```bash
# one-time: point your shell at ESP-IDF
. $IDF_PATH/export.sh

# from the vpx-os/ directory
idf.py set-target esp32s3     # first build only
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Replace `/dev/ttyACM0` with your board's serial port. Exit the monitor with `Ctrl-]`.

## 📱 Provisioning Wi-Fi

1. Open **Settings → Wi-Fi** on the watch.
2. On your phone, join the Wi-Fi network **`VPX_WATCH`** (password `12345678`).
3. A captive-portal page opens automatically — enter your home network's SSID and password.
4. The watch connects, and remembers the network for automatic reconnection on future boots.

## 🗄️ Persistent settings

User settings live in a dedicated **`settings`** NVS namespace, kept separate from Wi-Fi
credential storage. Today that's display brightness; the save/load path is written generically
so future preferences (theme, etc.) can reuse it. Values are committed only when the user
*finishes* adjusting a control (e.g. on slider release), to avoid unnecessary flash wear.

## 📝 Engineering notes

The [`docs/`](docs/) directory contains write-ups from notable debugging sessions, kept for
future reference:

- **[`settings-display-freeze.md`](docs/settings-display-freeze.md)** — why opening Settings
  could freeze the display (a DMA-capable-memory fragmentation issue), how it was diagnosed,
  and the `CONFIG_BSP_DISPLAY_LVGL_BUF_HEIGHT` fix.



## ❤️ Credits & license

Based on Espressif's [ESP-Brookesia](https://github.com/espressif/esp-brookesia) examples and
the Waveshare board support package. Original demo sources are licensed under **CC0-1.0**;
ESP-Brookesia and the service components under **Apache-2.0**. See individual source headers
for details.
