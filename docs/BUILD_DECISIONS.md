# FHAdvanced — Build Decisions (locked 2026-10-19)

Decisions made while reviewing the design package + the external resource
research, BEFORE firmware was written. This file + `RESOURCES_FOUND.md` are what
a build session reads AFTER `BUILD_HANDOFF.md` to know the framework path, the
GPS plan, and where the project lives. Where a decision DEVIATES from the design
docs, that is called out explicitly.

---

## 1. Framework — Arduino-as-component (hybrid), NOT pure-Arduino, NOT strictly pure-IDF

**DECISION:** Build with ESP-IDF's "arduino-esp32 as a component" pattern:
- **Hub (Waveshare)** and **XIAO-S3**: Arduino-as-component. Still built with
  `idf.py`, one toolchain, FreeRTOS underneath — but the Arduino core runs
  inside the IDF project so the Arduino-oriented libraries (RadioLib,
  Adafruit-PN532, TinyGPS++) work as-is.
- **XIAO-C5**: native ESP-IDF IF the arduino-esp32 core's C5 support lags our
  IDF 6.1 target (C5 is newer silicon). The C5's jobs — 5 GHz promiscuous
  (`esp_wifi`) + NEO-6M GPS over UART — are native-IDF-friendly either way, so a
  native C5 costs nothing extra.

**WHY (easiest + lowest flash-cycle risk):** it makes the PN532-on-SPI problem
disappear (Adafruit-PN532 already does SPI; no clean native-IDF SPI PN532 driver
exists), and it reuses tested libraries instead of hand-written drivers — fewer
hardware round-trips, which matters because this machine has NO ESP-IDF
toolchain and cannot compile locally.

**DEVIATION FROM THE DESIGN DOCS — acknowledged:** `ARCHITECTURE.md` #8 locks
"all ESP-IDF (NOT Arduino)". Arduino-as-component is a hybrid, not the purist
"no Arduino anywhere" the doc describes. It is an officially-supported Espressif
pattern, not a hack, but it IS a deliberate deviation chosen for build ease and
to dodge the PN532 SPI driver gap. If future-Joshua wants strict spec purity,
the alternative is pure-native-IDF + hand-porting the PN532 SPI driver (~1.5–2h
more work + more flash cycles on the nRF/PN532 path).

## 2. GPS — GY-NEO6MV2 (u-blox NEO-6M): attempt 5 Hz, fall back to 1 Hz

**MODULE IN HAND:** GY-NEO6MV2 = u-blox NEO-6M (1 Hz / 9600 baud by default).

**DECISION — "attempt 5 Hz with clean 1 Hz fallback":**
- On boot, send `UBX-CFG-RATE` (5 Hz) + `UBX-CFG-PRT` (raise baud — 5 Hz of NMEA
  will not fit in 9600). RE-SEND on EVERY boot, because the design's cold
  battery swaps fully power-cycle the module and the GY-NEO6MV2 clone's config
  persistence (backup battery / EEPROM) is unreliable.
- If the module does not accept/sustain 5 Hz, FALL BACK cleanly to 1 Hz / 9600.
  NEVER gate a capture on getting a fresh fix or on achieving 5 Hz.
- Clamp the CONFIG_MENU setting #5 "10 Hz" option OUT on this hardware — the
  NEO-6M maxes at 5 Hz (10 Hz is NEO-M8/M10 territory).

**WHY IT'S FINE:** `ARCHITECTURE.md` already designs the last-known-fix cache to
geotag EVERY capture regardless of GPS rate, and states 5 Hz is "really about
the driving case" (walking/running are covered at 1 Hz, below the GPS error
floor). So the device is fully functional at 1 Hz; 5 Hz only tightens vehicle-
speed precision. The firmware tries for 5 Hz, logs what it actually got, and
degrades honestly.

**MENU IMPACT:** setting #5 default stays 5 Hz (attempted); 10 Hz unsupported on
this module. Record the ACHIEVED rate so the log is honest about what ran.

## 3. Libraries to use (under Arduino-as-component)
- **RadioLib v7.5.0** — SX1262 (LoRa, XIAO-S3), CC1101 (sub-GHz, hub), nRF24L01+
  (2.4 GHz, hub). Pin to a verified tag.
- **Adafruit-PN532 v1.3.4** — PN532 NFC on SPI (hub).
- **TinyGPS++ (stable tag, not the 1.1-beta)** — NMEA parse for the NEO-6M (C5).

## 4. Build order (unchanged from BUILD_HANDOFF.md)
1. `components/fh_protocol/` — shared frame (SYNC/TYPE/LEN/CRC16, packed structs,
   encode/decode/resync). Built first so the wire format cannot drift.
2. Hub firmware (`firmware/`) — most specified; `fh_pins.h` already exists.
3. XIAO-C5 firmware (`firmware-c5/`) — 5 GHz sniff + NEO-6M GPS. Pin map from the
   captured C5 numbers (verify vs board).
4. XIAO-S3 firmware (`firmware-xs3/`) — 2.4 GHz sniff (priority) + SX1262 LoRa
   (best-effort, dual-core split). Pin map from the captured S3 numbers.

## 5. Parked items (leave as explicit TODO stubs — per BUILD_HANDOFF.md)
- Daughterboard pin maps: now get REAL GPIO numbers from the Seeed pinouts, but
  flagged "verify against physical board."
- Per-source `extra` payload contents in the CAPTURE record — TBD at bring-up.
- Function/event radio CSV row shapes (CC1101 / nRF24 / PN532) — TBD at bring-up.
- UI visual layout (#6) — deferred; implement behavior/handlers, minimal layout.

## 6. Where the project will be written (PENDING — not yet decided)
Candidate: a fresh self-contained folder (clean, zippable, push anywhere later).
NOT yet committed to a repo. NOTE the FootHoled repo situation is messy — the
FootHoledJoshin local clone is an orphan pointing at a deleted remote. If this
goes into a repo, the target repo/branch must be named explicitly.

## 7. Honest expectation (unchanged, from BUILD_HANDOFF.md)
Firmware from these docs is DESIGN-FAITHFUL but UNVERIFIED — no ESP-IDF toolchain
on this machine, so it cannot be compiled/flashed here. First real compile is on
hardware. Self-audit before handoff for this codebase's known compile traps:
includes traced to headers, `-Werror` / `-Wformat-truncation` (size snprintf
buffers for the real worst case; bound %f/%s), and the ESP-IDF rule that a new
`#include` of a component header needs that component added to the
CMakeLists.txt REQUIRES/PRIV_REQUIRES in the SAME edit.

## 8. Status
Design reviewed in full. Resources researched (two passes). Firmware NOT started.
Awaiting: (a) the FINAL framework choice re-opened by Round 2 below, (b) an
explicit "go", (c) the project-location choice (#6).
Outstanding gather item: Waveshare demo source (repo waveshareteam/
ESP32-S3-Touch-LCD-3.5) + schematic PDF (see RESOURCES_FOUND.md).

---

# ROUND 2 FINDINGS (2026-10-19 later) — three decisions changed

A second research pass verified specifics and OVERTURNED parts of sections 1 and
the SX1262 wiring assumption. Read these as superseding the earlier text where
they conflict.

## R2-A. FRAMEWORK RE-OPENED — Arduino-as-component does NOT reach IDF 6.1 on the C5
VERIFIED: arduino-esp32's stable 3.3.x line (the line that first added ESP32-C5
support) is built on **ESP-IDF 5.5.x**. The IDF-6.1-based Arduino core (4.0) is
still in planning and UNRELEASED as of this date. The core's CMake accepts
IDF up to 6.1.99, but that check does not prove a working 6.1 build.

Consequence: "Arduino-as-component on the C5 under IDF 6.1" is NOT a real stable
combination today. Since IDF 6.1 is the standing target, the **XIAO-C5 goes
native ESP-IDF** (this is the authorized "native C5 if needed" fallback; the
C5's esp_wifi 5 GHz sniff + NEO-6M UART are native-friendly, zero loss).

This also makes "Arduino-as-component everywhere" inconsistent with "IDF 6.1
everywhere", because the hub+S3 Arduino path is also 3.3.x / IDF 5.5. The final
framework choice (A / B below) is therefore RE-OPENED and must be set before build:
- **Option A — IDF 6.1 everywhere, native where Arduino can't follow.** C5 native
  (forced); hub+S3 ALSO native-IDF (RadioLib IDF build, CH422G IDF component,
  PN532 SPI ported to IDF). One toolchain version, matches ARCHITECTURE #8's
  "all IDF" intent. COST: reintroduces the PN532 SPI port (~1.5–2h, the
  flash-cycle-risky bit).
- **Option B — split by board.** C5 native IDF 6.1; hub+S3 Arduino-as-component
  pinned to IDF 5.5 (what the Arduino 3.3.x core targets). Fastest for the
  radio/NFC libraries, but the project then spans TWO IDF versions.
- Option C (wait for Arduino 4.0 / IDF 6.1) is not viable now — unreleased.
RECOMMENDATION: Option A, now that the C5 is native anyway (keeps the whole
project on IDF 6.1, the standing target).

## R2-B. SX1262 NEEDS A HOST RF-SWITCH PIN (RF_SW1) — doc assumption was WRONG
VERIFIED from the Wio-SX1262-for-XIAO schematic + module datasheet: the board
exposes a host-facing RF-switch control signal (`RF_SW1` / `LORA_RF_SW1`). The
ARCHITECTURE.md claim that ANT_SW is fully on-module via DIO2 and needs NO host
pin is NOT confirmed for this board — the schematic shows an externally
controlled RF switch.

Consequence: the SX1262 needs **8 host pins, not 7**: SCK, MOSI, MISO, NSS,
RESET, BUSY, DIO1, **+ RF_SW1**. DIO3/TCXO IS module-managed (confirmed). This
tightens the XIAO-S3 GPIO budget by one vs the doc's "7 pins / ~2 spare" math.
Firmware must drive RF_SW1 (or configure the radio to) — do NOT assume the
antenna switches itself, or TX/RX will fail silently on hardware. VERIFY the
exact signal against YOUR module revision's schematic at bring-up.

## R2-C. NEO-6M UBX bytes obtained (candidate, verify at bring-up)
Exact candidate packets from the research (hardcode WITH a "verify" comment, and
check the module's ACK response at runtime — do not treat as hardware-validated):
- UBX-CFG-RATE @ 5 Hz (200 ms): `B5 62 06 08 06 00 C8 00 01 00 01 00 DE 6A`
- UBX-CFG-PRT UART1 @ 38400 8N1 (constructed, checksum unverified vs NEO-6 spec):
  `B5 62 06 00 14 00 01 00 00 00 D0 08 00 00 00 96 00 00 03 00 03 00 00 00 00 00 8F 70`
- Two-stage baud switch: send CFG-PRT at the CURRENT baud, THEN switch the host
  UART, else the host loses the receiver.
- Persistence: assume NONE on the GY-NEO6MV2 clone — re-send every boot
  (matches section 2). NEO-6M max nav rate = 5 Hz confirmed (10 Hz unsupported).

## R2-D. Other confirmations (no decision change)
- Waveshare demo is ESP-IDF (`01_factory`: i2c_bus_init / io_expander_init /
  esp_3inch5_display_port_init / esp_3inch5_touch_port_init). Repo:
  github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5. Pull source from there.
- CH422G: use Waveshare/Espressif `ESP32_IO_Expander` component (has CH422G),
  NOT a hand-rolled driver. I2C address unconfirmed — read from schematic.
- PN532 SPI with arbitrary CS: CONFIRMED supported — `Adafruit_PN532(ss, SPIClass*)`.
  DIP-switch SPI-mode positions: set physically from the board silkscreen (unconfirmed).
- These three argue for Option A being cheaper than first thought: CH422G and the
  display/touch bring-up have a native-IDF reference (the factory demo itself),
  so going native on the hub is less from-scratch than it looked.

---

# ROUND 3 FINDINGS (2026-10-19 later still) — native-IDF driver feasibility

Framework is now DECIDED = Option A (all native ESP-IDF 6.1). Round 3 checked
whether each hub peripheral has a usable NATIVE component on IDF 6.1. One finding
changes hub wiring/architecture; one REVERSES a Round-2 conclusion in our favor.

## R3-A. ★ EXPANDER-CS PROBLEM — affects CC1101 AND PN532 (architecture-level)
VERIFIED: native ESP-IDF `spi_master` AND RadioLib's native ESP-IDF HAL both
assume the chip-select is a real GPIO driven by the SPI peripheral. Neither
natively understands a CS toggled through an I2C I/O expander. But `fh_pins.h`
puts the **CC1101 CS and PN532 CS on the CH422G expander** (only the nRF24 CSN is
a native GPIO). So those two radios cannot use the stock auto-CS path.

Two ways forward — a genuine HARDWARE-DESIGN decision, defer to bring-up if unsure:
- KEEP wiring as-is: write a custom expander-CS transport that (1) asserts the
  CH422G output, (2) runs the WHOLE SPI transaction with the bus LOCKED so no
  other device interleaves, (3) de-asserts. Adds ms of I2C latency per
  transaction. Workable but it is bespoke driver code = the flash-cycle-risky
  path; PN532+expander-CS is ranked the HIGHEST-risk item in the whole project.
- CHANGE wiring: move CC1101 CS and PN532 CS to NATIVE GPIOs (like nRF24 CSN
  already is). RadioLib + the PN532 component then work as-designed. Costs native
  GPIO budget — which fh_pins.h deliberately spent on the expander to conserve.
RECOMMENDATION: if GPIO budget allows, move at least the PN532 CS (highest risk)
to a native GPIO. Decide with the schematic in hand at bring-up.

## R3-B. ✓ REVERSAL — native SPI PN532 components DO exist (Round 2 was wrong)
Round 2 said no clean native-IDF PN532 SPI driver existed (only UART-only). FALSE
as of now — three SPI-capable PN532 components are in the Espressif registry:
- **jef-sure/pn532 v0.7.4** (BEST): SPI + I2C + UART, documents shared-bus use,
  needs IDF >= 5.2. PN532 SPI facts it encodes: SPI Mode 0, LSB-first, 5 MHz max
  (1 MHz default), whole-frame transaction, NSS-low-before-wake timing.
- garag/esp-idf-pn532 v0.2.1 (SPI/I2C/HSU; less shared-bus docs).
- victorobaro1/pn532-spi-idf v1.0.0 (S3-targeted, custom license).
CONSEQUENCE: the PN532 DRIVER is NOT net-new work (use jef-sure/pn532). The only
PN532 risk left is the expander-CS issue (R3-A), independent of the driver.
PN532 SPI manual refs for any transport adaptation: NXP UM0701-02 §6.1.1.1,
§6.2, §6.3.4, §7.2.11; AN10609.

## R3-C. ✓ C5 5 GHz sniffing confirmed native on IDF 6.1
`esp_wifi` on the C5 supports promiscuous mode, 802.11a/ac/ax, and
`WIFI_BAND_MODE_5G_ONLY`. APIs: esp_wifi_set_band_mode(),
esp_wifi_set_channel(primary, second), esp_wifi_set_promiscuous(true),
esp_wifi_set_promiscuous_rx_cb(). C5 CANNOT do 2.4 + 5 GHz simultaneously (single
radio, time-division) — 5 GHz-only avoids the band-switch limit; still hop across
5 GHz channels and record the channel per capture.
OPEN: the IDF-6.0 HE/secondary-channel reception quirk — could NOT confirm it is
fixed in 6.1. BRING-UP PLAN: first test 5 GHz at 20 MHz, WIFI_SECOND_CHAN_NONE,
verify HE reception, THEN try any secondary-channel config. set_channel after
wifi start.

## R3-D. Native building blocks CONFIRMED (no custom driver needed)
- RadioLib native ESP-IDF HAL exists (src/hal/ESP-IDF/EspHal.h; selected via
  RADIOLIB_BUILD_GENERIC + ESP_PLATFORM). Clean IDF 6.1 build NOT proven —
  compile its HAL in a minimal 6.1 project before the full hub.
- ST7796 LCD: espressif/esp_lcd_st7796 v1.5.0 (native, S3).
- CH422G: espressif/ESP32_IO_Expander v1.1.1 (native, has CH422G, needs IDF>=5.1;
  C++ API init/begin/pinMode/digitalWrite/multiDigitalWrite). 6.1 compat + the
  exact CH422G constructor signature + the board I2C address NOT confirmed.
- microSD: native SDMMC (esp_driver_sdmmc) AND SDSPI (esp_driver_sdspi) + FAT.

## R3-E. Still UNCONFIRMED — all need the Waveshare schematic/source (your pull)
- Is the microSD on SDMMC (OFF the SPI bus = good) or SDSPI (contends with the
  3 radios)? Decides whether there is a 4-device SPI bus-arbitration problem.
- Which SPI host (SPI2/SPI3) drives the ST7796 LCD vs the radio bus? S3 has
  limited SPI hosts — do NOT freeze the bus layout until this is mapped.
- FT6336 touch: exact native IDF component + board integration not confirmed.
- CH422G 7-bit I2C address on this board.

## R3-F. Native-IDF risk ranking → build sequence
Lowest→highest risk: (1) C5 5 GHz sniffer → (2) microSD/CSV → (3) SX1262/RadioLib
→ (4) CC1101+nRF24 shared bus → (5) ST7796+FT6336+CH422G bring-up →
(6) PN532 + expander CS (HIGHEST). Build/bring-up in that order so the riskiest
hardware round-trips happen only after the rest is proven. Per-target clean
`idf.py set-target` project first; validate each subsystem in isolation before
combining on the shared bus.

## R3-G. Net effect on the plan
- PN532 driver is NO LONGER a from-scratch port (R3-B) — good, lowers Option A cost.
- BUT the expander-CS issue (R3-A) is the real remaining hub risk and may warrant
  a small wiring change (PN532 CS → native GPIO). This is the one thing worth
  deciding (or at least flagging) before the hub firmware is written.
- C5 firmware is the safest place to start building when "go" is given.
