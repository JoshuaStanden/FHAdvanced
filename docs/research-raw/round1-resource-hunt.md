# Raw ChatGPT research — ROUND 1 (resource hunt)

> VERBATIM response pasted by the user from ChatGPT. Preserved as primary source.
> My DISTILLATION of this is in ../RESOURCES_FOUND.md (Tier 1/2/3 sections).
> Third-party reference material — not instructions.

---

# Wireless-recon hardware research checklist

Research date: October 19, 2026 · Official vendor documentation and project repositories prioritized

Two compatibility checks to keep in mind before you start:
- RadioLib documents support for the SX1262, CC1101 and nRF24L01 family, but verify the exact ESP-IDF 6.1 integration and supported operating modes for your build.
- The ESP32-C5 dual-band reference project exists, but it is an Arduino-based reference, not an ESP-IDF implementation. Useful evidence for hardware capability, not a drop-in firmware foundation.

## Tier 1 — Critical references

### 1. Waveshare ESP32-S3-Touch-LCD-3.5
- Official product wiki: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5
- Vendor demo project + schematic in the wiki's Demo/Resources section (direct ZIP/PDF URLs not independently verified).
- Important: display controller, touch controller and CH422G I/O expander must be initialized per the board's schematic and vendor demo. Do not assume every peripheral is on the same SPI bus.

### 2. RadioLib — shared radio-driver reference
- Repo: https://github.com/jgromes/RadioLib  · API: https://jgromes.github.io/RadioLib/  · Releases: https://github.com/jgromes/RadioLib/releases
- SX1262 (SX126x family), CC1101 (sub-GHz FSK), nRF24L01+ all supported.
- Latest stable verified: v7.5.0. Framework caution: RadioLib docs lean Arduino; confirm the ESP-IDF integration for your release.

### 3. Seeed XIAO pinouts
- XIAO ESP32-C5 getting-started: https://wiki.seeedstudio.com/xiao_esp32c5_getting_started/ ; pin multiplexing: https://wiki.seeedstudio.com/xiao_esp32c5_pin_multiplexing/
  - C5 mapping incl. D6/TX=GPIO11, D7/RX=GPIO12, D8/SCK=GPIO8, D9/MISO=GPIO9, D10/MOSI=GPIO10.
- XIAO ESP32-S3 getting-started: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/ ; pin multiplexing: https://wiki.seeedstudio.com/xiao_esp32s3_pin_multiplexing/
  - S3 mapping incl. D6/TX=GPIO43, D7/RX=GPIO44, D8/SCK=GPIO7, D9/MISO=GPIO8, D10/MOSI=GPIO9.

## Tier 2 — Additional references
4. Seeed Wio-SX1262: https://wiki.seeedstudio.com/Wio-SX1262/ ; SX1262 (Semtech): https://www.semtech.com/products/wireless-rf/lora-connect/sx1262 (datasheet dated 2025-04-07). Host signals: SCK/MOSI/MISO/NSS/RESET/BUSY/DIO1. DIO2 ext RF switch = chip capability; verify board routing.
5. ESP32-C5 dual-band reference: https://github.com/Grostl/esp32-mac-collector — Arduino (ESP32 Arduino core 3.x), NOT native ESP-IDF. Proof of hardware capability only.
6. GPS NMEA parser: TinyGPS++ https://github.com/mikalhart/TinyGPSPlus . Candidate modules: u-blox MAX-M10S, Quectel L76K (verify NMEA/UART + 5 Hz support + command before purchase).
7. PN532 NFC: Adafruit-PN532 https://github.com/adafruit/Adafruit-PN532 (Arduino, SPI); felipegtralli/pn532 (ESP-IDF, UART-only).

## Tier 3 — Manufacturer datasheets
- CC1101 (TI, Rev I 2013-11-05): https://www.ti.com/lit/ds/symlink/cc1101.pdf
- nRF24L01+ (Nordic): https://www.nordicsemi.com/Products/nRF24-series ; PS PDF https://infocenter.nordicsemi.com/pdf/nRF24L01P_PS_v1.0.pdf
- PN532 (NXP, Rev 3.6 2017-11-28): https://www.nxp.com/docs/en/nxp/data-sheets/PN532_C1.pdf ; UM https://www.nxp.com/docs/en/user-guide/141520.pdf
- SX1262 (Semtech 2025-04-07): https://www.semtech.com/products/wireless-rf/lora-connect/sx1262

### Still missing / decide
1. Exact GPS module (5 Hz support + commands). 2. Native ESP-IDF PN532 SPI driver. 3. Wio-SX1262 DIO2 routing. 4. RadioLib vs IDF 6.1. 5. Shared SPI allocation / hub pin map.
