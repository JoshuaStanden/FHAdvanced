# FHAdvanced

Experimental three-ESP32 wireless-recon device (a branch of the FootHoled line).
A Waveshare ESP32-S3 touch-LCD **HUB** aggregates two XIAO daughterboards — a
**C5** (5 GHz Wi-Fi + GPS) and an **S3** (2.4 GHz Wi-Fi + LoRa) — into one
geotagged CSV on a single microSD. Designed with environments like DEF CON in
mind.

> **Status:** design phase complete and documented. Firmware authoring has
> **started** with the shared protocol component only. NOTHING has been compiled
> or flashed — the authoring machine has no ESP-IDF toolchain, so all code here
> is **design-faithful but UNVERIFIED** until built and flashed on real hardware.

## Repository layout

| Path | What it is |
|---|---|
| `docs/ARCHITECTURE.md` | Authoritative system design: node roles, verified pinouts, byte-level UART frame format, dedup model, SD/CSV schema, health protocol, failure modes. |
| `docs/CONFIG_MENU.md` | Authoritative config spec: 6 exposed settings + 4 maintenance actions; everything else compiled-in. |
| `docs/BUILD_HANDOFF.md` | Start-here brief to build the firmwares from the docs alone. |
| `docs/BUILD_DECISIONS.md` | Decisions locked before/while building (framework, GPS, libraries) + two research rounds' findings. **Read after BUILD_HANDOFF.** |
| `docs/RESOURCES.md` | The reference-material gather list (vendor demos, libraries, datasheets). |
| `docs/RESOURCES_FOUND.md` | Research results: located links + exact values + honesty caveats. |
| `firmware/src/fh_pins.h` | HUB (Waveshare) pin map — hardware-verified. |
| `components/fh_protocol/` | **Shared UART wire-format component** (`fh_frame.h/.c`) + host unit test. Built FIRST so the frame layout cannot drift across the three firmwares. |

## Key build decisions (see `docs/BUILD_DECISIONS.md` for full rationale)

- **Framework:** all **native ESP-IDF 6.1** across all three nodes (Option A).
  The arduino-esp32 core that supports the C5 is pinned to IDF 5.5, so Arduino
  cannot reach the 6.1 target — hence native everywhere.
- **GPS:** GY-NEO6MV2 (u-blox NEO-6M). Attempt 5 Hz + raised baud, re-apply every
  boot (clone does not persist config), fall back cleanly to 1 Hz. 10 Hz clamped
  out (NEO-6M max is 5 Hz).
- **SX1262 (XIAO board):** needs **8** host pins — SPI(3)+NSS+RESET+BUSY+DIO1 **+
  RF_SW1**. The board exposes a host RF-switch signal; it is NOT fully on-module
  via DIO2 (an earlier assumption that was corrected during research).

## Build order (dependency order)

1. ✅ `components/fh_protocol/` — shared frame (done).
2. ⬜ `firmware/` — HUB (Waveshare): 2 UART RX, unified CSV on microSD, SPI radios
   (CC1101/nRF24/PN532) one-at-a-time, BLE sniff, heartbeat/PING health, NVS
   config, RTC-from-GPS.
3. ⬜ `firmware-c5/` — XIAO-C5: 5 GHz promiscuous sniff + NEO-6M GPS.
4. ⬜ `firmware-xs3/` — XIAO-S3: 2.4 GHz sniff (priority) + SX1262 LoRa gateway.

## Verifying the protocol on a PC (no hardware needed)

`components/fh_protocol/` is plain C with no framework/hardware dependency, so it
can be unit-tested on any machine with a C compiler:

```
gcc -std=c11 -Wall -Wextra -Werror \
    -I components/fh_protocol/include \
    components/fh_protocol/fh_frame.c \
    components/fh_protocol/test/test_fh_frame.c -o test_fh_frame
./test_fh_frame      # expect: ALL TESTS PASSED
```

## Parked (explicit TODOs, deferred to hardware bring-up)

- Daughterboard pin maps (`firmware-c5/`, `firmware-xs3/`) — real GPIOs captured
  from the Seeed pinouts, flag "verify against physical board."
- Per-source `extra` payload contents in the CAPTURE record.
- Function/event radio CSV row shapes (CC1101 / nRF24 / PN532).
- UI visual layout (deferred; behavior/handlers only).
