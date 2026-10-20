# FHAdvanced

Experimental **four-node** wireless-recon device (branch of FootHoled), built
with environments like DEF CON in mind. Continuously captures 2.4 GHz Wi-Fi,
5 GHz Wi-Fi, and BLE; performs on-demand sub-GHz / 2.4 GHz-proprietary / NFC
actions and a promptable LoRa link to a sensor stick; aggregates everything into
one geotagged CSV on a single microSD.

> **This is the `design-docs` branch — the complete "safe home" for the whole
> project** (all design docs, research, hardware pinouts, decisions, the shared
> wire protocol, the hub pin map, and the Pico on-demand node). It is the single
> source of truth; there are no separate per-node branches.

## 👉 Building firmware? START AT `docs/FIRMWARE_BRIEF.md`
It is the single current, authoritative starting point. `docs/ARCHITECTURE.md`
is the full system design (updated to the four-node layout); `docs/OPEN_DECISIONS.md`
has the decision trail.

## Node split (final, pin-verified 2026-10-20)
| Node | Chip | Role | Firmware |
|---|---|---|---|
| Hub | Waveshare ESP32-S3-Touch-LCD-3.5 **V2** | BLE + GPS + microSD + UI + aggregate 4 UART feeds | ESP-IDF 6.1, `firmware/` |
| XIAO-C5 | ESP32-C5 | 5 GHz Wi-Fi (continuous) | ESP-IDF 6.1, `firmware-c5/` |
| XIAO-S3 | ESP32-S3 | 2.4 GHz Wi-Fi (continuous) | ESP-IDF 6.1, `firmware-xs3/` |
| Pico | Raspberry Pi Pico (RP2040) | CC1101 + nRF24 + PN532 + SX1262 LoRa (on-demand) | arduino-pico, `firmware-pico-rp2040/` |

No camera (its J8 header pins are the hub's 4 UART links). No I/O expander needed.

## Documents
- **`docs/FIRMWARE_BRIEF.md`** — START HERE: consolidated build reference.
- **`docs/OPEN_DECISIONS.md`** — full decision trail; read the RESOLVED banner at top.
- **`docs/BUILD_DECISIONS.md`** — framework/GPS/library decisions + research rounds.
- **`docs/RESOURCES_FOUND.md`** — research findings (links, exact values, pin facts).
- **`docs/research-raw/`** — raw ChatGPT research (primary source).
- **`docs/hardware/`** — pinout images (Waveshare matrix, J8 header, and an
  ItsyBitsy pinout kept only as a historical reference — the on-demand node is
  now the Pico).
- **`docs/ARCHITECTURE.md`** — authoritative system design (four-node): node
  roles, UART frame format, dedup model, SD/CSV schema, failure taxonomy,
  DEF CON data-path, config + UI requirements. **`docs/CONFIG_MENU.md`** —
  authoritative config spec (6 settings + 4 maintenance actions). Both current.

## Code already written (scaffolding; UNVERIFIED until built on hardware)
- `components/fh_protocol/` — shared UART wire format (`fh_frame.h/.c` + host test).
- `components/fh_protocol/include/fh_ondemand.h` — hub↔Pico command set.
- `firmware/src/fh_pins.h` — hub pin map (rewritten for the real S3 pins).
- `firmware-pico-rp2040/` — Pico pin map + 4-radio compile test (PASSED: 4% flash, 4% RAM).

Per-node firmware LOGIC (hub aggregator/SD/GPS, the two sniffers, the Pico
radio handlers) is NOT yet written — that is the next step at "go time".
