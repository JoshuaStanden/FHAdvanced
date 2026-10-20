# firmware-itsybitsy — FHAdvanced on-demand radio node (Adafruit ItsyBitsy M0)

This node is the **"on-demand / function-event radio" controller** in the
FHAdvanced topology. It is NOT an ESP32 and NOT built with ESP-IDF — it is an
Adafruit **ItsyBitsy M0 Express** (ATSAMD21G18, 48 MHz, 32 KB RAM, 256 KB flash)
built with the **Arduino IDE**. It hosts the radios the hub prompts on demand:

    CC1101 (sub-GHz) · nRF24L01+ (2.4 GHz) · PN532 (NFC) · SX1262 (LoRa)

and talks to the Waveshare hub over ONE UART link using the shared `fh_protocol`
frame format (see ../components/fh_protocol). It runs none of the CONTINUOUS
capture radios — those live on the ESP32 nodes (C5 = 5 GHz+GPS, S3 = 2.4 GHz,
hub = BLE).

Why a separate toolchain for this one board: the ItsyBitsy is SAMD21, so it uses
Arduino/CircuitPython, not ESP-IDF. It is kept as a subfolder of the FHAdvanced
repo (not a separate repo) because it is a node in THIS system and uses THIS
project's wire protocol.

---

## STEP 1 — the compile test (do this FIRST, before any wiring)

`fh_ondemand_compiletest/fh_ondemand_compiletest.ino` is a VERIFY-ONLY sketch.
Its only job is to prove all four radio drivers FIT in the M0's 32 KB RAM with
headroom. It needs NO hardware attached — it is a compile/link test.

### One-time Arduino IDE setup (on your build computer)
1. Install the **Arduino IDE** (2.x recommended).
2. Tools > Board > Boards Manager → install **"Arduino SAMD Boards"** and
   **"Adafruit SAMD Boards"**. (Adafruit's package adds the ItsyBitsy M0 target.)
   - Adafruit SAMD also needs the Adafruit board-index URL in
     File > Preferences > "Additional boards manager URLs":
     https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
3. Tools > Manage Libraries → install **"RadioLib"** and **"Adafruit PN532"**
   (let it also install the **Adafruit BusIO** dependency it asks for).
4. Tools > Board → **Adafruit SAMD > Adafruit ItsyBitsy M0**.

### Run it
1. Open `fh_ondemand_compiletest/fh_ondemand_compiletest.ino`.
2. Press **Verify** (the checkmark — do NOT need Upload, and no board/radios
   need to be plugged in).
3. At the end of the output, read these two lines and report them back:
   - `Sketch uses X bytes (Y%) of program storage space.` (flash, max 262144)
   - `Global variables use Z bytes (W%) of dynamic memory ...` (RAM, max 32768)

### How to read the result
- **Flash (256 KB):** almost certainly fine; not the limiting factor.
- **RAM (32 KB) — the number that decides it:**
  - `< 60%` used → comfortable, the 4-radio node is viable.
  - `60–80%` → tight but workable (watch runtime buffers + stack at build time).
  - `> 80%` → risky; may not hold all four plus working buffers. Would reconsider
    (e.g. PN532 on I2C to drop a driver's footprint, or LoRa back on the XIAO-S3).

If the compiler throws an ERROR instead (e.g. a RadioLib constructor signature
mismatch, or a missing library), copy the exact error back — those are expected
to be quick fixes, not dead ends.

---

## IMPORTANT — the pins in the sketch are PLACEHOLDERS
The `#define PIN_*` values exist only so the driver constructors are well-formed
for the compiler. They are NOT the real wiring. The real ItsyBitsy pin map is a
later design step, done after the compile test confirms the node is viable.

## Status
STEP 1 (compile test) written, awaiting a Verify run on the build computer.
Nothing here is wired, flashed, or finalized. This is the viability gate for the
whole "ItsyBitsy as on-demand radio node" decision.
