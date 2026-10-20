# Raw ChatGPT research — ROUND 2 (gap-fill: Waveshare files, NEO-6M bytes, Arduino-C5, SX1262, CH422G, PN532 SPI)

> VERBATIM-derived from the user's pasted ChatGPT response. Primary source.
> Distillation in ../RESOURCES_FOUND.md (Round 2 section) + ../BUILD_DECISIONS.md.

---

## GAP 1 — Waveshare demo + schematic
- Wiki: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5
- Schematic PDF (exact board, not 3.5B): https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5-Schematic.pdf
- Source repo: https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5
- Demo folders: 01_factory (comprehensive), 02_lvgl_example, 03_lvgl_system, 04_lvgl_qmi8658, 05_lvgl_axp2101, 06_lvgl_camera, 07_lvgl_wifi, 08_lvgl_image.
- Factory init sequence: i2c_bus_init(); io_expander_init(); esp_3inch5_display_port_init(...); esp_3inch5_touch_port_init(...). ESP-IDF-style C APIs → factory demo is ESP-IDF.
- Direct ZIP URL NOT confirmed.

## GAP 2 — NEO-6M 5 Hz config
- Docs: NEO-6 datasheet https://content.u-blox.com/sites/default/files/products/documents/NEO-6_DataSheet_%28GPS.G6-HW-09005%29.pdf ; LEA-6/NEO-6/MAX-6 HIM https://content.u-blox.com/sites/default/files/products/documents/LEA-NEO-MAX-6_HIM_%28UBX-14054794%29_1.pdf
- CFG-RATE 200ms/5Hz: `B5 62 06 08 06 00 C8 00 01 00 01 00 DE 6A`  (sync B5 62, class/id 06 08, len 06 00, meas 200ms=C8 00, navRate 01 00, timeRef 01 00=GPS, cksum DE 6A).
- CFG-PRT UART1 38400 8N1 (CONSTRUCTED, checksum unverified vs NEO-6 spec): `B5 62 06 00 14 00 01 00 00 00 D0 08 00 00 00 96 00 00 03 00 03 00 00 00 00 00 8F 70`.
- Two-stage baud switch: send CFG-PRT at current baud, THEN switch host UART.
- Persistence: GY-NEO6MV2 clone — assume NONE; reconfigure every boot. NEO-6M max nav rate = 5 Hz (10 Hz NOT supported).

## GAP 3 — arduino-esp32 / XIAO ESP32-C5 / IDF 6.1
- Releases: https://github.com/espressif/arduino-esp32/releases ; roadmap: https://github.com/espressif/arduino-esp32/wiki/Release-Roadmap-and-Management
- First C5 support ~3.3.0-alpha1 line; stable 3.3.x is IDF 5.5.x based. CMake accepts IDF up to 6.1.99 (not proof). 4.0 line targets IDF 6.1 but UNRELEASED.
- Bottom line: for IDF 6.1, use NATIVE ESP-IDF for the C5; Arduino-on-C5-under-6.1 not a stable combo on this date.

## GAP 4 — Wio-SX1262 host wiring (CORRECTION)
- Wiki: https://wiki.seeedstudio.com/Wio-SX1262/ ; XIAO schematic: https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Schematic_Diagram_Wio-SX1262_for_XIAO.pdf ; module datasheet: https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Wio-SX1262_Module_Datasheet.pdf
- Module EXPOSES a host RF-switch signal (RF_SW1 / LORA_RF_SW1). DIO2-only-on-module is NOT confirmed for this board. DIO3/TCXO IS module-managed.
- Host pins: SCK/MOSI/MISO/NSS/RST/BUSY/DIO1 + RF_SW1 = 8.

## GAP 5 — CH422G expander
- Component registry: https://components.espressif.com/components/espressif/esp32_io_expander ; repo: https://github.com/esp-arduino-libs/ESP32_IO_Expander
- v1.1.1, lists CH422G support, needs esp-idf >= 5.1 (6.1 compat not proven). API init/begin/pinMode/digitalWrite. I2C address NOT confirmed. (NOTE: later found the board's actual expander is a TCA9554, not CH422G — see RESOURCES_FOUND Round 5.)

## GAP 6 — PN532 SPI mode
- SPI DIP-switch positions NOT confirmed (read board silkscreen).
- Adafruit-PN532 supports HW SPI with arbitrary CS: `Adafruit_PN532(uint8_t ss, SPIClass *theSPI)`. SPI Mode 0, LSB-first, 5 MHz max per NXP UM0701-02.
