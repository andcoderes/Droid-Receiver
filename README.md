# Chopper Receiver

> This firmware was strongly influenced by the [Amidala](https://github.com/thePunderWoman/Amidala) firmware by [thePunderWoman](https://github.com/thePunderWoman).

Firmware for the Chopper receiver, built on a **Seeed XIAO ESP32-S3**. Bridges
the DroidController phone app (BLE) to the chopper body and head boards
(encrypted ESP-NOW), translating JSON commands from the app into the
`BodyCommand` / `HeadCommand` structs the other boards expect, and forwarding
telemetry back the other way.

## Features

- **BLE peripheral** — advertises as "Chopper Droid", receives JSON commands from the DroidController app
- **ESP-NOW mesh** — encrypted peer-to-peer links to the body and head boards
- **Command translation** — parses app JSON into `BodyCommand`/`HeadCommand` structs (movement, buttons/macros, settings, audio, connection state)
- **Telemetry forwarding** — relays body/head connection and status back to the app over BLE
- **Unit tests** — native-platform tests with mock hardware abstractions

## Project Structure

```
src/
  main.cpp                        Entry point, BLE<->ESP-NOW message routing
  config.h                        BLE device name, timing constants (UUIDs/MACs/keys come from secrets.h)
  communication/
    BleController.h/.cpp          BLE peripheral, JSON message buffering
    EspNowController.h/.cpp       Encrypted ESP-NOW links to body/head
    CommandParser.h/.cpp          App JSON -> BodyCommand/HeadCommand translation
    MessageTypes.h                Shared struct definitions
test/                             Unit tests and hardware mocks
scripts/
  load_secrets.py                 Pre-build script: .env -> include/secrets.h
.env.example                      Template for ESP-NOW keys — copy to .env
platformio.ini
```

## Prerequisites

- [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) extension
- Python 3 (used by the `load_secrets.py` build script)

## Setup

1. Clone the repository and open it in VS Code.

2. Set up `.env` — see [Generating ESP-NOW keys](#generating-esp-now-keys) below. This covers the ESP-NOW keys, `CONTROLLER_MAC`, `BODY_MAC`/`HEAD_MAC`, and the BLE UUIDs. The build fails with a clear error if `.env` is missing or incomplete.

3. Build the firmware:
   ```bash
   pio run -e seeed_xiao_esp32s3
   ```

4. Upload to the board:
   ```bash
   pio run -e seeed_xiao_esp32s3 -t upload
   ```

5. Open the serial monitor:
   ```bash
   pio device monitor -b 115200
   ```

## Running Tests

Unit tests run on the native platform (no hardware required):

```bash
pio test -e native
```

Tests cover: BleController, EspNowController, CommandParser, and MessageTypes.

## Generating ESP-NOW keys

This board talks to the chopper body and head boards over encrypted
ESP-NOW, and to the DroidController app over BLE. Before your first build,
copy the template and fill in real values:

```bash
cp .env.example .env
```

`.env` needs six values:

**`PMK_KEY` / `LMK_KEY`** — two 16-byte (128-bit) ESP-NOW keys, each as 32
hex characters:

```bash
# Option 1: openssl
openssl rand -hex 16   # run twice — once for PMK_KEY, once for LMK_KEY

# Option 2: python
python3 -c "import secrets; print(secrets.token_hex(16))"
```

**These two keys must be byte-for-byte identical across chopper body,
chopper head, and chopper receiver** — generate them once and copy the same
`.env` values into all three projects. Mismatched keys mean the boards can't
decrypt each other's ESP-NOW packets and the mesh will never connect.

**`CONTROLLER_MAC`** — the receiver's own WiFi MAC address for the ESP-NOW
mesh (as hex, e.g. `C0CDD6CA29E0`). Must also be identical across all three
firmware projects, since body/head are hardcoded to expect ESP-NOW packets
from this address.

**`BODY_MAC` / `HEAD_MAC`** — the real MAC addresses of your body and head
boards, printed on their serial console at boot:
```
MAC Address: XX:XX:XX:XX:XX:XX
```

**`SERVICE_UUID` / `CHARACTERISTIC_UUID`** — the BLE service and
characteristic UUIDs the app connects to. Generate fresh ones with:

```bash
python3 -c "import uuid; print(uuid.uuid4())"
```

**These two UUIDs must match the DroidController app's copies exactly** —
see that project's README for where they live on the app side (its own
`.env`-equivalent, `local.properties`).

`scripts/load_secrets.py` runs automatically before every build (see
`platformio.ini`) and turns `.env` into `include/secrets.h`, which
`config.h` includes. Neither `.env` nor the generated `secrets.h` are
committed to git.

## License

All rights reserved.
