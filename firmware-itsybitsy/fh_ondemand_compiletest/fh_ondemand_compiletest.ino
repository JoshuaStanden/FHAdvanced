/*
 * fh_ondemand_compiletest.ino
 * -----------------------------------------------------------------------------
 * FHAdvanced — ItsyBitsy M0 "on-demand radio node" COMPILE TEST (not a product).
 *
 * PURPOSE: prove the Adafruit ItsyBitsy M0 Express (ATSAMD21G18, 48 MHz,
 * 32 KB RAM, 256 KB flash) can hold the driver code for ALL FOUR on-demand
 * radios at once, with enough RAM/flash headroom to be usable:
 *     - SX1262  (LoRa)        via RadioLib
 *     - CC1101  (sub-GHz)     via RadioLib
 *     - nRF24L01+ (2.4 GHz)   via RadioLib
 *     - PN532   (NFC)         via Adafruit-PN532
 *
 * This sketch does NOT need any hardware attached. It is a VERIFY/COMPILE test:
 * open it in the Arduino IDE, select the ItsyBitsy M0 board, and press Verify
 * (the checkmark). Then read the two lines the compiler prints at the end:
 *
 *     "Sketch uses X bytes (Y%) of program storage space. Maximum is 262144 bytes."
 *     "Global variables use Z bytes (W%) of dynamic memory ... Maximum is 32768 bytes."
 *
 * Report X/Y and Z/W back. Interpretation:
 *     - Flash (program storage): 256 KB. Plenty; unlikely to be the limit.
 *     - RAM (dynamic memory): 32 KB total. THIS is the number that matters.
 *         < 60% used  -> comfortable, the 4-radio node is viable.
 *         60-80% used -> tight but workable (watch runtime buffers/stack).
 *         > 80% used  -> risky; the node may not hold all four + working buffers.
 *
 * NOTE: the pin numbers below are PLACEHOLDERS so the constructors compile.
 * They are NOT the real wiring — the real ItsyBitsy pin map is a later step.
 * This test only exercises the compiler/linker (does code + RAM fit), it never
 * talks to a radio.
 *
 * REQUIRED INSTALLS (all via the Arduino IDE's own managers):
 *   Boards Manager  : "Arduino SAMD Boards" + "Adafruit SAMD Boards"
 *   Library Manager : "RadioLib"  and  "Adafruit PN532"  (PN532 pulls in
 *                     Adafruit BusIO as a dependency — let the IDE add it).
 *   Board selection : Tools > Board > Adafruit SAMD > "Adafruit ItsyBitsy M0".
 * -----------------------------------------------------------------------------
 */

#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <Adafruit_PN532.h>

// ---- PLACEHOLDER pins (compile-only; NOT the real wiring) -------------------
// Just distinct, valid Dx numbers so the constructors are well-formed.
#define PIN_SX_NSS    1
#define PIN_SX_DIO1   2
#define PIN_SX_RST    3
#define PIN_SX_BUSY   4

#define PIN_CC_CS     5
#define PIN_CC_GDO0   7     // (D6 avoided arbitrarily; any valid pin is fine)
#define PIN_CC_GDO2   9

#define PIN_NRF_CS    10
#define PIN_NRF_IRQ   11
// nRF24 over RadioLib also needs a CE-equivalent; RadioLib's nRF24 module uses
// (cs, irq, ce). Provide a CE placeholder:
#define PIN_NRF_CE    12

#define PIN_PN532_SS  13    // PN532 in SPI mode (Adafruit lib, HW SPI + CS)

// ---- Radio driver instances (this is what we are proving FITS in RAM) -------
// RadioLib Module(cs, irq, rst, gpio) signatures vary by radio; these match the
// common RadioLib API. If a signature differs in your RadioLib version, the
// compiler error will say so — report it and we adjust.
SX1262 lora   = new Module(PIN_SX_NSS, PIN_SX_DIO1, PIN_SX_RST, PIN_SX_BUSY);
CC1101 subghz = new Module(PIN_CC_CS, PIN_CC_GDO0, RADIOLIB_NC, PIN_CC_GDO2);
nRF24  nrf    = new Module(PIN_NRF_CS, PIN_NRF_IRQ, PIN_NRF_CE);

// PN532 over hardware SPI with an arbitrary chip-select (Adafruit-PN532).
Adafruit_PN532 nfc(PIN_PN532_SS, &SPI);

// Keep the instances from being optimized away so the linker really counts them.
volatile uint32_t keepalive = 0;

void setup() {
  Serial.begin(115200);

  // Touch each driver so the linker pulls in its code + statics.
  // (begin() may return an error with no hardware attached — that is FINE here;
  //  we only care that this COMPILES and LINKS within 32 KB RAM.)
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
