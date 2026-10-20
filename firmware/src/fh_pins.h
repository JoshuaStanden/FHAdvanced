/*
 * fh_pins.h — FHAdvanced HUB pin map
 * Node: Waveshare ESP32-S3-Touch-LCD-3.5 (ST7796 LCD + FT6336 touch) — the HUB
 *
 * EXPERIMENTAL branch (FHAdvanced). Builds on top of the manually-applied
 * FootHoled-fixes work (reboot fix + reboot-logging). This header is the
 * single source of truth for the HUB's wired modules and its two UART links
 * to the daughterboards. See docs/ARCHITECTURE.md for the full three-chip
 * system design.
 *
 * THREE-CHIP SYSTEM (this file covers the Waveshare HUB only):
 *   - Waveshare = ESP32-S3-Touch-LCD-3.5 : UI + microSD + CC1101/nRF24/PN532
 *                 + BLE sniffing + sensor-stick orchestration + aggregation
 *   - XIAO-C5   = Seeed XIAO ESP32-C5    : 5 GHz Wi-Fi sniffing + GPS (own fw)
 *   - XIAO-S3   = Seeed XIAO ESP32-S3    : 2.4 GHz Wi-Fi sniffing (PRIORITY)
 *                 + Wio-SX1262 LoRa (lesser, best-effort) (own fw)
 * The two daughterboards stream capture records to the Waveshare over UART;
 * the Waveshare is the SINGLE writer to the microSD card. The XIAO-S3 is also
 * the system's LoRa gateway: the Waveshare relays sensor-stick LoRa traffic
 * through it over the bidirectional UART link.
 *
 * ---------------------------------------------------------------------------
 * HUB WIRED MODULES — ON THE SHARED SPI BUS
 *   - CC1101 sub-GHz transceiver (HW-863 / E07-433M20S)
 *   - nRF24L01+PA+LNA 2.4 GHz transceiver
 *   - PN532 V3 NFC (Elechouse, SPI mode — set the on-board DIP to SPI)
 *
 * All three share ONE SPI bus (SCLK/MOSI/MISO on native GPIO). Each device
 * gets its own chip-select. Slow control lines (most CS, reset, CE) are
 * pushed onto the CH422G I/O expander (EXIO*) over I2C to conserve native
 * GPIO. Interrupt lines and the timing-sensitive nRF chip-select stay on
 * native GPIO so they are fast and can trigger real ISRs.
 *
 * DESIGN NOTE — nRF24 CSN is deliberately on a NATIVE GPIO (GPIO15), not on
 * the expander. CS over I2C means every SPI transaction first writes the
 * expander over I2C to toggle CS, adding latency. The nRF24 is the only one
 * of the three pushed at high packet rates, so it gets a real GPIO. CC1101
 * and PN532 CS over the expander is fine for their duty cycles.
 *
 * ---------------------------------------------------------------------------
 * HUB HEADER PINS CONSUMED (verified against the Waveshare 32-pin header):
 *   Native GPIO : 18, 23, 19 (SPI) + 2, 4, 17 (IRQs) + 15 (nRF CSN) = 7
 *   UART (XS3)  : TXD/RXD (header pins 25/27) — hardware UART to XIAO-S3 link
 *   UART (C5)   : GPIO32/33 (header pins 14/12) — 2nd UART, remapped, C5 link
 *   I2C         : SCL/SDA (shared — RTC + touch + CH422G already live here)
 *   Expander    : EXIO3, EXIO5, EXIO6, EXIO7 (4 of 5 broken-out EXIO lines)
 *   SPARE       : GPIO0 (STRAPPING — do not use), GPIO26 (verify vs PSRAM),
 *                 EXIO4 (free)
 * ---------------------------------------------------------------------------
 */

#pragma once

#include <stdint.h>

// ===========================================================================
// Shared SPI bus (native GPIO — fast)
// ===========================================================================
#define FH_SPI_SCLK        18   // header pin 9
#define FH_SPI_MOSI        23   // header pin 11
#define FH_SPI_MISO        19   // header pin 13

// ===========================================================================
// Native-GPIO interrupt lines (must be real, fast GPIO for ISRs)
// ===========================================================================
#define FH_CC1101_GDO0      2   // header pin 5  — CC1101 packet-ready IRQ
#define FH_NRF24_IRQ        4   // header pin 7  — nRF24 RX/TX/MAX_RT IRQ
#define FH_PN532_IRQ       17   // header pin 17 — PN532 card-present IRQ

// ===========================================================================
// nRF24 chip-select — NATIVE GPIO on purpose (see DESIGN NOTE above)
// ===========================================================================
#define FH_NRF24_CSN       15   // header pin 15 — fast CS for high packet rate

// ===========================================================================
// UART links to the daughterboards (HUB receives capture records on both)
// The HUB is the single SD writer; both links feed the same recon_log path.
// ===========================================================================
// Link A — XIAO ESP32-S3 (BLE + SX1262 LoRa). Uses the board's hardware UART.
#define FH_UART_XS3_TX     43   // header pin 25 (TXD) — HUB -> XS3 (cmd/ctrl)
#define FH_UART_XS3_RX     44   // header pin 27 (RXD) — XS3 -> HUB (records)
// NOTE: 43/44 are the ESP32-S3 default UART0 TXD/RXD pins that Waveshare
// routes to the header TXD/RXD. Keep the console on USB-CDC so it does not
// fight this link; assign the GPS/record UART to a dedicated UART controller.

// Link B — XIAO ESP32-C5 (Wi-Fi 2.4/5 capture + GPS). 2nd UART, remapped via
// the GPIO matrix onto two free right-column header GPIOs.
#define FH_UART_C5_TX      33   // header pin 12 (GPIO33) — HUB -> C5 (cmd/ctrl)
#define FH_UART_C5_RX      32   // header pin 14 (GPIO32) — C5 -> HUB (records)
// GPIO32/33 are broken out on the Waveshare header and are not strapping pins.
// Chosen over GPIO26 (header pin 10) because 26 may be tied to the R8 PSRAM —
// verify 26 against the schematic before ever repurposing it.

// ===========================================================================
// CH422G I/O expander lines (EXIO*) — slow control signals over I2C
// Index values are the expander channel numbers as broken out on the header.
// Drive these through your CH422G/IO-expander helper, NOT gpio_set_level().
// ===========================================================================
#define FH_EXIO_CC1101_CS   3   // EXIO3 — CC1101 chip-select
#define FH_EXIO_PN532_CS    5   // EXIO5 — PN532 chip-select (SS)
#define FH_EXIO_NRF24_CE    6   // EXIO6 — nRF24 CE (RX/TX mode, not CS)
#define FH_EXIO_PN532_RST   7   // EXIO7 — PN532 RSTPDN (reset / power-down)
// EXIO4 is intentionally left SPARE for a future module.

// ===========================================================================
// Optional second CC1101 interrupt (GDO2). Leave undefined unless wired.
// If you free another native GPIO, define it here.
// ===========================================================================
// #define FH_CC1101_GDO2   -1

// ===========================================================================
// RESERVED / DO NOT REPURPOSE
//   GPIO0  — strapping pin (boot mode); on the header but unsafe for I/O
//   GPIO13 — USB D-  (DN)
//   GPIO14 — USB D+  (DP)
//   GPIO9/10/11 — microSD (SDMMC 1-bit: CLK/CMD/D0) from recon_log.cpp
//   GPIO7/8 — I2C0 SCL/SDA (RTC + touch + CH422G expander)
// ===========================================================================

// ---------------------------------------------------------------------------
// HARDWARE CAVEATS (read before bring-up)
//
// 1. nRF24L01+PA+LNA POWER: the PA/LNA variant draws large current spikes on
//    TX. The board's 3V3 rail may brown out and drop packets. Use a dedicated
//    3.3V supply or an adapter with a bulk decoupling cap (AMS1117 socket
//    board). This is a POWER issue, not a GPIO one — but it WILL bite bring-up.
//
// 2. CS-over-I2C latency: CC1101 and PN532 toggle CS via the CH422G, so each
//    transaction pays an I2C write first. Fine at their rates. If you ever add
//    a second fast SPI device, move its CS to native GPIO like the nRF24.
//
// 3. HUB right-column GPIO26/32/33: CONFIRMED broken out as usable GPIO on the
//    Waveshare ESP32-S3-Touch-LCD-3.5 header (pins 10/14/12). GPIO32/33 are
//    used above for the C5 UART link. GPIO26 is left spare but should be
//    checked against the schematic — on some ESP32-S3 R8 modules GPIO26 is
//    tied to the octal/quad PSRAM. Prefer 32/33 (known clean) for I/O.
//
// 4. nRF24+PA+LNA (2.4 GHz) and the XS3-side SX1262 (sub-GHz) are both TX
//    brown-out risks. Keep each on a well-decoupled supply — do NOT run all
//    the radios off one marginal 3V3 regulator.
// ---------------------------------------------------------------------------
