<!-- ⚠️ SUPERSEDED (2026-10-20): this is the ORIGINAL 3-node summary. The design
     is now FOUR nodes — GPS moved to the hub, LoRa + the 3 wired radios moved to
     an ItsyBitsy M0, XIAO-S3 is 2.4 GHz only, XIAO-C5 is 5 GHz only. See the
     current README.md and docs/FIRMWARE_BRIEF.md. Kept for history. -->

# FHAdvanced

Experimental three-ESP32 wireless-recon device (branch of FootHoled). A
Waveshare ESP32-S3 touch-LCD HUB aggregates two XIAO daughterboards — a C5
(5 GHz Wi-Fi + GPS) and an S3 (2.4 GHz Wi-Fi + LoRa) — into one geotagged CSV
on a single microSD. Built with environments like DEF CON in mind.

**Status (2026-10-19):** design phase COMPLETE and documented. Nothing built or
flashed. The remaining work is firmware + hardware bring-up.

## Documents
- **`docs/BUILD_HANDOFF.md`** — START HERE to build firmware. A self-contained
  brief so a fresh session can go from "here are my docs, build the firmwares"
  to a design-faithful draft, in the right order, with the parked TODOs flagged.
- **`docs/ARCHITECTURE.md`** — authoritative system design: node roles, verified
  pinouts, UART frame format, dedup model, SD/CSV schema, heartbeat/health,
  failure behavior, and the system-level status tracker.
- **`docs/CONFIG_MENU.md`** — authoritative config spec: the 6 exposed settings
  + 4 maintenance actions, and everything locked as a compiled-in constant.
- **`docs/RESOURCES.md`** — what to GATHER before building firmware (vendor demo
  code, libraries, pinouts, datasheets), with plain-English context on each so
  you know what you're looking for.
- **`firmware/src/fh_pins.h`** — the HUB (Waveshare) pin map, hardware-verified.
  (Daughterboard pin maps are not written yet — see BUILD_HANDOFF "Parked".)

## To build the firmwares later
Open this folder and read `docs/BUILD_HANDOFF.md` — it names the read order, the
dependency build order (`fh_protocol` component → hub → XIAO-C5 → XIAO-S3), the
invariants to honor, and the deliberately-deferred items to leave as stubs.
