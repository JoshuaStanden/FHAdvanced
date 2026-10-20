# FHAdvanced — Resources FOUND (research results, 2026-10-19)

Companion to `RESOURCES.md` (the gather list). This file records the actual
links located in the external research pass, with the honesty caveats noted so
nothing here is mistaken for independently-verified fact. Where a direct
download URL could not be confirmed, that is said plainly — open the vendor wiki
page and grab the asset by hand.

> IMPORTANT: Several direct-asset URLs (ZIPs, schematic PDFs, pinout images)
> were NOT independently verified by the research pass. Treat links marked
> "(unverified direct URL)" as a starting point, not a guarantee. The vendor
> WIKI/PRODUCT pages are the reliable anchors.

---

## TIER 1 — critical

### 1. Waveshare ESP32-S3-Touch-LCD-3.5 — demo code + schematic
- Product wiki (reliable anchor): https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5
- Demo code: listed in the wiki's "Resources" section — the `01_factory` demo
  (initializes I2C bus, CH422G I/O expander, display, touch) plus LVGL examples.
  Direct ZIP URL NOT verified — download from the wiki Resources section by hand.
- Schematic PDF: in the same wiki Resources section. Direct PDF URL NOT verified.
- STILL TO DO BY HAND: download the demo ZIP + schematic from the wiki. These
  are what make the screen / touch / microSD / CH422G expander actually work and
  confirm the header wiring. This is the main outstanding gather item.
- Hardware note from the research: do NOT assume every peripheral is on the same
  SPI bus — verify display and storage wiring against the schematic.

### 2. RadioLib — shared radio driver (SX1262 + CC1101 + nRF24)
- Repo: https://github.com/jgromes/RadioLib
- API docs: https://jgromes.github.io/RadioLib/
- Releases: https://github.com/jgromes/RadioLib/releases
- Latest stable verified: v7.5.0
- Covers all three of our radios: SX1262 (SX126x driver family), CC1101
  (sub-GHz FSK), nRF24L01+ (nRF24 family).
- CAUTION: RadioLib is Arduino-first. Under our Arduino-as-component decision it
  works as-is. Pin to a verified tag; re-check the releases page before pinning.

### 3. XIAO pinouts (both daughterboards) — numbers captured
- XIAO ESP32-C5 getting-started: https://wiki.seeedstudio.com/xiao_esp32c5_getting_started/
- XIAO ESP32-C5 pin multiplexing: https://wiki.seeedstudio.com/xiao_esp32c5_pin_multiplexing/
- XIAO ESP32-C5 pinout image (unverified direct URL): https://files.seeedstudio.com/wiki/XIAO-ESP32C5/img/Pinout.png
  Documented C5 mapping: D6/TX=GPIO11, D7/RX=GPIO12, D8/SCK=GPIO8,
  D9/MISO=GPIO9, D10/MOSI=GPIO10.
- XIAO ESP32-S3 getting-started: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
- XIAO ESP32-S3 pin multiplexing: https://wiki.seeedstudio.com/xiao_esp32s3_pin_multiplexing/
- XIAO ESP32-S3 pinout image (unverified direct URL): https://files.seeedstudio.com/wiki/XIAO-ESP32S3/img/Pinout.png
  Documented S3 mapping: D6/TX=GPIO43, D7/RX=GPIO44, D8/SCK=GPIO7,
  D9/MISO=GPIO8, D10/MOSI=GPIO9.
- These numbers let the two parked daughterboard pin maps be written with REAL
  GPIOs (still flag "verify against physical board" — strapping/boot/debug pins
  and onboard peripherals reduce what is actually free).

---

## TIER 2 — additional

### 4. Wio-SX1262 LoRa module
- Seeed wiki: https://wiki.seeedstudio.com/Wio-SX1262/
- Semtech SX1262 product page + datasheet: https://www.semtech.com/products/wireless-rf/lora-connect/sx1262
  (SX1261/SX1262 datasheet dated 2025-04-07.)
- Seven host signals to wire: SCK, MOSI, MISO, NSS, RESET, BUSY, DIO1.
- DIO2 antenna switch: the SX1262 CHIP can drive an external RF switch via DIO2;
  whether THIS Wio-SX1262 board routes it on-module must be confirmed from the
  module schematic. Our ARCHITECTURE.md treats this as resolved (on-module via
  DIO2); the research flags it as "verify at bring-up." Low risk, confirm with
  hardware in hand.

### 5. ESP32-C5 dual-band Wi-Fi reference — esp32-mac-collector
- Repo: https://github.com/Grostl/esp32-mac-collector
- FRAMEWORK: Arduino (ESP32 Arduino core 3.x), NOT native ESP-IDF.
- Value: proof the C5 HARDWARE does 2.4 + 5 GHz promiscuous capture + a code
  reference. It is NOT a drop-in ESP-IDF foundation — our C5 firmware's 5 GHz
  sniff is esp_wifi (native-IDF-friendly) regardless.

### 6. GPS
- TinyGPS++ (NMEA parser): https://github.com/mikalhart/TinyGPSPlus
  Latest stable tag seen: v1.0.3a (a 1.1-beta line exists — use the stable tag).
  Arduino-oriented; fine under Arduino-as-component.
- MODULE IN HAND (DECIDED): GY-NEO6MV2 = u-blox NEO-6M. See DECISIONS doc.
  (The MAX-M10S / L76K candidates from research are NOT being used.)

### 7. PN532 NFC
- Adafruit-PN532 (Arduino, does SPI): https://github.com/adafruit/Adafruit-PN532
  Latest stable seen: v1.3.4. This is what we use on SPI under Arduino-as-component.
- felipegtralli/pn532 (native ESP-IDF, UART-ONLY): https://github.com/felipegtralli/pn532
  NOT usable for our SPI wiring — recorded only to explain why we did not go
  native for the PN532.

---

## TIER 3 — manufacturer datasheets (backup reference)
- CC1101 (TI, Rev I, 2013-11-05): https://www.ti.com/lit/ds/symlink/cc1101.pdf
- nRF24L01+ (Nordic) product family: https://www.nordicsemi.com/Products/nRF24-series
  PS PDF (unverified direct URL): https://infocenter.nordicsemi.com/pdf/nRF24L01P_PS_v1.0.pdf
- PN532 (NXP, Rev 3.6, 2017-11-28): https://www.nxp.com/docs/en/nxp/data-sheets/PN532_C1.pdf
  PN532 User Manual: https://www.nxp.com/docs/en/user-guide/141520.pdf
- SX1262 (Semtech, 2025-04-07): https://www.semtech.com/products/wireless-rf/lora-connect/sx1262

---

## Outstanding gather items (do these by hand when convenient)
1. Waveshare demo source + schematic. Confirmed repo + direct PDF (Round 2):
   - Repo: https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5
   - Schematic PDF (direct): https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5-Schematic.pdf
   - Demo is ESP-IDF; `01_factory` is the comprehensive one. Pull the source from the repo.
2. Confirm the two XIAO pinout image URLs resolve (GPIO numbers already captured).
3. At bring-up: confirm Wio-SX1262 RF_SW1 host pin on YOUR module revision (see below).

---

# ROUND 2 — verified links + exact values (2026-10-19 later)

## Waveshare hub
- Wiki: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5
- Official source repo: https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5
- Schematic PDF (exact board, NOT the 3.5B variant):
  https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5-Schematic.pdf
- Demo framework: ESP-IDF (C APIs i2c_bus_init / io_expander_init /
  esp_3inch5_display_port_init / esp_3inch5_touch_port_init). `01_factory` demo.
- Direct demo ZIP URL: NOT confirmed — get source from the repo above.

## NEO-6M GPS — official docs + candidate UBX packets
- NEO-6 datasheet (GPS.G6-HW-09005): https://content.u-blox.com/sites/default/files/products/documents/NEO-6_DataSheet_%28GPS.G6-HW-09005%29.pdf
- LEA-6/NEO-6/MAX-6 Hardware Integration Manual: https://content.u-blox.com/sites/default/files/products/documents/LEA-NEO-MAX-6_HIM_%28UBX-14054794%29_1.pdf
- CANDIDATE packets (verify vs NEO-6 protocol spec + check module ACK at bring-up):
  - CFG-RATE 5 Hz: B5 62 06 08 06 00 C8 00 01 00 01 00 DE 6A
  - CFG-PRT UART1 38400 8N1 (checksum unverified): B5 62 06 00 14 00 01 00 00 00 D0 08 00 00 00 96 00 00 03 00 03 00 00 00 00 00 8F 70
- Max nav rate = 5 Hz (10 Hz NOT supported). Assume NO persistence on the clone.

## arduino-esp32 / C5 / IDF 6.1
- Releases: https://github.com/espressif/arduino-esp32/releases
- Release roadmap: https://github.com/espressif/arduino-esp32/wiki/Release-Roadmap-and-Management
- Finding: C5 support is in the 3.3.x line (IDF 5.5.x). IDF-6.1-based Arduino 4.0
  is unreleased. => C5 must be native ESP-IDF for the 6.1 target.

## Wio-SX1262 (CORRECTION: host RF_SW1 pin exists)
- Wiki: https://wiki.seeedstudio.com/Wio-SX1262/
- XIAO board schematic: https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Schematic_Diagram_Wio-SX1262_for_XIAO.pdf
- Module datasheet: https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Wio-SX1262_Module_Datasheet.pdf
- Host pins = 8: SCK, MOSI, MISO, NSS, RST, BUSY, DIO1, RF_SW1. DIO3/TCXO on-module.

## CH422G I/O expander
- Espressif component registry: https://components.espressif.com/components/espressif/ESP32_IO_Expander
- Waveshare library source (has CH422G): https://github.com/waveshareteam/ESP32-S3-Touch-LCD-5/tree/master/examples/Arduino/libraries/ESP32_IO_Expander
- I2C address: NOT confirmed — read from the hub schematic.

## PN532 (SPI, confirmed)
- Adafruit-PN532: https://github.com/adafruit/Adafruit-PN532
- HW SPI constructor: Adafruit_PN532(uint8_t ss, SPIClass *theSPI) — arbitrary CS OK.
- NXP PN532 datasheet (Rev 3.6): https://www.nxp.com/docs/en/nxp/data-sheets/PN532_C1.pdf
- NXP User Manual: https://www.nxp.com/docs/en/user-guide/141520.pdf
- DIP-switch SPI-mode positions: set physically from board silkscreen (unconfirmed).

---

# ROUND 4 — Waveshare official docs read (2026-10-19 latest)

Source: https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5 and its Resources /
Working-with-ESP-IDF subpages. These RESOLVE several Round-3 unknowns.

## Confirmed direct resource links (all live)
- Demo source (ESP-IDF + Arduino): https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5
- V1 schematic: https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5-Schematic.pdf
- V2 schematic (Rev2.0): https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5_Rev2.0.pdf
- 2D/3D + project doc: https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5-2Dand3D.zip
- Chip datasheets (all on files.waveshare.com/wiki/common/): ST7796S, FT6336U,
  AXP2101, PCF85063A, QMI8658C, ES8311.

## Factory-demo (01_factory) native-ESP-IDF init order — MIRROR THIS on the hub
  i2c_bus_init();
  io_expander_init();                 // CH422G
  esp_3inch5_display_port_init(&io_handle, &panel_handle, LCD_BUFFER_SIZE);  // ST7796 SPI
  esp_3inch5_touch_port_init(&touch_handle, i2c_bus_handle, H_RES, V_RES, ROT); // FT6336 I2C
  esp_axp2101_port_init(i2c_bus_handle);   // power mgmt
  esp_es8311_port_init(i2c_bus_handle);    // audio codec
  esp_qmi8658_port_init(i2c_bus_handle);   // IMU
  esp_pcf85063_port_init(i2c_bus_handle);  // RTC (battery-backed — the time base)
  esp_sdcard_port_init();                  // microSD
  esp_camera_port_init(I2C_PORT_NUM);
  esp_wifi_port_init(...);
  esp_3inch5_brightness_port_init(); esp_3inch5_brightness_port_set(80);
  lv_port_init();

## Resolved unknowns
- ST7796 LCD = SPI; FT6336 touch = I2C (touch is OFF the SPI bus — good).
- RTC = PCF85063 (I2C) with backup-battery header → this IS the battery-backed
  RTC the ARCHITECTURE time-sync design assumes. Confirmed real.
- microSD has a dedicated esp_sdcard_port_init(); fh_pins.h notes SD on
  GPIO9/10/11 SDMMC 1-bit → consistent with SDMMC, OFF the SPI bus (good case
  for the radios). Confirm exact pins from the demo source/schematic.
- Power mgmt = AXP2101 (the chip the earlier FootHoled AXP2101 lesson referenced).
- Board brings out I2C/UART/USB + GPIOs on a 2.54mm header.

## ⚠️ Two design-relevant flags
1. ESP-IDF VERSION TENSION: Waveshare requires ESP-IDF **V5.5.0+** for this
   board's examples (screenshots use 5.5.2). The demo port layer is a 5.5-era
   codebase. Our target is 6.1 — it will LIKELY build but Waveshare does not
   promise 6.1, and 6.0→6.1 had breaking changes. Expect possible small fixups
   when building the Waveshare display/touch/SD/CH422G code on 6.1. (Does NOT
   change the Option A decision; just budget a little porting on the hub.)
2. BOARD REVISION: ★ CONFIRMED = **V2** (user checked the label, 2026-10-19).
   Pin all wiring verification against the V2 (Rev2.0) schematic ONLY:
   https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5_Rev2.0.pdf
   (ignore the V1 schematic). Firmware is identical V1/V2 — nothing in the build
   changes — but the schematic reference and any revision-specific GPIO do.
   V2 advantages FOR THIS PROJECT:
   - TF-card insert/removal DETECTION → a real hardware card-detect line to drive
     the ARCHITECTURE "SD card has an issue" alert + safe-swap flow, instead of
     only detecting at write-failure. If we want it, read the card-detect GPIO
     off the Rev2.0 schematic (V2-only).
   - Reworked AXP2101 power + battery reverse-protection + warning LED → better
     for the 18650 cold-swap field use.
   - "Adjusted RF circuit" + better EMC/ESD → cleaner baseline for a board we are
     adding CC1101/nRF24/PN532 + two XIAO radios onto.
- SKUs: 30733 (ESP32-S3-Touch-LCD-3.5), 30934 (…-C variant).

---

# ROUND 5 — VERIFIED hub pins from Waveshare's own ESP-IDF demo source
# (cloned waveshareteam/ESP32-S3-Touch-LCD-3.5, ESP-IDF/01_factory, 2026-10-19)

GROUND TRUTH from the demo source (not the datasheet, not a guess). These are
what the board actually uses.

## Verified pin assignments (01_factory)
| Function              | GPIO        | Bus / notes                                  |
|-----------------------|-------------|----------------------------------------------|
| I2C SDA               | GPIO8       | shared: touch(FT6336), RTC(PCF85063), AXP2101, QMI8658, expander |
| I2C SCL               | GPIO7       | shared I2C, 400 kHz                           |
| microSD CLK           | GPIO11      | SDMMC, 1-bit (slot width=1)                   |
| microSD CMD           | GPIO10      | SDMMC 1-bit                                   |
| microSD D0            | GPIO9       | SDMMC 1-bit — SD is OFF the SPI bus (good)    |
| LCD MOSI              | GPIO1       | SPI2_HOST, 80 MHz                             |
| LCD SCLK              | GPIO5       | SPI2_HOST                                     |
| LCD DC                | GPIO3       | SPI2_HOST                                     |
| LCD backlight (BL)    | GPIO6       | LEDC PWM (not SPI)                            |
| LCD MISO / CS / RST   | NC          | not connected (CS tied, no readback)          |
| BOOT button           | GPIO0       | strapping pin                                 |

## ★ TWO CORRECTIONS to earlier assumptions
1. I/O EXPANDER IS A **TCA9554**, NOT a CH422G. The demo calls
   `esp_io_expander_new_i2c_tca9554(..., ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, ...)`.
   ALL earlier notes (and fh_pins.h) assumed CH422G — WRONG for this board/demo.
   Use the TCA9554 path of espressif/esp_io_expander (idf-managed component used
   by the demo: `esp_io_expander_tca9554.h`). Address = base 000. The CH422G
   research (ESP32_IO_Expander, addresses) does NOT apply here. NOTE: the fh_pins.h
   "EXIO*" expander-CS plan was built on the CH422G assumption — re-derive it for
   the TCA9554 (8 IO, different API) OR move those CS lines to native GPIO.
2. LCD occupies GPIO1 (MOSI), GPIO3 (DC), GPIO5 (SCLK), GPIO6 (BL) on SPI2_HOST.
   These are NOT free. Any FHAdvanced radio mapped onto 1/3/5/6 is a HARD conflict.

## ★ CONFLICTS / CHECKS vs the current fh_pins.h (hub pin map)
MATCHES (good — fh_pins.h got these right):
- SD on GPIO9/10/11 ✅ (though order: real = D0=9, CMD=10, CLK=11).
- I2C on GPIO7/8 ✅.
MUST RE-CHECK (fh_pins.h assigns radio pins; verify none collide + that they are
actually broken out on the V2 header — the demo does not touch them, which does
NOT prove they are free; camera/audio/PSRAM may use them):
- fh_pins SPI radios SCLK=18 / MOSI=23 / MISO=19 — confirm free on V2 schematic.
- fh_pins CC1101 GDO0=2, nRF24 IRQ=4, PN532 IRQ=17, nRF24 CSN=15 — confirm free.
- fh_pins C5 UART = GPIO32/33, XS3 UART = GPIO43/44 — confirm free on V2 header.
- Expander-CS (EXIO3/5/6/7) must be redone for TCA9554, not CH422G.
- The LCD SPI bus is SPI2_HOST; put the radio SPI bus on SPI3_HOST to avoid
  sharing the display's bus (S3 has SPI2 + SPI3 general-purpose hosts).

## Net
The exact hub GPIOs are now known for the board's own peripherals. Remaining
hub wiring work is: (a) switch the expander assumption to TCA9554, (b) validate
every FHAdvanced radio GPIO against the V2 schematic for collisions with the
board peripherals above and confirm header break-out, (c) assign the radio SPI
bus to SPI3_HOST (LCD owns SPI2_HOST). fh_pins.h needs a revision pass before
the hub firmware is written.

---

# ROUND 6 — AUTHORITATIVE V2 pin matrix (from Waveshare pinout image, user-supplied)
# This is the clean human-readable pin-function table. SUPERSEDES guesses.

## Onboard-peripheral pins — RESERVED, do not reuse
IO1  LCD SPI MOSI        IO2  LCD SPI MISO     IO3  LCD DC
IO4  EXIO_INT (TCA9554)  IO5  LCD SPI SCLK     IO6  LCD BL
IO7  I2C SCL (shared: TP/AU/ESP/RTC/IMU/AXP)   IO8  I2C SDA (shared)
IO9  SD D0               IO10 SD CMD           IO11 SD CLK
IO12 I2S MCLK  IO13 I2S SCLK  IO14 I2S ASDOUT  IO15 I2S LRCK  IO16 I2S DSDIN (ES8311)
IO17 CAM VSYNC  IO18 CAM HREF  IO19 USB D-(U_N)  IO20 USB D+(U_P)
IO21 CAM D7  IO38 CAM XCLK  IO39 CAM D6  IO40 CAM D5  IO41 CAM PCLK  IO42 CAM D4
IO43 ESP TXD (UART0 console)  IO44 ESP RXD (UART0 console)
IO45 CAM D0  IO46 CAM D3  IO47 CAM D1  IO48 CAM D2
TCA9554 expander lines (ALL 8 used): EXIO0 CAM_PWDN, EXIO1 LCD_RST, EXIO2 TP_INT,
  EXIO3 SD_CD, EXIO4 RTC_INT, EXIO5 AXP_IRQ, EXIO6 SYS_OUT, EXIO7 PA_CTRL.

## PIN_OUT header (2x16, 2.54mm) — what is actually BROKEN OUT
IO0, ESP_SCL(IO7), ESP_SDA(IO8), ESP_MISO(IO9), ESP_MOSI(IO10), ESP_SCLK(IO11),
IO17, IO18, U_N(IO19), U_P(IO20), IO21, IO38, IO39, IO40, IO41, IO42,
ESP_TXD(IO43), ESP_RXD(IO44), IO45, IO46, IO47, IO48.
NOTE: IO9/10/11 are broken out AS the SPI nets but are SHARED WITH THE SD CARD.
IO17/18/21/38-42/45-48 are broken out but are the CAMERA pins (free only if the
camera is unused). IO43/44 are the console UART.

## ★★ fh_pins.h RECONCILIATION — multiple HARD conflicts (must rewrite)
| fh_pins.h            | V2 reality                         | verdict        |
|----------------------|------------------------------------|----------------|
| SPI SCLK=18          | IO18 = CAM HREF                    | CONFLICT       |
| SPI MOSI=23          | IO23 does NOT exist on ESP32-S3    | INVALID PIN    |
| SPI MISO=19          | IO19 = USB D-                      | CONFLICT       |
| CC1101 GDO0=2        | IO2 = LCD SPI MISO                 | CONFLICT       |
| nRF24 IRQ=4          | IO4 = EXIO_INT                     | CONFLICT       |
| PN532 IRQ=17         | IO17 = CAM VSYNC (free if no cam)  | conditional    |
| nRF24 CSN=15         | IO15 = I2S LRCK                    | CONFLICT       |
| C5 UART=32/33        | IO32/33 do NOT exist on ESP32-S3   | INVALID PINS   |
| XS3 UART=43/44       | IO43/44 = UART0 console TXD/RXD    | usable (=console)|
| SD=9/10/11           | matches SD D0/CMD/CLK              | OK             |
| I2C=7/8              | matches I2C SCL/SDA                | OK             |
| EXIO3/5/6/7 for CS   | TCA9554 EXIO3/5/6/7 already used   | CONFLICT (SD_CD/AXP/SYS/PA) |

## ★★ CRITICAL DESIGN FINDING
1. fh_pins.h was written against the WRONG GPIO RANGE — it uses GPIO32/33/23,
   which exist on the classic ESP32 but NOT on the ESP32-S3 (S3 = 0-21, 26-48).
   The entire hub radio pin map must be REDONE against the S3 range + the actual
   broken-out header above.
2. The hub is FAR more pin-constrained than the design assumed. The ONLY pins
   realistically free for FHAdvanced radios are the CAMERA pins
   (IO17,18,21,38,39,40,41,42,45,46,47,48 ~ 12 pins) AND ONLY IF THE CAMERA IS
   NOT USED. There is NO spare native SPI bus separate from the SD/console.
3. The TCA9554 expander is FULLY allocated (all 8 EXIO used) — there are NO free
   expander lines for radio chip-selects. The fh_pins.h expander-CS plan is dead;
   CS lines must come from the ~12 camera GPIOs (camera sacrificed).
4. DECISION NEEDED FROM USER: sacrifice the camera to free ~12 GPIOs for the
   three SPI radios (CC1101/nRF24/PN532) + their CS/IRQ lines? If NOT, the three
   wired radios do not physically fit on the hub and the architecture needs
   rethinking (fewer hub radios, or move some to a daughterboard). This is the
   single most important open item — it gates whether the hub design is buildable
   as specced.
5. A dedicated radio SPI bus would use the camera GPIOs on SPI3_HOST (LCD keeps
   SPI2_HOST on IO1/3/5; SD is SDMMC on IO9/10/11). UART links to the
   daughterboards: only IO43/44 (console) are a real UART on the header — may
   need to move the console to USB-CDC and reuse 43/44, and find a 2nd UART from
   the camera-GPIO pool.

---

# ROUND 7 — DECISION: camera SACRIFICED (user, 2026-10-19)

User confirmed the camera is not needed. This UNBLOCKS the hub design: the ~12
camera GPIOs are now the FHAdvanced radio/UART pool.

## Freed pool (camera off)
Native GPIO: IO17, IO18, IO21, IO38, IO39, IO40, IO41, IO42, IO45, IO46, IO47,
IO48  (= 12 native pins). Plus TCA9554 EXIO0 (ex CAM_PWDN) frees one slow
expander output.

## Honest pin budget — it FITS, but tightly (not comfortably)
Demand on the hub:
- SPI3 shared bus (SCLK+MOSI+MISO) ....... 3
- CC1101 (CS + GDO0 IRQ) ................. 2
- nRF24  (CSN + CE + IRQ) ................ 3   (nRF24 needs BOTH CE and CSN)
- PN532  (CS + IRQ) ..................... 2
- UART link A to XS3 (TX+RX) ............ 2
- UART link B to C5  (TX+RX) ............ 2
  TOTAL .............................. 14 native pins
Supply: 12 native camera pins. SHORTFALL = 2.

## Closing the 2-pin gap (decisions, not guesses — finalize at fh_pins.h rewrite)
- MOVE CONSOLE TO USB-CDC → frees IO43/IO44 for one UART link (+2). Clean, normal
  on the S3 (USB-Serial-JTAG console). This alone closes the gap → design fits.
- OPTIONAL relief: put ONE slow chip-select (CC1101 or PN532 CS) on the freed
  TCA9554 EXIO0 → recovers a native pin, at the cost of the expander-CS transport
  complexity (Round 3 R3-A: bus-locked manual CS). Use only if still tight.
- LAST RESORT: drop a radio IRQ to polled mode (hurts throughput) — avoid.

## Resulting allocation plan (to be finalized in the fh_pins.h rewrite)
- LCD: SPI2_HOST, IO1/3/5 (+BL IO6) — untouched.
- SD: SDMMC 1-bit, IO9/10/11 — untouched.
- Radios: SPI3_HOST on three of {IO21,38,39,40,41,42,45,46,47,48}; CS/CE/IRQ
  from the rest + IO17/IO18.
- UART link A (XS3): IO43/IO44 (console moved to USB-CDC).
- UART link B (C5): two more from the camera pool.
- TCA9554 EXIO0 available as an optional slow CS.
STATUS: camera-off decision locked. The exact GPIO-per-signal assignment is the
fh_pins.h REWRITE — real design work, done deliberately (not bolted on). This is
the last gather/plan step before the hub pin map can be rewritten and the hub
firmware written. The design is confirmed BUILDABLE on the V2 board.

---

# ROUND 8 — CORRECTION: actual J8 2x16 header (user-supplied diagram)
# Supersedes Round 7's "12 camera pins" pool — NOT all camera GPIOs are on the
# header. This is the physical J8 connector, pin-numbered. AUTHORITATIVE.

## J8 header (odd col | even col)
 1 VBUS        | 2  VBAT
 3 GND         | 4  GND
 5 U_N (IO19)  | 6  IO21
 7 U_P (IO20)  | 8  IO38
 9 ESP_SCLK'(IO11) | 10 IO39
11 ESP_MOSI(IO10)  | 12 IO40
13 ESP_MISO(IO9)   | 14 IO42
15 IO17       | 16 IO45
17 IO18       | 18 IO46
19 IO0        | 20 IO47
21 ESP_EN     | 22 IO48
23 PWRON      | 24 ESP_TXD (IO43)
25 ESP_SCL(IO7)   | 26 ESP_RXD (IO44)
27 ESP_SDA(IO8)   | 28 GND
29 GND        | 30 VCC3V3
31 VCC3V3     | 32 VCC3V3

## Usable-for-radio GPIO on the header (the REAL pool)
Genuinely free native GPIO: IO17, IO18, IO0*, IO21, IO38, IO39, IO40, IO42,
IO45, IO46, IO47, IO48, IO43**, IO44**  = 14 pins.
  * IO0 is a STRAPPING pin (boot mode) — safe only as a boot-safe output/CS,
    never a boot-blocking input.
  ** IO43/IO44 are the UART0 CONSOLE — usable only if the console is moved to
     USB-CDC (USB-Serial-JTAG).
Shared-but-present: IO9/IO10/IO11 (pins 13/11/9) are the SPI nets BUT shared with
the SD card (SDMMC). IO7/IO8 (pins 25/27) = shared I2C. USB IO19/IO20, ESP_EN,
PWRON = not GPIO.

## ★ CORRECTION to Round 7
- IO41 is NOT on J8 (pin 12=IO40, pin 14=IO42 — 41 is skipped). Round 7's
  12-pin "camera pool" over-counted by one and is superseded by this 14-pin
  header-verified pool.

## ★ ZERO-MARGIN REALITY
Usable header GPIO = 14. Radio+UART demand = 14 (SPI3 bus 3 + CC1101 2 +
nRF24 3 + PN532 2 + two UART links 4). EXACTLY 14 = 14 — NO spare — and only if:
  (a) console -> USB-CDC to free IO43/IO44, AND
  (b) IO0 (strapping) is used as a boot-safe CS, OR the radio SPI bus rides the
      SHARED SD pins IO9/10/11 (accepting SD<->radio bus contention).
Implication: three wired SPI radios + two UART daughterboard links sit right at
the physical limit of J8. Options to buy margin (user/design decision):
  - Put the radio SPI bus on the shared SD lines IO9/10/11 (one bus for SD+radios,
    arbitrated by CS) → frees native pins but couples SD and radio timing.
  - Drop one hub radio or move it to a daughterboard.
  - Drop a radio IRQ to polled (throughput cost).
  - Use TCA9554 EXIO0 (freed ex-CAM_PWDN) for one slow CS (expander-CS complexity).
STATUS: the header is now exactly known. The fh_pins.h rewrite must fit within
THIS 14-pin pool (not the Round-7 12-pin assumption). Design is buildable but
tight — the margin tradeoff above is a real decision before/at the rewrite.
