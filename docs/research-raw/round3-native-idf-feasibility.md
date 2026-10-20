# Raw ChatGPT research — ROUND 3 (native ESP-IDF 6.1 driver feasibility)

> VERBATIM-derived from the user's pasted ChatGPT response. Primary source.
> Distillation in ../RESOURCES_FOUND.md (Round 3) + ../BUILD_DECISIONS.md.
> NOTE: this round predates the hardware redesign that moved all wired radios to
> the ItsyBitsy M0. The expander-CS / shared-SPI concerns below applied to the
> OLD plan (radios on the hub) and are largely moot now, but the per-chip native
> -IDF facts remain valid reference.

---

## Q1 — RadioLib as native ESP-IDF 6.1 component
- Native HAL EXISTS: src/hal/ESP-IDF/EspHal.h, selected via RADIOLIB_BUILD_GENERIC + ESP_PLATFORM. Repo https://github.com/jgromes/RadioLib
- Clean IDF 6.1 build NOT independently confirmed — pin a release + compile its HAL in a minimal 6.1 project first.
- Shared SPI bus across modules: architecturally yes (ESP-IDF spi_master is multi-device). Each module needs correct device handle + config.
- CH422G-controlled CS: NOT confirmed supported — RadioLib's HAL assumes a GPIO CS. Treat expander-CS as needing custom handling.

## Q2 — PN532 over SPI on native IDF 6.1  (REVERSAL vs Round 2)
- Native SPI-capable PN532 components DO exist in the registry:
  - jef-sure/pn532 v0.7.4 (BEST): SPI + I2C + UART, shared-bus docs, needs IDF >= 5.2. https://components.espressif.com/components/jef-sure/pn532
  - garag/esp-idf-pn532 v0.2.1 (SPI/I2C/HSU). victorobaro1/pn532-spi-idf v1.0.0 (S3-targeted, custom license).
- PN532 SPI facts: Mode 0, LSB-first, 5 MHz max (1 MHz default), whole-frame transaction, NSS-low-before-wake. Refs: NXP UM0701-02 §6.1.1.1/§6.2/§6.3.4/§7.2.11; AN10609.
- Expander-CS for PN532: electrically plausible if NSS held through the whole transaction + bus serialized, but not confirmed with the existing component; bench-test.

## Q3 — ESP32-C5 5 GHz promiscuous on IDF 6.1
- Works natively at API level. Docs: https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32c5/api-guides/wifi-driver/overview.html ; esp_wifi ref https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32c5/api-reference/network/esp_wifi.html
- APIs: esp_wifi_set_band_mode() (WIFI_BAND_MODE_5G_ONLY), esp_wifi_set_channel(primary, second), esp_wifi_set_promiscuous(true), esp_wifi_set_promiscuous_rx_cb(). set_channel AFTER wifi start.
- IDF-6.0 HE/secondary-channel reception quirk: could NOT confirm fixed in 6.1. Plan: test 5 GHz at 20 MHz / WIFI_SECOND_CHAN_NONE first, verify HE reception, then try secondary-channel.
- C5 CANNOT do 2.4 + 5 GHz simultaneously (single radio). 5 GHz-only avoids band-switch; still channel-hop within 5 GHz.

## Q4 — Waveshare hub microSD + SPI coexistence
- Could NOT confirm SDMMC vs SDSPI from docs text at the time. (Later resolved via demo source: SDMMC 1-bit on IO9/10/11, OFF the SPI bus.)
- ST7796 native driver: espressif/esp_lcd_st7796 v1.5.0. SPI-host assignment per board not confirmed from text (later: LCD on SPI2_HOST).

## Q5 — CH422G as ESP-IDF component
- espressif/ESP32_IO_Expander v1.1.1 lists CH422G, needs IDF >= 5.1 (6.1 not proven). C++ API init/begin/pinMode/digitalWrite/multiDigitalWrite. Exact CH422G constructor + board I2C address not confirmed.
- (Superseded: the board's expander is actually a TCA9554 per the demo source.)

## Q6 — native-IDF 6.1 risk ranking (OLD radios-on-hub plan)
Low→high: (1) C5 5 GHz sniffer → (2) microSD/CSV → (3) SX1262 via RadioLib → (4) CC1101+nRF24 shared bus → (5) ST7796+FT6336+CH422G bring-up → (6) PN532 SPI + expander CS (highest).
(The redesign later removed all hub radios, dissolving items 3/4/6's hub pin pressure.)
