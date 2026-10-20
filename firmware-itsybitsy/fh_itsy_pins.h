/*
 * fh_itsy_pins.h — FHAdvanced ItsyBitsy M0 on-demand radio node pin map
 * Board: Adafruit ItsyBitsy M0 Express (ATSAMD21G18, 3.3V, 48 MHz).
 *
 * Assigned from the official Adafruit ItsyBitsy M0 Express pinout
 * (https://www.adafruit.com/product/3727). This node hosts FOUR radios on ONE
 * shared hardware-SPI bus, driven one at a time (single-core), and talks to the
 * Waveshare hub over a UART carrying fh_protocol frames (see fh_ondemand.h).
 *
 * BUS STRATEGY
 *   - ONE hardware SPI bus (the board's dedicated SCK/MOSI/MISO pads) is shared
 *     by CC1101, nRF24, PN532 (SPI mode), and SX1262. Each radio has its own
 *     chip-select; only one transaction is active at a time.
 *   - Every radio's fast IRQ/BUSY/GDO line is on a normal digital pin (all M0
 *     GPIO are interrupt-capable per the pinout). The nRF24 needs BOTH CE and
 *     CSN; the SX1262 needs NSS+RESET+BUSY+DIO1+RF_SW1 (8 host signals total,
 *     SPI shared) — this board has ample pins for all of it.
 *   - UART to the hub uses a SERCOM in UART mode (the M0's D0/D1 default UART,
 *     or any SERCOM-capable pad — see the pinout's SERCOM columns).
 *
 * These are Arduino "Dx"/"Ax" pin NAMES (the sketch is Arduino/SAMD). They are
 * NOT raw port numbers. Verify against the board silkscreen at wiring time.
 */

#pragma once

/* ===========================================================================
 * Shared hardware SPI bus (dedicated SPI pads on the ItsyBitsy M0)
 * Use the Arduino `SPI` object; these are its fixed SCK/MOSI/MISO pads.
 * ======================================================================== */
#define FH_ITSY_SPI_SCK     SCK    // dedicated SPI clock pad
#define FH_ITSY_SPI_MOSI    MOSI   // dedicated SPI MOSI pad
#define FH_ITSY_SPI_MISO    MISO   // dedicated SPI MISO pad

/* ===========================================================================
 * UART to the Waveshare hub (fh_protocol frames; bidirectional)
 * D0/D1 are the M0's default Serial1 RX/TX (SERCOM UART).
 * ======================================================================== */
#define FH_ITSY_HUB_RX      0      // D0  = Serial1 RX  (hub TX -> here)
#define FH_ITSY_HUB_TX      1      // D1  = Serial1 TX  (here -> hub RX)

/* ===========================================================================
 * CC1101 (sub-GHz) — SPI + CS + two GDO interrupt lines
 * ======================================================================== */
#define FH_ITSY_CC1101_CS   5      // D5  — chip-select
#define FH_ITSY_CC1101_GDO0 7      // D7  — primary packet IRQ
#define FH_ITSY_CC1101_GDO2 9      // D9  — optional 2nd status line

/* ===========================================================================
 * nRF24L01+ (2.4 GHz) — SPI + CSN + CE + IRQ  (needs CE *and* CSN)
 * ======================================================================== */
#define FH_ITSY_NRF24_CSN   10     // D10 — SPI chip-select
#define FH_ITSY_NRF24_CE    11     // D11 — chip-enable (RX/TX mode strobe)
#define FH_ITSY_NRF24_IRQ   12     // D12 — RX/TX/MAX_RT interrupt

/* ===========================================================================
 * PN532 (NFC) — SPI mode + SS + IRQ
 * (On this node PN532 can be SPI or I2C; SPI chosen to share the one bus.
 *  If switched to I2C, free D13/A0 below and use SDA/SCL instead.)
 * ======================================================================== */
#define FH_ITSY_PN532_SS    13     // D13 — chip-select
#define FH_ITSY_PN532_IRQ   A0     // A0  — card-present IRQ
#define FH_ITSY_PN532_RST   A1     // A1  — reset / power-down

/* ===========================================================================
 * SX1262 (LoRa) — SPI + NSS + RESET + BUSY + DIO1 + RF_SW1
 * RF_SW1 is a HOST pin on the Wio-SX1262-for-XIAO board (confirmed in research
 * — it is NOT fully on-module). DIO3/TCXO IS module-managed (no host pin).
 * ======================================================================== */
#define FH_ITSY_SX_NSS      A2     // A2  — SPI chip-select (NSS)
#define FH_ITSY_SX_RST      A3     // A3  — reset
#define FH_ITSY_SX_BUSY     A4     // A4  — busy (polled during ops)
#define FH_ITSY_SX_DIO1     A5     // A5  — primary IRQ/event line
#define FH_ITSY_SX_RFSW1    2      // D2  — external RF-switch control (host)

/* ===========================================================================
 * PIN BUDGET CHECK (ItsyBitsy M0 Express has 23 GPIO)
 *   SPI bus        : 3 (dedicated pads)
 *   UART to hub    : 2 (D0/D1)
 *   CC1101         : 3 (CS + GDO0 + GDO2)
 *   nRF24          : 3 (CSN + CE + IRQ)
 *   PN532 (SPI)    : 3 (SS + IRQ + RST)
 *   SX1262         : 5 (NSS + RST + BUSY + DIO1 + RF_SW1)  [SPI shared]
 *   -------------------------------------------------------------------
 *   TOTAL non-SPI-bus signals = 16 + 3 SPI = well within 23. Spare remains
 *   (e.g. D3/D4 and the APA102 dotstar pins are untouched). COMFORTABLE.
 *
 * NOTES
 *  - All M0 GPIO are 3.3V — matches every radio. No level shifting.
 *  - Single-core: radios are serviced one at a time (see fh_ondemand.h BUSY
 *    contract). A long LoRa TX blocks other radio ops briefly — acceptable
 *    because none of these radios is a continuous capture stream.
 *  - These Dx/Ax assignments are a DRAFT from the published pinout; confirm
 *    each against the physical board silkscreen before soldering. If a chosen
 *    pin turns out to conflict with the DotStar/USB/bootloader, shift it to a
 *    free neighbour — there is slack.
 * ======================================================================== */
