/*
 * fh_pico_pins.h — FHAdvanced Raspberry Pi Pico (RP2040, NON-W) on-demand radio
 * node pin map.
 * Board: Raspberry Pi Pico, RP2040 (dual Cortex-M0+ @133 MHz, 264 KB RAM,
 *        2 MB flash, 26 usable GPIO: GP0-GP22, GP26-GP28, two HW SPI blocks).
 *
 * This is the committed on-demand radio host for FHAdvanced. It hosts four
 * radios (CC1101 + nRF24 + PN532 + SX1262 LoRa) on
 * ONE shared hardware-SPI bus, driven one at a time, and talks to the Waveshare
 * hub over a UART carrying fh_protocol frames (see ../components/fh_protocol +
 * fh_ondemand.h). The plain Pico (not the Pico W) is chosen deliberately: the
 * RP2040's on-board resources (dual core, 264 KB RAM, 2 SPI buses) all help the
 * radio-host job, while the Pico W's CYW43439 Wi-Fi/BLE would be REDUNDANT here
 * (2.4 GHz Wi-Fi is already the XIAO-S3's job, BLE the Waveshare's) AND would
 * consume GP23/24/25/29 talking to the wireless chip. Non-W = cleaner fit.
 *
 * BUS STRATEGY
 *   - ONE hardware SPI bus (spi0, the Arduino `SPI` object) is shared by
 *     CC1101, nRF24, PN532 (SPI mode) and SX1262, each with its own chip-select;
 *     only one transaction active at a time (matches the fh_ondemand.h BUSY
 *     contract). spi1 is LEFT FREE as headroom — a future split (e.g. LoRa on
 *     its own bus) is possible but not needed.
 *   - CRITICAL RP2040 DETAIL: the Arduino `SPI` bus pins are NOT fixed — you
 *     fixed — you MUST assign them explicitly BEFORE SPI.begin() with
 *     SPI.setSCK(), SPI.setTX() (MOSI) and SPI.setRX() (MISO). The pins below
 *     are the valid spi0 SCK/MOSI/MISO group used for that. (This is the one
 *     RP2040 gotcha the research flagged: RadioLib does not set SPI pins itself,
 *     so the sketch sets them on the bus object first, then hands SPI to it.)
 *
 * These are GPIO NUMBERS ("GPn"), which under the arduino-pico core are the
 * digit you pass to the Arduino API (e.g. pin 18 == GP18). Verify against the
 * Pico silkscreen/pinout at wiring time.
 */

#pragma once

/* ===========================================================================
 * Shared hardware SPI bus = spi0 (Arduino `SPI`). Set these with
 *   SPI.setSCK(FH_PICO_SPI_SCK); SPI.setTX(FH_PICO_SPI_MOSI);
 *   SPI.setRX(FH_PICO_SPI_MISO);  SPI.begin();
 * BEFORE any radio .begin(). (spi0 valid pin group: SCK=GP18, TX=GP19,
 * RX=GP16 — one of several valid spi0 groups.)
 * ======================================================================== */
#define FH_PICO_SPI_SCK     18     // GP18 — spi0 SCK
#define FH_PICO_SPI_MOSI    19     // GP19 — spi0 TX (MOSI)
#define FH_PICO_SPI_MISO    16     // GP16 — spi0 RX (MISO)

/* ===========================================================================
 * UART to the Waveshare hub (fh_protocol frames; bidirectional).
 * GP0/GP1 = uart0 default (Arduino Serial1) TX/RX.
 * ======================================================================== */
#define FH_PICO_HUB_TX      0      // GP0  = Serial1 TX  (here -> hub RX)
#define FH_PICO_HUB_RX      1      // GP1  = Serial1 RX  (hub TX -> here)

/* ===========================================================================
 * CC1101 (sub-GHz) — SPI + CS + two GDO interrupt lines
 * ======================================================================== */
#define FH_PICO_CC1101_CS   17     // GP17 — chip-select
#define FH_PICO_CC1101_GDO0 20     // GP20 — primary packet IRQ
#define FH_PICO_CC1101_GDO2 21     // GP21 — optional 2nd status line

/* ===========================================================================
 * nRF24L01+ (2.4 GHz) — SPI + CSN + CE + IRQ  (needs CE *and* CSN)
 * ======================================================================== */
#define FH_PICO_NRF24_CSN   22     // GP22 — SPI chip-select
#define FH_PICO_NRF24_CE    26     // GP26 — chip-enable (RX/TX mode strobe)
#define FH_PICO_NRF24_IRQ   27     // GP27 — RX/TX/MAX_RT interrupt

/* ===========================================================================
 * PN532 (NFC) — SPI mode + SS + IRQ + RST
 * ======================================================================== */
#define FH_PICO_PN532_SS    28     // GP28 — chip-select
#define FH_PICO_PN532_IRQ   15     // GP15 — card-present IRQ
#define FH_PICO_PN532_RST   14     // GP14 — reset / power-down

/* ===========================================================================
 * SX1262 (LoRa) — SPI + NSS + RESET + BUSY + DIO1 + RF_SW1
 * RF_SW1 is a HOST pin on the Wio-SX1262-for-XIAO board (confirmed in research
 * — NOT fully on-module). DIO3/TCXO IS module-managed (no host pin).
 * ======================================================================== */
#define FH_PICO_SX_NSS      13     // GP13 — SPI chip-select (NSS)
#define FH_PICO_SX_RST      12     // GP12 — reset
#define FH_PICO_SX_BUSY     11     // GP11 — busy (polled during ops)
#define FH_PICO_SX_DIO1     10     // GP10 — primary IRQ/event line
#define FH_PICO_SX_RFSW1     9     // GP9  — external RF-switch control (host)

/* ===========================================================================
 * PIN BUDGET CHECK (Pico non-W: 26 usable GPIO; GP0-GP22, GP26-GP28)
 *   SPI bus (spi0) : 3 (GP18/19/16)
 *   UART to hub    : 2 (GP0/GP1)
 *   CC1101         : 3 (CS + GDO0 + GDO2)
 *   nRF24          : 3 (CSN + CE + IRQ)
 *   PN532 (SPI)    : 3 (SS + IRQ + RST)
 *   SX1262         : 5 (NSS + RST + BUSY + DIO1 + RF_SW1)  [SPI shared]
 *   -------------------------------------------------------------------
 *   TOTAL = 3 SPI + 2 UART + 14 radio signals = 19 of 26 used. 7 spare,
 *   AND the entire spi1 bus is untouched. VERY COMFORTABLE.
 *
 * NOTES
 *  - All Pico GPIO are 3.3V — matches every radio. No level shifting.
 *  - Non-W board: GP23/24/25/29 (the Pico W's wireless-chip pins) are either
 *    free or reserved for on-board functions; this map avoids them entirely.
 *  - Dual-core: the RP2040 COULD service a long LoRa TX on core1 while core0
 *    handles other on-demand radios. The current design keeps the one-at-a-time
 *    BUSY contract (fh_ondemand.h) for simplicity; dual-core parallelism is an
 *    available future optimization, not a requirement.
 *  - These GPn assignments are a DRAFT from the published Pico pinout; confirm
 *    each against the physical board + the spi0 pin-group table before
 *    soldering. GP16/18/19 are one valid spi0 group; if a chosen pin conflicts,
 *    shift within a valid spi0 group — there is slack.
 * ======================================================================== */
