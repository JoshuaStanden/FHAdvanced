/*
 * fh_pico_compiletest.ino
 * -----------------------------------------------------------------------------
 * FHAdvanced — Raspberry Pi Pico (RP2040, NON-W) "on-demand radio node"
 * COMPILE TEST (not a product).
 *
 * PURPOSE: prove a plain Raspberry Pi Pico (RP2040: dual Cortex-M0+ @133 MHz,
 * 264 KB RAM, 2 MB flash) can hold the driver code for ALL FOUR on-demand
 * radios at once, with ample RAM/flash headroom:
 *     - SX1262   (LoRa)      via RadioLib
 *     - CC1101   (sub-GHz)   via RadioLib
 *     - nRF24L01+ (2.4 GHz)  via RadioLib
 *     - PN532    (NFC)       via Adafruit-PN532
 *
 * This is the compile test for the Pico on-demand radio node. The RP2040 has
 * 264 KB RAM, so RAM is not expected to be the limit here — this test mainly
 * confirms the four drivers BUILD and LINK on the arduino-pico core, and that
 * the RP2040 custom-SPI-pin pattern is wired right.
 *
 * It needs NO hardware attached. Open it in the Arduino IDE, select the Raspberry
 * Pi Pico board (arduino-pico core), press Verify (checkmark), then read the two
 * lines the compiler prints:
 *
 *     "Sketch uses X bytes (Y%) of program storage space. Maximum is 2093056 bytes."
 *     "Global variables use Z bytes (W%) of dynamic memory ... Maximum is 262144 bytes."
 *
 * Report X/Y and Z/W back. Interpretation (RAM is 256 KB on the RP2040):
 *     - Flash (program storage): ~2 MB. Plenty.
 *     - RAM (dynamic memory): 256 KB. Four radio drivers are tens of KB at most,
 *       so expect a LOW percentage. Anything < 50% is comfortable; this board is
 *       not RAM-constrained for this job.
 *
 * NOTE: the pins below ARE the draft real pin map (fh_pico_pins.h), not random
 * placeholders — this test doubles as a sanity check that the pin map header
 * compiles and the SPI-pin assignment pattern is correct. It still never talks
 * to a radio (begin() may error with nothing attached — that is FINE here).
 *
 * REQUIRED INSTALLS (all via the Arduino IDE's own managers):
 *   Boards Manager  : "Raspberry Pi Pico/RP2040" by Earle F. Philhower, III
 *                     (add the board-index URL below in Preferences first).
 *   Library Manager : "RadioLib"  and  "Adafruit PN532"  (PN532 pulls in
 *                     Adafruit BusIO as a dependency — let the IDE add it).
 *   Board selection : Tools > Board > Raspberry Pi Pico/RP2040 > "Raspberry Pi Pico".
 *                     (The plain Pico, NOT "Raspberry Pi Pico W".)
 * -----------------------------------------------------------------------------
 */

#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <Adafruit_PN532.h>

#include "fh_pico_pins.h"

// ---- Radio driver instances (this is what we are proving BUILDS + LINKS) ----
// RadioLib Module(cs, irq, rst, gpio) signatures vary by radio; these match the
// common RadioLib API. If a signature differs in your RadioLib version, the
// compiler error will say so — report it and we adjust.
SX1262 lora   = new Module(FH_PICO_SX_NSS, FH_PICO_SX_DIO1, FH_PICO_SX_RST, FH_PICO_SX_BUSY);
CC1101 subghz = new Module(FH_PICO_CC1101_CS, FH_PICO_CC1101_GDO0, RADIOLIB_NC, FH_PICO_CC1101_GDO2);
nRF24  nrf    = new Module(FH_PICO_NRF24_CSN, FH_PICO_NRF24_IRQ, FH_PICO_NRF24_CE);

// PN532 over hardware SPI with its chip-select (Adafruit-PN532).
Adafruit_PN532 nfc(FH_PICO_PN532_SS, &SPI);

// Keep the instances from being optimized away so the linker really counts them.
volatile uint32_t keepalive = 0;

void setup() {
  Serial.begin(115200);

  // ---- RP2040-SPECIFIC: assign spi0 pins BEFORE SPI.begin() ----------------
  // On the RP2040 the Arduino `SPI` bus pins are NOT fixed. They must be set on
  // the bus object first; RadioLib and Adafruit-PN532 then use that configured
  // bus. This is the single biggest difference from the M0 sketch, and the one
  // thing most likely to be wrong — so the compile test exercises it directly.
  SPI.setSCK(FH_PICO_SPI_SCK);
  SPI.setTX(FH_PICO_SPI_MOSI);   // TX == MOSI
  SPI.setRX(FH_PICO_SPI_MISO);   // RX == MISO
  SPI.begin();

  // UART to the hub (Serial1) on GP0/GP1.
  Serial1.setTX(FH_PICO_HUB_TX);
  Serial1.setRX(FH_PICO_HUB_RX);
  Serial1.begin(115200);

  // Touch each driver so the linker pulls in its code + statics.
  // (begin() may return an error with no hardware attached — that is FINE here;
  //  we only care that this COMPILES and LINKS.)
  int16_t a = lora.begin();
  int16_t b = subghz.begin();
  int16_t c = nrf.begin();
  bool    d = (nfc.getFirmwareVersion() != 0);

  keepalive += (uint32_t)a + (uint32_t)b + (uint32_t)c + (d ? 1 : 0);

  Serial.print("compile-test references: ");
  Serial.println(keepalive);
}

void loop() {
  // Nothing. This is a compile/RAM test, not a runtime test.
  delay(1000);
}
