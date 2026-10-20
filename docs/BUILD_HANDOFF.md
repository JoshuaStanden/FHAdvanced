# FHAdvanced — Firmware Build Handoff

**Purpose of this file:** a self-contained brief so a FUTURE assistant session
(with NO memory of the design conversation) can build the firmwares from the
design docs alone. If you are that session: read this file, then the two design
docs it points to, then proceed in the order below. Everything you need to start
is here; where something is deliberately unfinished it is called out as such.

Author's intent: "Here are the documents I built — now build me the firmwares,"
with no other prompting. This file is what makes that possible.

---

## What this project is (one paragraph)
FHAdvanced is a three-ESP32 wireless-recon device for environments like DEF CON.
A **Waveshare ESP32-S3-Touch-LCD-3.5** is the HUB (UI, microSD, wired radios
CC1101 / nRF24 / PN532, BLE sniffing, sensor-stick orchestration, aggregates
everything to one geotagged CSV). A **Seeed XIAO ESP32-C5** daughterboard does
dedicated 5 GHz Wi-Fi sniffing + GPS. A **Seeed XIAO ESP32-S3** daughterboard
does 2.4 GHz Wi-Fi sniffing + SX1262 LoRa (acts as the system LoRa gateway).
Two UART links carry dedup'd capture records from the daughterboards to the hub.

## The authoritative design docs (READ THESE FIRST, in this order)
1. `docs/ARCHITECTURE.md` — system design, node roles, verified pinouts, the
   UART frame format (byte-level), dedup model, SD/CSV schema, heartbeat/health
   protocol, failure-mode behavior, and the status tracker. AUTHORITATIVE for
   system behavior and the wire protocol.
2. `docs/CONFIG_MENU.md` — the decided config: 6 exposed settings + 4 maintenance
   actions, and everything locked as a compiled-in constant. AUTHORITATIVE for
   what is user-configurable and the defaults.
3. `firmware/src/fh_pins.h` — the HUB (Waveshare) pin map, already written and
   hardware-verified. The two daughterboard pin maps do NOT exist yet (see
   "Parked" below).

Where the two docs ever disagree, the specific "authoritative" note in the text
wins; both were reconciled on 2026-10-19 and should agree.

## Before you start: gather the reference material
See `docs/RESOURCES.md` — it lists the vendor demo code, libraries, pinouts, and
datasheets to collect first (with context on each). Most important: the Waveshare
3.5 demo code + schematic, RadioLib (covers SX1262/CC1101/nRF24), and the two
XIAO pinout diagrams (needed to write the daughterboard pin maps that are
currently stubbed).

## Toolchain / framework (DECIDED)
- **All three firmwares are ESP-IDF** (NOT Arduino). The C5 needs a recent IDF
  (v6.x) for 5 GHz promiscuous. One `idf.py` workflow; per project
  `idf.py set-target {esp32s3|esp32c5}`.
- The shared wire protocol lives in ONE IDF component, `components/fh_protocol/`
  (`fh_frame.h` — packed structs + CRC16), included by all three firmwares so
  the frame definitions cannot drift. Build this FIRST.

## Build order (dependency order — follow it)
1. **`components/fh_protocol/`** — the shared frame component. Transcribe the
   UART frame format section of `ARCHITECTURE.md` verbatim into `fh_frame.h`:
   SYNC/TYPE/LEN/CRC framing, all TYPE codes (CAPTURE/GPS_FIX/SUMMARY/
   LORA_RELAY_TX/RX/HEARTBEAT/PING/PONG/ACK), the packed payload structs, the
   CRC16/CCITT helper, and the frame encode/decode + resync functions. Little-
   endian, `__attribute__((packed))`.
2. **Hub firmware (`firmware/`)** — most specified; `fh_pins.h` already exists.
   Build: two UART receivers (link A = XIAO-S3 on hdr TXD/RXD GPIO43/44, link B
   = XIAO-C5 on GPIO33/32) → parse frames → unified CSV `recon_log` on microSD
   (reuse the existing reboot-logging RTC-staging + batched-flush path) → the
   SPI radios (CC1101/nRF24/PN532) one-at-a-time → BLE sniffing → heartbeat/PING
   health tracking of both daughterboards → the failure-mode announce behavior →
   NVS config (6 settings + constants) → battery-backed RTC set from GPS fix
   relayed over link B. CSV on the SD (not binary); binary is wire-only.
3. **XIAO-C5 firmware (`firmware-c5/`)** — 5 GHz promiscuous sniff (continuous),
   GPS over its own UART, last-known-fix caching, dedup seen-table, emit CAPTURE
   + GPS_FIX + HEARTBEAT frames over the hub link. NEEDS a C5 pin map (parked).
4. **XIAO-S3 firmware (`firmware-xs3/`)** — 2.4 GHz promiscuous sniff (PRIORITY),
   SX1262 LoRa gateway (best-effort, must yield to the sniffer; dual-core split),
   dedup seen-table, LoRa-relay both directions for the sensor stick. NEEDS an
   XS3 pin map (parked). SX1262 host pins: SPI + NSS + RESET + BUSY + DIO1 (7);
   ANT_SW is on-module via DIO2 — do NOT allocate a host pin for it.

## Key invariants the firmware MUST honor (from the docs)
- **Dedup is IDENTITY-BASED ("seen is seen").** Emit a record only when a device
  key (MAC, or MAC+SSID for probe reqs) is NEW in the seen-table. RSSI is
  recorded once, NEVER a re-log trigger. Channel/subtype are NOT triggers. One
  row per re-log window. `count` accumulates; periodic SUMMARY carries it.
- **Re-log window** = CONFIG_MENU setting #4: timed (default 60 s) OR per-session
  (log once). This is the ONLY dedup knob.
- **One SD writer** = the hub. Daughterboards never touch SD; they stream frames.
- **CSV on the SD**, UTC timestamps only, one file rolled at a FIXED 128 MB.
  First line = header (format version, node eFuse MAC, session start UTC,
  config STOCK/MODIFIED).
- **eFuse MAC** (`esp_read_mac`) is the device identity in the log header — keep
  it out of capture rows so it is never confused with a sniffed device.
- **UART links run at 2 Mbaud**, binary framed; the GPS→C5 UART is separate at
  38400–115200.
- **Failure posture = "honest, not heroic":** detect + announce on screen, human
  reboots. No auto-reset of nodes. Drop-NEWEST + counter when a buffer fills.
- **Legal/TX posture:** capture is passive. BLE active-scan is OPT-IN only. The
  only intentional TX is the sensor-stick LoRa/BLE link. Do not add TX paths.

## PARKED — stubs/TODOs the firmware should leave explicit (not invent)
These were deliberately deferred to hardware bring-up. Do NOT guess values —
stub them with clear TODOs:
1. **Daughterboard pin maps** (`firmware-c5/`, `firmware-xs3/`) — only the hub's
   `fh_pins.h` exists. Create pin-map headers with TODO placeholders flagged for
   confirmation against the physical boards.
2. **Per-source `extra` payload contents** in the CAPTURE record — the exact
   fields packed into `extra` per source (SSID hash, OUI flags, etc.) are TBD at
   bring-up.
3. **Function/event radio record formats** (CC1101 / nRF24 / PN532) — these are
   EVENT-based (a deliberate burst/probe/NFC read), not the continuous recon
   stream; their per-function CSV row shapes are TBD. Stub the hooks.
4. **UI layout** (#6) — the screen layout / interaction model is deferred
   ("future Joshua"). The UI REQUIREMENTS are gathered in ARCHITECTURE.md
   (status bar colors, alert strings, maintenance actions) — implement the
   behavior/handlers, but the visual layout can be minimal/placeholder.

## Honest expectation (so nobody is surprised)
Firmware written from these docs will be DESIGN-FAITHFUL but UNVERIFIED — it
cannot be compiled against a real ESP-IDF toolchain, flashed, or run without the
physical boards, and several items above are stubs pending bring-up. The output
is a well-structured, substantial draft to flash-and-debug WITH hardware, not a
finished tested product. The docs were built to make that draft as complete and
correct-by-construction as possible.

## The instruction this file answers
> "Here are the documents I built — now build me the firmwares."
Start at Build order step 1 (the `fh_protocol` component), proceed in order,
honor the invariants, and leave the PARKED items as explicit TODO stubs.
