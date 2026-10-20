# FHAdvanced — What to Bring to the Firmware Build

**Read this when you're about to say "build the firmwares."** It's a shopping
list of reference material to gather FIRST, written so you don't have to
remember any of the reasoning — each item says what it is, why it matters, and
where to get it. Fill in the blanks (URLs or "saved at <path>") as you collect
them, so future-you isn't guessing.

Quick mental model: for ESP32 firmware you lean on **existing driver code and
vendor examples** far more than on raw datasheets. So the example code and
libraries below matter MORE than the PDFs. Datasheets are backup reference.

---

## TIER 1 — bring these first (each one unblocks a big chunk of work)

### 1. Waveshare ESP32-S3-Touch-LCD-3.5 — demo code + schematic
**What it is:** the vendor's example project and wiring diagram for the HUB board.
**Why it matters:** this single download is what makes the screen, touch, SD card,
and the CH422G IO-expander actually work. These parts are fiddly and the demo
code already solves them — writing them from scratch off a datasheet is slow and
error-prone. The schematic also lets the build confirm exact header pins (and
settle the GPIO26-vs-PSRAM question if it ever comes up — right now we just leave
GPIO26 unused).
**Where:** Waveshare product wiki for "ESP32-S3-Touch-LCD-3.5" → Demo/Resources
section (demo code .zip + schematic PDF).
**Got it:** _________________________________________________

### 2. RadioLib (library)
**What it is:** one well-maintained library that drives THREE of your radios —
the **SX1262 (LoRa)**, the **CC1101 (sub-GHz)**, AND the **nRF24L01+**.
**Why it matters:** instead of three separate hand-written drivers, one library
covers all three with tested code. Huge time saver and far fewer bugs.
**Where:** RadioLib on GitHub (jgromes/RadioLib) — bring the repo link + its
wiki/API docs.
**Got it:** _________________________________________________

### 3. XIAO ESP32-C5 and XIAO ESP32-S3 pinout diagrams
**What it is:** the pin maps for the two daughterboards.
**Why it matters:** the daughterboard pin maps are currently TODO stubs — the
firmware CANNOT assign pins for the SX1262, GPS, or the UART links without these.
This is what turns the stubs into real pin maps. (The HUB pin map already exists
as `firmware/src/fh_pins.h`; these two do not.)
**Where:** Seeed Studio wiki — "Seeed Studio XIAO ESP32C5" and "XIAO ESP32S3"
pages, the pinout image on each.
**Got it:** _________________________________________________

---

## TIER 2 — bring if you can, easy to pull on demand otherwise

### 4. Wio-SX1262 pinout / schematic
**What it is:** the wiring for the LoRa module.
**Why it matters:** to connect the 7 host pins correctly (SPI + NSS + RESET +
BUSY + DIO1). NOTE already settled: the antenna switch (ANT_SW) is handled
on-module via the chip's DIO2 — do NOT wire a host pin for it.
**Where:** Seeed wiki "Wio-SX1262" page (pinout + schematic + datasheet links).
**Got it:** _________________________________________________

### 5. esp32-mac-collector (reference project)
**What it is:** a real, working dual-band (2.4 + 5 GHz) Wi-Fi sniffer for the
ESP32-C5.
**Why it matters:** it is the live proof that 5 GHz promiscuous capture works on
the C5, and a concrete code reference for the least-documented capability in the
whole build. Worth reading before writing the C5 sniffer.
**Where:** GitHub — search "Grostl/esp32-mac-collector".
**Got it:** _________________________________________________

### 6. GPS module — NMEA parser library
**What it is:** a library to read the GPS module's output (TinyGPS++ or similar).
**Why it matters:** turns raw GPS sentences into lat/lon/time without you parsing
text by hand. Also note the GPS module's own model/datasheet so the build knows
how to command it to 5 Hz and the right UART baud.
**Where:** TinyGPS++ (mikalhart) on GitHub; plus whatever GPS module you actually
buy (note the model here): _______________________
**Got it:** _________________________________________________

### 7. PN532 (NFC) library
**What it is:** a driver for the NFC reader.
**Why it matters:** handles the NFC protocol so the firmware just calls read
functions. The PN532 is on SPI in this design.
**Where:** Adafruit-PN532 library (or an ESP-IDF PN532 component).
**Got it:** _________________________________________________

---

## TIER 3 — datasheets (backup reference; you'll use these LEAST)
Handy for register details and electrical specs, but the libraries above abstract
most of this away. Grab them if easy; don't stress if not.
- CC1101 datasheet (TI) — _______________________
- nRF24L01+ datasheet (Nordic) — _______________________
- PN532 datasheet (NXP) — _______________________
- SX1262 datasheet (Semtech) — _______________________
- GPS module datasheet (depends on the module you buy) — _______________________

---

## If you only grab three things
1. **Waveshare 3.5 demo code + schematic** (unlocks the whole hub).
2. **RadioLib** (unlocks SX1262 + CC1101 + nRF24 at once).
3. **The two XIAO pinout diagrams** (so the daughterboard pin maps get written,
   not just stubbed).

## Tip
Links beat downloaded PDFs where possible — a URL stays current, a PDF saved
today can go stale. Paste the actual links into the "Where/Got it" blanks above
as you collect them, so this file becomes your ready-to-go gather list.
