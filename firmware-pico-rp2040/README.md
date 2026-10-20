# firmware-pico-rp2040 — FHAdvanced on-demand radio node (Raspberry Pi Pico, RP2040)

This node is the **"on-demand / function-event radio" controller** in the
FHAdvanced topology — a plain **Raspberry Pi Pico (RP2040, non-W)**. It is NOT
an ESP32 and NOT built with ESP-IDF; it builds with the **arduino-pico** core
(Earle Philhower). It hosts the radios the hub prompts on demand:

    CC1101 (sub-GHz) · nRF24L01+ (2.4 GHz) · PN532 (NFC) · SX1262 (LoRa)

and talks to the Waveshare hub over ONE UART link using the shared `fh_protocol`
frame format (see ../components/fh_protocol). It runs none of the CONTINUOUS
capture radios — those live on the ESP32 nodes (C5 = 5 GHz, S3 = 2.4 GHz,
hub = BLE + GPS).

## Why the plain Pico (and NOT the Pico W) for this role
The RP2040's on-board resources all help a four-radio SPI host: **dual core**
(133 MHz), **264 KB RAM**, **2 MB flash**, **two hardware SPI buses**, 26 usable
GPIO. The **Pico W's** CYW43439 Wi-Fi/BLE is **redundant here** (2.4 GHz Wi-Fi is
the XIAO-S3's job; BLE is the Waveshare's) and consumes GP23/24/25/29 to talk to
the wireless chip, plus it adds another 2.4 GHz antenna to an already-crowded
same-band layout. For a pure wired-radio host, the non-W Pico is the right board.

## Compile test — PASSED (verified on hardware toolchain)
The four-radio compile test below was built with the arduino-pico core
(RadioLib 7.8.1 + Adafruit PN532 1.3.4 + Adafruit BusIO 1.17.4):

    Flash:  94,564 bytes  (4% of 2,093,056)
    RAM:    11,372 bytes  (4% of 262,144) — 250,772 bytes free

All four radio drivers compiled AND linked (RadioLib has a native RP2040 HAL,
`hal/RPiPico/PicoHal.cpp`), and the RP2040 custom-SPI-pin pattern
(`SPI.setSCK/setTX/setRX` before `begin()`) is exercised by the sketch. The one
remaining unverified step is the four radios wired and talking on real hardware
(the same hardware-in-hand bring-up every node faces).

---

## STEP 1 — the compile test (do this FIRST, before any wiring)

`fh_pico_compiletest/fh_pico_compiletest.ino` is a VERIFY-ONLY sketch. Its job is
to prove all four radio drivers BUILD and LINK on the arduino-pico core (and that
the pin map header + the RP2040 SPI-pin pattern compile). It needs NO hardware
attached — it is a compile/link test.

### One-time Arduino IDE setup (on your build computer)
1. Install the **Arduino IDE** (2.x recommended).
2. File > Preferences > "Additional boards manager URLs" → add:
   `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
3. Tools > Board > Boards Manager → install
   **"Raspberry Pi Pico/RP2040" by Earle F. Philhower, III**.
4. Tools > Manage Libraries → install **"RadioLib"** and **"Adafruit PN532"**
   (let it also install the **Adafruit BusIO** dependency it asks for).
5. Tools > Board → **Raspberry Pi Pico/RP2040 > "Raspberry Pi Pico"**.
   ⚠️ Pick the plain **"Raspberry Pi Pico"**, NOT "Raspberry Pi Pico W".

### Run it
1. Open `fh_pico_compiletest/fh_pico_compiletest.ino`.
2. Press **Verify** (the checkmark — you do NOT need Upload, and no board or
   radios need to be plugged in to compile).
3. At the end of the output, read these two lines and report them back:
   - `Sketch uses X bytes (Y%) of program storage space.` (flash, max ~2093056)
   - `Global variables use Z bytes (W%) of dynamic memory ...` (RAM, max 262144)

### How to read the result
- **Flash (~2 MB):** will be fine; not the limiting factor.
- **RAM (256 KB):** four radio drivers are tens of KB at most, so expect a LOW
  percentage. `< 50%` is comfortable. Unlike the M0 (32 KB, where RAM was the
  deciding number), the RP2040 is not expected to be RAM-constrained for this job.
- If the compiler throws an **ERROR** instead (a RadioLib constructor-signature
  mismatch, a `SPI.setTX` not-found on an older core, or a missing library), copy
  the exact error back — those are expected to be quick fixes, not dead ends.

### OPTIONAL STEP 2 — flash it (only if you want to go past compile)
Verify alone answers the viability question. If you also want to confirm it runs:
1. Hold **BOOTSEL** on the Pico while plugging it into USB (mounts as a drive).
2. Press **Upload** in the IDE.
3. Open Tools > Serial Monitor at **115200 baud**. You should see one line:
   `compile-test references: <number>`
   That only proves the firmware boots and the drivers initialize (returning
   errors with no radios attached is expected) — it is NOT a radio test.

### Real radio bring-up (later, hardware-in-hand)
Wiring even ONE module per `fh_pico_pins.h` and confirming a SPI chip-ID read is
the actual radio test — that is a separate, later step once this viability gate
passes.

---

## Files
- `fh_pico_compiletest/fh_pico_compiletest.ino` — the VERIFY-only four-radio test.
- `fh_pico_pins.h` — draft real pin map (shared SPI bus + per-radio CS/IRQ/etc.).

## Status
Compile test PASSED (see numbers above). This Pico is the committed on-demand
radio node for FHAdvanced. Remaining work is the hardware bring-up: wire the
radios per `fh_pico_pins.h` and confirm each over SPI, at go-time.
