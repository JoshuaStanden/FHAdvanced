# FHAdvanced — FIRMWARE BRIEF (read this FIRST when building begins)

Single consolidated starting point for writing the firmware. When the user says
"build the firmware," start here — it points at every durable source you need so
you never have to reassemble state from chat. Status as of 2026-10-20.

> NOTHING is compiled/flashed on the authoring machine (no ESP-IDF toolchain, no
> C compiler). All firmware is DESIGN-FAITHFUL but UNVERIFIED until built on the
> user's hardware. The ONE compile proven so far: the ItsyBitsy 4-radio driver
> fit (26% flash, RAM within 32 KB) — see firmware-itsybitsy/.

## The device (one paragraph)
Four-node wireless-recon device. Continuous capture of 2.4 GHz Wi-Fi, 5 GHz
Wi-Fi, and BLE, split across chips so no band contends. On-demand "function/event"
radios (CC1101 sub-GHz, nRF24, PN532 NFC) and a promptable SX1262 LoRa link to a
sensor stick. One geotagged CSV on one microSD, written by the hub.

## FINAL hardware split (locked + pin-verified)
| Node | Chip | Continuous | On-demand | Firmware |
|---|---|---|---|---|
| Hub | Waveshare ESP32-S3-Touch-LCD-3.5 **V2** | BLE + GPS + SD + UI + aggregate | — | ESP-IDF 6.1, firmware/ |
| XIAO-C5 | ESP32-C5 | 5 GHz Wi-Fi | — | ESP-IDF 6.1, firmware-c5/ |
| XIAO-S3 | ESP32-S3 | 2.4 GHz Wi-Fi | — | ESP-IDF 6.1, firmware-xs3/ |
| ItsyBitsy | ItsyBitsy M0 Express (SAMD21) | — | CC1101+nRF24+PN532+SX1262 LoRa | Arduino/SAMD, firmware-itsybitsy/ |

No camera (dropped — its pins are the hub's 4 UART links). No I/O expander needed.

## Where everything durable lives (repo: JoshuaStanden/FHAdvanced, branch itsybitsy-ondemand)
- **Design (authoritative):** docs/ARCHITECTURE.md, docs/CONFIG_MENU.md, docs/BUILD_HANDOFF.md
- **Decisions:** docs/BUILD_DECISIONS.md, docs/OPEN_DECISIONS.md (read the RESOLVED banner at its top)
- **Research distilled:** docs/RESOURCES_FOUND.md (Rounds 1–8)
- **Research raw (primary source):** docs/research-raw/round1..3*.md
- **Hardware images:** docs/hardware/*.png (Waveshare pin matrix, J8 header, ItsyBitsy pinout)
- **Shared wire protocol:** components/fh_protocol/ (fh_frame.h/.c + host test) and include/fh_ondemand.h (hub↔ItsyBitsy command set)
- **Pin maps:** firmware/src/fh_pins.h (hub), firmware-itsybitsy/fh_itsy_pins.h (ItsyBitsy)
- **Compile test:** firmware-itsybitsy/fh_ondemand_compiletest/ (PASSED)

## Build order (dependency order)
1. ✅ components/fh_protocol/ — shared frame (DONE). + include/fh_ondemand.h (DONE).
2. ⬜ Hub (firmware/) — 4 UART RX/aggregate → unified CSV on microSD (reuse reboot-logging RTC-staging + batched flush) → BLE sniff → GPS (NEO-6M, attempt 5 Hz/fallback 1 Hz, re-send every boot) → NVS config (6 settings + 4 actions) → health (heartbeat/PING) → UI behavior (layout minimal). Pins: firmware/src/fh_pins.h.
3. ⬜ XIAO-C5 (firmware-c5/) — 5 GHz promiscuous (WIFI_BAND_MODE_5G_ONLY, test 20 MHz first for the HE quirk), dedup seen-table, emit CAPTURE/SUMMARY/HEARTBEAT. (GPS moved to hub, so C5 no longer does GPS.) Pin map: parked stub, create from XIAO-C5 pinout.
4. ⬜ XIAO-S3 (firmware-xs3/) — 2.4 GHz promiscuous (priority), dedup, emit frames. (LoRa moved to ItsyBitsy, so S3 is pure sniffer.) Pin map: parked stub.
5. ⬜ ItsyBitsy (firmware-itsybitsy/) — radio-command handlers for fh_ondemand.h over UART to the hub; one-at-a-time (single-core BUSY contract). Pins: firmware-itsybitsy/fh_itsy_pins.h.

## Key invariants (from ARCHITECTURE.md — honor these)
- Dedup is IDENTITY-BASED ("seen is seen"); RSSI never a re-log trigger; one row per re-log window (default 60 s); periodic SUMMARY carries counts.
- One SD writer = the hub. CSV on SD (UTC only), header per file, roll at FIXED 128 MB.
- eFuse MAC = device identity in the log HEADER, never in capture rows.
- Capture links 2 Mbaud binary framed; GPS link 9600–38400. Failure posture "honest not heroic" (detect + announce, human recovers). Drop-newest + counter on full buffers.
- Legal/TX: capture passive; BLE active-scan opt-in; only intentional TX is the sensor-stick LoRa link.

## Parked TODO stubs (do NOT invent — leave flagged)
- Daughterboard pin maps (real GPIOs from the XIAO pinouts, flag "verify vs board").
- Per-source `extra` payload contents in CAPTURE.
- Function/event radio CSV row shapes (CC1101/nRF24/PN532).
- UI visual layout (behavior/handlers only; minimal placeholder).

## Known pre-flash hazards to self-audit (this codebase has bitten before)
- Every macro/symbol traced to its header include (e.g. pdMS_TO_TICKS → freertos/FreeRTOS.h).
- -Werror + -Wformat-truncation is fatal: size snprintf buffers for the real worst case; pre-render %.1f into a small fixed buffer and use %s.
- A new #include of an ESP-IDF component header REQUIRES adding that component to the CMakeLists idf_component_register REQUIRES/PRIV_REQUIRES in the SAME edit.
- State expected flash-cycle cost up front for new peripheral drivers (user flashes; each failed build costs credits).

## Open sub-decisions (small; decide at the relevant build step)
- ItsyBitsy PN532 on SPI (current fh_itsy_pins.h) vs I2C — either fits; SPI chosen to share the one bus.
- Hub UART controller→pin routing (which HW UART per link; GPS can use a slow/LP UART).
- Confirm each hub J8 pin + each ItsyBitsy Dx/Ax against the physical board at bring-up.
