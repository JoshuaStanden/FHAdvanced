# docs/hardware — primary-source pinout images

Authoritative pinout references captured during design. These are the images the
pin maps were derived from — keep them so no future session has to re-request them.

- **waveshare-s3-3.5-v2-pin-matrix.png** — Waveshare ESP32-S3-Touch-LCD-3.5 **V2**
  full GPIO function matrix (CAM/LCD/SD/Codec/EXIO/RTC/IMU/AXP2101/PIN_OUT columns).
  Source of every hub onboard-peripheral pin fact in RESOURCES_FOUND.md Round 6.
- **waveshare-s3-3.5-v2-J8-header.png** — the physical J8 2x16 (2.54mm) expansion
  header, pin 1–32. The authoritative list of what is actually BROKEN OUT. Source
  for the hub fh_pins.h UART-pin choices and the camera-vs-UART fit check (Round 8).
- **itsybitsy-m0-express-pinout.png** — Adafruit ItsyBitsy M0 Express official
  pinout. HISTORICAL REFERENCE ONLY: the ItsyBitsy was briefly the on-demand
  radio node but was retired in favor of the Raspberry Pi Pico (RP2040). The
  live on-demand pin map is firmware-pico-rp2040/fh_pico_pins.h (derived from
  the standard Pico pinout). Kept for the decision trail.

All three were user-supplied (photographed/pasted) and confirmed by reading.
The board is the V2 revision (user-confirmed); pin against the V2 schematic:
https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5_Rev2.0.pdf
