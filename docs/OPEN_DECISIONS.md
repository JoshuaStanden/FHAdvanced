# FHAdvanced — OPEN DECISIONS / where we stopped (2026-10-19 night)

Picking this up tomorrow. Everything below is research/planning — NO firmware has
been written beyond the `components/fh_protocol/` component. The hub pin map
(`firmware/src/fh_pins.h`) still needs a full rewrite and is NOT yet done.

## The situation in one paragraph
The FHAdvanced HUB is the Waveshare ESP32-S3-Touch-LCD-3.5 **V2** (confirmed).
Reading the real J8 2x16 header (see docs/RESOURCES_FOUND.md Round 8) showed the
hub is severely pin-constrained: almost every GPIO is taken by LCD / I2C / SD /
I2S-audio / USB / console, and the old `fh_pins.h` is invalid (it used classic-
ESP32 pins GPIO32/33/23 that don't exist on the S3, and collided with LCD/USB/
I2S). Dropping the camera (user decided: camera NOT needed) frees a pool of GPIO,
but on the actual J8 header the real usable pool is **14 native pins**, and the
three wired SPI radios + two UART links need ~14 — a **zero-margin** fit.

## The dilemma we're parked on: how to buy pin margin on the hub
The hub must drive: SPI bus (SCLK/MOSI/MISO), CC1101 (CS+IRQ), nRF24 (CSN+CE+IRQ),
PN532 (CS+IRQ), and two UART links to the XIAO daughterboards. That is right at
the 14-pin limit of the J8 header. Options discussed (pick a combo tomorrow):

RECOMMENDED COMBO (cleanest, no extra parts strictly required):
- **A1 — Move the console to USB-CDC** (S3 has USB-Serial-JTAG). Frees IO43/IO44
  for a UART link. Free, no downside. Do this regardless.
- **C1 — Run the PN532 over I2C instead of SPI** (PN532 supports I2C via a jumper;
  NFC is slow so I2C is fine). Removes the PN532 from the SPI/CS budget entirely
  AND sidesteps the whole PN532-SPI-driver + expander-CS problem. Net: SPI demand
  drops to just CC1101 + nRF24 -> ~12 native pins vs 14 usable -> ~2 spare, with
  NO add-on expander, NO strapping pin, NO SD-bus sharing.

IF STILL TIGHT / want comfortable margin — add an I/O EXPANDER on the header I2C
bus (IO7=SCL / IO8=SDA) for the SLOW lines only (chip-selects, resets, mode pins):
- Chosen chip: a **second TCA9554** at a DIFFERENT I2C address from the onboard
  one (board already uses TCA9554 -> the ESP-IDF `esp_io_expander` component
  already drives it -> zero new driver work; 8 outputs is plenty). Alternatives:
  TCA9555/PCA9555 (16-bit), PCF8574 (cheap, quasi-bidirectional), MCP23017.
- Keep FAST lines on native GPIO: SPI clk/data, all IRQ lines, and the nRF24 CSN
  (toggles every transaction — I2C latency would throttle it). Expander-CS is fine
  only for CC1101 CS and slow resets.

NOT an expander — do not confuse: **TCA9548A is an I2C MUX**, it adds I2C channels,
NOT GPIO. It does nothing for the pin crunch. Keep it in reserve ONLY if the I2C
device list turns up an unavoidable address collision.

AVOID: sharing the radio SPI bus onto the SD pins IO9/10/11 (SD is SDMMC on V2;
would hurt the logging path). Avoid 74HC595 shift register (worse than an expander).
Architecture alternatives if you'd rather rebalance than add parts: move the
CC1101 or nRF24 onto a daughterboard, or drop one hub radio (scope decision).

## The ONE check to do first tomorrow (gates the expander/mux decision)
**Map the hub I2C address list** on IO7/8: FT6336 touch, PCF85063 RTC, QMI8658
IMU, AXP2101, ES8311 codec, onboard TCA9554 — then confirm a 2nd TCA9554 (strapped
to a free address) and, if going C1, the PN532-over-I2C address all coexist with
NO collision. If a collision exists and can't be re-addressed -> that's the only
case where the TCA9548A mux earns a place.

## Then: the fh_pins.h REWRITE (first real build step of the hub)
Once the margin approach is chosen + I2C addresses are clean, rewrite
`firmware/src/fh_pins.h` for the real S3 pins within the J8 14-pin pool:
- LCD stays SPI2_HOST (IO1/3/5 + BL IO6); SD stays SDMMC (IO9/10/11); I2C IO7/8.
- Radio SPI bus on SPI3_HOST using header GPIO; IRQs native; nRF24 CSN native;
  CC1101 CS (and PN532 CS if NOT using C1) on the add-on expander; UART links on
  IO43/44 (console->USB-CDC) + two more header GPIO.
- IO0 is a STRAPPING pin — boot-safe output only, never a boot-blocking input.

## State of the rest of the build (for context)
- Framework DECIDED: all native ESP-IDF 6.1 (Option A).
- fh_protocol component: DONE (frame format + CRC16 + decoder + host test).
- GPS: GY-NEO6MV2 NEO-6M, attempt 5 Hz / fall back 1 Hz, candidate UBX bytes saved.
- SX1262 needs a host RF_SW1 pin (8 host pins, not 7).
- C5 5 GHz promiscuous confirmed native on IDF 6.1.
- Build order remaining: hub -> XIAO-C5 -> XIAO-S3 (all unwritten).
See docs/RESOURCES_FOUND.md (Rounds 1-8) and docs/BUILD_DECISIONS.md for detail.
