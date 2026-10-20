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
