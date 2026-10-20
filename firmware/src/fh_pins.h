/*
 * fh_pins.h — FHAdvanced HUB pin map  (REWRITTEN 2026-10-20)
 * Node: Waveshare ESP32-S3-Touch-LCD-3.5 **V2** (ST7796 LCD + FT6336 touch).
 *
 * ##########################################################################
 * # THIS FILE WAS FULLY REWRITTEN after the hardware split was redesigned.  #
 * # The hub NO LONGER carries any wired radios. All on-demand radios        #
 * # (CC1101 / nRF24 / PN532 / SX1262 LoRa) live on a dedicated Raspberry Pi #
 * # Pico (RP2040) node (see ../../firmware-pico-rp2040/). The hub is now:    #
 * # BLE (internal radio) + GPS + microSD + LCD/touch UI + the aggregator of #
 * # 3 UART feeds.                                                            #
 * #                                                                          #
 * # The previous version of this file was INVALID: it used GPIO 32/33/23    #
 * # (classic-ESP32 numbers that DO NOT EXIST on the ESP32-S3) and mapped    #
 * # radio pins onto the LCD/USB/I2S. All of that is gone.                   #
 * ##########################################################################
 *
 * FOUR-NODE SYSTEM (this file covers the Waveshare HUB only):
 *   - Waveshare S3 (HUB) : BLE sniff + GPS + microSD + UI + aggregate 3 feeds
 *   - XIAO-C5            : 5 GHz Wi-Fi sniff (continuous)           — own fw
 *   - XIAO-S3            : 2.4 GHz Wi-Fi sniff (continuous)         — own fw
 *   - Raspberry Pi Pico  : CC1101 + nRF24 + PN532 + SX1262 LoRa (on-demand)
 *                          — arduino-pico fw, see firmware-pico-rp2040/
 * All three satellite nodes stream to the hub over UART using the shared
 * fh_protocol frame format (../../components/fh_protocol/). The hub is the
 * SINGLE writer to the microSD card.
 *
 * ---------------------------------------------------------------------------
 * AUTHORITATIVE V2 PIN FACTS (from the Waveshare pinout + J8 header diagram):
 *   ONBOARD, RESERVED — never repurpose:
 *     IO1  LCD MOSI   IO3 LCD DC    IO5 LCD SCLK   IO6 LCD BL   (SPI2_HOST)
 *     IO2  LCD MISO   IO4 EXIO_INT (TCA9554 expander irq)
 *     IO7  I2C SCL    IO8 I2C SDA   (shared: touch/RTC/IMU/AXP2101/codec/expander)
 *     IO9  SD D0      IO10 SD CMD   IO11 SD CLK   (SDMMC 1-bit)
 *     IO12-16 I2S audio (ES8311)    IO19/20 USB D-/D+
 *     IO43/44 UART0 console TXD/RXD (kept as console; see note)
 *   TCA9554 expander EXIO0..7 all used by onboard functions.
 *   microSD is SDMMC (OFF the SPI bus). FT6336 touch is on I2C (OFF SPI).
 *
 * J8 2x16 HEADER — pins genuinely FREE for our use (camera NOT populated):
 *   IO17, IO18, IO21, IO38, IO39, IO40, IO42, IO45, IO46, IO47, IO48  (11)
 *   + IO0 (STRAPPING — boot-safe use only)  + IO43/IO44 (console).
 *   (These 11 are the camera data/clock pins; the camera is omitted on this
 *    build, so they are free. Verified against the J8 diagram.)
 *
 * HUB DEMAND NOW = FOUR UART links = 8 pins. 8 of 11 free -> ~3 spare. No
 * strapping pin, no SD-bus sharing, no expander needed. Comfortable fit.
 * ---------------------------------------------------------------------------
 */

#pragma once

#include <stdint.h>

/* ===========================================================================
 * UART LINK A — XIAO-C5  (5 GHz Wi-Fi capture; receive-mostly into the hub)
 * Hub TX -> C5 RX (control/time), C5 TX -> hub RX (capture records).
 * ======================================================================== */
#define FH_UART_C5_TX        17   // J8 pin 15 (IO17) — HUB -> C5
#define FH_UART_C5_RX        18   // J8 pin 17 (IO18) — C5  -> HUB

/* ===========================================================================
 * UART LINK B — XIAO-S3  (2.4 GHz Wi-Fi capture; receive-mostly into the hub)
 * ======================================================================== */
#define FH_UART_XS3_TX       21   // J8 pin 6  (IO21) — HUB -> XS3
#define FH_UART_XS3_RX       38   // J8 pin 8  (IO38) — XS3 -> HUB

/* ===========================================================================
 * UART LINK C — Raspberry Pi Pico  (on-demand radio node; BIDIRECTIONAL)
 * Hub sends prompts (do sub-GHz burst / NFC read / LoRa relay TX) and the
 * Pico returns results + LoRa relay RX. See the command set in
 * ../../components/fh_protocol/ (fh_ondemand.h).
 * ======================================================================== */
#define FH_UART_PICO_TX      39   // J8 pin 10 (IO39) — HUB -> Pico (prompts)
#define FH_UART_PICO_RX      40   // J8 pin 12 (IO40) — Pico -> HUB (results)

/* ===========================================================================
 * UART LINK D — GPS (GY-NEO6MV2 / u-blox NEO-6M), RECEIVE-ONLY in practice.
 * NMEA in; hub also TX's UBX-CFG-RATE/CFG-PRT at boot to attempt 5 Hz (falls
 * back to 1 Hz). Re-sent every boot (clone does not persist config). The hub
 * caches last-known-fix and stamps it onto every record for geotagging.
 * ======================================================================== */
#define FH_UART_GPS_TX       42   // J8 pin 14 (IO42) — HUB -> GPS (config only)
#define FH_UART_GPS_RX       45   // J8 pin 16 (IO45) — GPS -> HUB (NMEA)

/* ===========================================================================
 * SPARE free header GPIO (available for future use / status LED / etc.)
 * ======================================================================== */
// IO46 (J8 pin 18), IO47 (J8 pin 20), IO48 (J8 pin 22) — 3 spare.
// IO0  (J8 pin 19) — STRAPPING pin; boot-safe output only if ever needed.

/* ===========================================================================
 * UART CONTROLLER ASSIGNMENT (ESP32-S3 has UART0/1/2; use the GPIO matrix to
 * route any of the above pins to a controller). Suggested:
 *   UART0 = USB-CDC console (keep IO43/44 free of our links, OR move console
 *           fully to USB-Serial-JTAG and reuse 43/44 as a 5th UART if ever
 *           needed). With 4 links we do NOT need 43/44, so the console stays.
 *   The four data links share UART1/UART2 via time-multiplex OR use the S3's
 *   LP/HP UARTs; at 2 Mbaud the capture links need real HW UARTs. If only 3 HW
 *   UARTs exist, the GPS (slow, 9600–38400) can go on a software/LP UART since
 *   it is low-rate, freeing the fast HW UARTs for the 2 Mbaud capture links.
 * NOTE: finalize controller-to-pin routing in firmware init; the GPIO matrix
 * makes any pin above usable by any UART controller.
 * ======================================================================== */

/* ===========================================================================
 * RESERVED / DO NOT REPURPOSE (onboard V2 functions)
 *   IO1/2/3/5/6  — LCD (ST7796) on SPI2_HOST + DC + backlight
 *   IO4          — TCA9554 expander interrupt
 *   IO7/8        — I2C0 (touch, PCF85063 RTC, QMI8658 IMU, AXP2101, ES8311,
 *                  TCA9554 expander)
 *   IO9/10/11    — microSD (SDMMC 1-bit)
 *   IO12-16      — I2S audio (ES8311 codec)
 *   IO19/20      — USB D-/D+
 *   IO43/44      — UART0 console (left as console)
 *   GPS/time base: battery-backed PCF85063 RTC on I2C, set from GPS UTC.
 * ======================================================================== */

/* ---------------------------------------------------------------------------
 * HARDWARE NOTES (bring-up)
 * 1. Link baud: the C5 and S3 capture links run at 2 Mbaud (dedup'd binary
 *    frames). The ItsyBitsy link is low-rate (occasional prompts/results) and
 *    can run slower. The GPS link is 9600 (default) up to 38400 (5 Hz attempt).
 * 2. Console: keep on USB-CDC / USB-Serial-JTAG so it never fights the links.
 * 3. All these pins were the camera's on a camera-populated board; this build
 *    omits the camera, which is what frees them. If a camera is ever added,
 *    this pin map must change (camera + 4 UARTs do NOT both fit on J8).
 * 4. Verify each pin against YOUR board at bring-up with a continuity/boot test
 *    before trusting it — the J8 diagram is authoritative but belt-and-suspenders.
 * --------------------------------------------------------------------------- */
