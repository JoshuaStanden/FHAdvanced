# FHAdvanced — Architecture Notes (locked four-node design)

Experimental branch of FootHoled. Builds on top of the manually-applied
FootHoled-fixes work (reboot fix + reboot-logging).

Status: four-node HARDWARE TOPOLOGY locked + pin-verified (2026-10-20). Nothing
flashed yet. Waveshare hub pin map lives in `firmware/src/fh_pins.h`; the
Pico pin map in `firmware-pico-rp2040/fh_pico_pins.h`; the hub↔Pico
on-demand command set in `components/fh_protocol/include/fh_ondemand.h`. This
file is the system-level design, data-flow, and SD-schema contract. For the
build starting point see `FIRMWARE_BRIEF.md`; for the decision trail see
`OPEN_DECISIONS.md` and `BUILD_DECISIONS.md`.

---

## Naming convention (use these names everywhere)

| Name | Board | Role |
|---|---|---|
| **Waveshare** | ESP32-S3-Touch-LCD-3.5 V2 (mainboard) | UI, SD, BLE sniff, GPS, orchestration, aggregation |
| **XIAO-C5** | Seeed XIAO ESP32-C5 (daughterboard) | 5 GHz Wi-Fi sniffing (continuous) |
| **XIAO-S3** | Seeed XIAO ESP32-S3 (daughterboard) | 2.4 GHz Wi-Fi sniffing (continuous) |
| **Pico** | Raspberry Pi Pico (RP2040) | On-demand radios: CC1101 + nRF24 + PN532 + SX1262 LoRa |

Four nodes. The three ESP32s build with ESP-IDF 6.1; the Pico builds with
arduino-pico (it is the one non-ESP-IDF node). All four exchange `fh_protocol`
frames over UART with the hub.

---

## Division of labor (locked)

### Waveshare — mainboard / orchestrator
- **BLE sniffing** — continuous, on the Waveshare's own internal 2.4 GHz radio.
- **GPS** — the GY-NEO6MV2 (u-blox NEO-6M) is wired to the hub over UART. The
  hub caches last-known-fix and stamps it onto every record (geotagging the
  whole log). NEO-6M: attempt 5 Hz + raised baud at boot, fall back to 1 Hz
  (re-sent every boot; the clone does not persist config). Max nav rate 5 Hz.
- **microSD** — the SINGLE log writer for the whole system.
- **3.5" LCD + touch UI**.
- **Sensor-stick orchestration** — decides BLE-vs-LoRa transport (see below).
- **Aggregates FOUR UART feeds** (XIAO-C5, XIAO-S3, Pico, GPS) into one log.
- Hosts NO wired radios itself (they live on the Pico).

### Pico — on-demand / function-event radio node
Hosts every NON-continuous radio, driven ONE AT A TIME, prompted
by the hub over UART (command set: `fh_ondemand.h`):
- CC1101 (SPI) — sub-GHz raw/OOK-ASK
- nRF24L01+PA+LNA (SPI) — 2.4 GHz transceiver
- PN532 V3 (SPI) — NFC
- **SX1262 LoRa (SPI)** — the promptable sensor-stick LoRa link (= system LoRa
  gateway). All four share one SPI bus with per-radio chip-selects.
Because none of these is a continuous capture stream, serializing them one at a
time is acceptable (a long LoRa TX briefly blocks the other on-demand radios —
fine by design; the RP2040 is dual-core, so parallelizing is a future option).

### XIAO-C5 — dedicated 5 GHz
- **5 GHz Wi-Fi sniffing, continuous** — its sole radio job. Being 5 GHz-only
  sidesteps the C5's single-band-at-a-time limit entirely (it never touches
  2.4 GHz, so no band-hopping compromise). GPS is NO LONGER on the C5 (moved to
  the hub); the C5 is a pure 5 GHz sniffer.

### XIAO-S3 — dedicated 2.4 GHz
- **2.4 GHz Wi-Fi sniffing, continuous** — its sole radio job. The SX1262 LoRa
  is NO LONGER on the S3 (moved to the Pico); the S3 is a pure 2.4 GHz
  sniffer with ample spare GPIO.

**GPS geotag strategy (every capture gets a location):** the hub holds the GPS
and caches LAST-KNOWN FIX — captures arrive far faster than GPS updates, so
NEVER gate a capture on a fresh fix; stamp the most recent fix onto every record
as it is aggregated. Zero extra cost. At 1 Hz on foot the fix is already below
the ~2.5–3 m GPS error floor; 5 Hz only tightens the vehicle case. Honest limit:
the geotag is "position at the most recent fix," not the exact capture instant —
at or below the GPS hardware error floor, so chasing tighter is not meaningful.

---

## Band coverage (why this split is clean)

Wi-Fi capture is split across two bands on two separate chips, BLE on a third:
- 2.4 GHz Wi-Fi → XIAO-S3 (internal radio)
- 5 GHz Wi-Fi  → XIAO-C5 (dedicated)
- BLE (2.4 GHz) → Waveshare (internal radio)

Three different chips, so 2.4 Wi-Fi, 5 GHz Wi-Fi, and BLE run in genuine
parallel with no RF contention. The on-demand radios — sub-GHz (CC1101), NFC
(PN532), 2.4 GHz-proprietary (nRF24) and LoRa (SX1262) — all live on the
Pico and are NOT continuous, so they do not compete for continuous airtime
with the three sniffers.

---

## Pico on-demand contract (one radio at a time)

The Pico hosts four radios but runs only ONE radio action at a time, prompted
by the hub (the RP2040 is dual-core, so this is a chosen model, not a hardware
limit):

1. **Serialize on demand:** the hub issues one request (sub-GHz burst / NFC read
   / nRF24 scan / LoRa relay) and waits for the response (or a timeout) before
   the next. A request arriving while another op runs is answered BUSY.
2. **No continuous stream here:** none of these radios is a continuous capture
   feed (those are the three sniffers on other chips), so serialization is fine.
3. **LoRa is the longest op:** a high-spreading-factor LoRa transmit can hold the
   node busy for up to ~1 s; the hub tolerates that latency on the other
   on-demand radios during a relay. LoRa is occasional (sensor-stick comms only).

The command set for all of this is `components/fh_protocol/include/fh_ondemand.h`.

---

## Sensor-stick link — dual transport (BLE + LoRa)

Mirrors the sensor-node project's dual-transport handoff: use **BLE when in
range**, **LoRa when at distance**. Transport selection logic lives on the
**Waveshare** (the orchestrator).

- **BLE path:** Waveshare talks to the sensor stick directly on its own radio.
- **LoRa path:** the Waveshare has NO LoRa radio. The only SX1262 is on the
  **Pico**, so the Pico is the **system LoRa gateway**. The Waveshare
  sends "relay this to the sensor stick over LoRa" across the Pico UART
  link (LORA_TX_REQ in `fh_ondemand.h`); the Pico transmits on the SX1262
  and relays replies back (LORA_RX_IND). The Waveshare↔Pico link is
  BIDIRECTIONAL: hub prompts down, radio results + inbound LoRa up.

---

## Topology

```
   ┌──────────────────────────┐ UART (2 Mbaud) ┌───────────────────────────────┐
   │ XIAO-C5                  │───────────────►│ Waveshare ESP32-S3 V2 (HUB)   │
   │  5 GHz Wi-Fi sniff       │                │  BLE sniff (internal radio)   │
   └──────────────────────────┘                │  GPS (NEO-6M on UART)         │
   ┌──────────────────────────┐ UART (2 Mbaud) │  3.5" LCD + touch UI          │
   │ XIAO-S3                  │───────────────►│  sensor-stick orchestration   │
   │  2.4 GHz Wi-Fi sniff     │                │  4× UART aggregator           │
   └──────────────────────────┘                │          ▼                    │
   ┌──────────────────────────┐ UART (bidir)   │  recon_log → microSD (1 CSV)  │
   │ Pico RP2040 (on-demand)  │◄──────────────►│                               │
   │  CC1101 nRF24 PN532       │  prompts down  └───────────────────────────────┘
   │  SX1262 LoRa (gateway)    │  results up     ▲
   └──────────────────────────┘        GPS (NEO-6M) UART ─┘
```
All three satellite nodes + the GPS feed the hub over UART; the hub is the single
SD writer and the only board with a screen.

---

## Interconnect — FOUR UART links into the Waveshare

The hub aggregates four UART feeds. Exact hub GPIO assignments are in
`firmware/src/fh_pins.h` (pins taken from the verified J8 header, camera omitted).

| Link | Peer | Rate | Direction |
|---|---|---|---|
| C5  | XIAO-C5 (5 GHz capture) | 2 Mbaud | mainly C5→hub (records) |
| S3  | XIAO-S3 (2.4 GHz capture) | 2 Mbaud | mainly S3→hub (records) |
| Pico| Pico (on-demand radios) | moderate | BIDIRECTIONAL (prompts down, results + inbound LoRa up) |
| GPS | GY-NEO6MV2 NEO-6M | 9600–38400 | GPS→hub NMEA (hub TX only for CFG at boot) |

Console stays on USB-CDC (USB-Serial-JTAG) so it never fights the data links.
Frames are length-prefixed, type-tagged, CRC-checked (full spec below). The two
capture links run at 2 Mbaud; the Pico link is lower-rate (occasional
prompts/results); the GPS link is slow NMEA.

---

## Data path under heavy load (DEF CON-class) — dedup + binary framing

Design target: this device may run at **DEF CON**, the most hostile RF
environment there is — tens of thousands of devices, deliberate saturation and
jamming on both bands. Raw frame rates on a saturated channel reach **tens of
thousands of frames/sec**. The three moves below are what make capture
tractable on these chips; they are REQUIREMENTS for the heavy case, not
optimizations.

### Move 1 — dedup/aggregate ON the daughterboard, BEFORE the UART
Do NOT stream raw frames to the Waveshare and filter there — that wastes the
link on data that gets discarded. Each capture chip (XIAO-C5 for 5 GHz,
XIAO-S3 for 2.4 GHz) maintains a short-window "seen" table and emits a record
only when something is **new or changed**. This is the single biggest pressure
relief: at DEF CON it can cut the emitted record rate by 10× or more, and it
puts the filtering where a dedicated core is already watching that one radio.

Seen-table contract (per daughterboard) — IDENTITY-BASED, "seen is seen"
(see `CONFIG_MENU.md` dedup model, which is authoritative):
- KEY: device identity for the frame type — typically `mac` (Wi-Fi/BLE), or
  `(mac, ssid)` for probe requests where the requested SSID is itself the
  signal worth capturing.
- VALUE: `{ last_seen_ms, first_rssi, count, first_channel }`.
- EMIT a record ONLY when the key is NEW (not currently in the seen-table).
  RSSI is NOT a trigger — it is recorded once on the sighting row, never
  re-logged on change. Channel/subtype changes are NOT triggers either. A
  device gets ONE row per re-log window. Otherwise just bump `count` and
  `last_seen_ms`. This is a PRESENCE dataset, not movement tracking.
- RE-LOG WINDOW (the one knob, `CONFIG_MENU.md` setting #4): a key is forgotten
  after N seconds quiet (default 60) and re-logs as a fresh sighting if seen
  again — OR "per session" (never forgotten, logged once until reboot).
- PERIODIC SUMMARY: every N seconds (e.g. 5–10 s) emit one summary row per
  still-active device carrying its `count` since last summary, then reset the
  counter. This collapses a thousand beacons from one AP into one row that
  still records "seen, N times."
- WINDOW EVICTION: in timed mode a key not seen for the window ages out; in
  per-session mode nothing ages out. Either way the table is bounded by max
  size; at DEF CON it WILL fill — evict oldest/weakest first, and increment a
  dropped-key counter shown on the UI so you know when you are saturating.
- The `count` field rides in the record `extra` payload (see SD schema).

### Move 2 — binary frame on the wire (expand to CSV on the hub)
A compact binary record (~40–50 B framed) vs a CSV-ish line (~80–100 B) nearly
DOUBLES effective LINK capacity for free. Stream binary over UART; the Waveshare
expands it to the CSV schema and writes CSV to the SD (the SD format is CSV, not
binary — decided, see `CONFIG_MENU.md` storage section, so the card is readable
anywhere with no decoder). Binary is a WIRE optimization only. See the frame
layout below.

### Move 3 — the SD card is the next bottleneck after the link
Even if the link and chips keep up, a cheap microSD stalls tens-to-hundreds of
ms on internal housekeeping under sustained small writes. Mitigations (the
RTC-staging + batched-flush from the reboot-logging work is already the right
pattern):
- Batch records into large sequential writes (one big append, not many small).
- Size the write buffer generously for burst absorption.
- Use a HIGH-ENDURANCE / industrial microSD (e.g. SanDisk High Endurance), not
  a bargain card — sustained small writes are exactly the workload that kills
  cheap cards.

### Honest limit
At true peak DEF CON saturation NO embedded setup captures 100% of frames —
nor does a laptop with a good adapter. You are SAMPLING, not exhausting. The
goal is "log every distinct device and its behavior," not "every packet" —
which is exactly what the dedup-first design captures, and what is actually
useful for recon.

---

## UART frame format (binary, framed, checked)

All inter-board UART traffic (the capture links and the Pico link) uses
this frame. It is self-synchronizing (a dropped byte resyncs on the next SYNC
marker rather than poisoning the stream) and type-tagged so one bidirectional
link can carry capture records, summaries, health and LoRa-relay payloads. The
hub↔Pico on-demand commands (0x20–0x3F) extend this same frame in
`fh_ondemand.h`; the TYPE table below is the capture/health core in `fh_frame.h`.
(The GPS link is NOT framed this way — the hub reads raw NMEA from the NEO-6M
directly; see the GPS notes.)

```
Offset  Size  Field      Notes
------  ----  ---------  ------------------------------------------------------
  0      2    SYNC       0xA5 0x5A — frame start marker (resync anchor)
  2      1    TYPE       message type (table below)
  3      1    LEN        payload length in bytes (0–255)
  4    LEN    PAYLOAD    type-specific body (see payload layouts)
 4+LEN   2    CRC16      CRC-16/CCITT over TYPE..end-of-PAYLOAD (not SYNC)
```

- Max frame = 2 + 1 + 1 + 255 + 2 = **261 bytes**. Capture records are far
  smaller (~40–50 B framed), well under the LEN ceiling.
- **Resync rule:** the receiver scans for `0xA5 0x5A`, reads TYPE/LEN, then
  reads LEN+2 more bytes and checks CRC16. CRC fail OR a SYNC seen mid-frame →
  discard, resume scanning for the next SYNC. A single dropped byte costs at
  most one frame, never stream alignment.
- **Byte-stuffing not required:** SYNC collisions inside the payload are
  tolerated because framing is length-driven (LEN tells the receiver exactly
  how many payload bytes to read); the mid-frame SYNC check is only a
  fast-recovery aid, not the primary delimiter. If a future transport is more
  lossy, add COBS encoding — not needed for a clean board-to-board trace.
- **CRC16/CCITT** (poly 0x1021, init 0xFFFF) — cheap on every node (the three
  ESP32s and the RP2040), catches the single/double-bit and burst errors UART
  actually produces.
- **Endianness:** little-endian for all multi-byte fields (native to every node
  — the ESP32s and the RP2040 are all little-endian — no byte-swapping).

### TYPE codes

| TYPE | Name          | Direction        | Payload |
|------|---------------|------------------|---------|
| 0x01 | CAPTURE       | C5/S3 → hub      | capture record (below) |
| 0x02 | GPS_FIX       | (RESERVED/UNUSED) | legacy from when a node held GPS; GPS is now hub-local (NEO-6M read directly), so no node sends this. Kept reserved — do not reuse the code. |
| 0x03 | SUMMARY       | C5/S3 → hub      | periodic per-device count summary |
| 0x10 | LORA_RELAY_TX | hub → Pico       | payload to transmit to sensor stick (see also fh_ondemand.h) |
| 0x11 | LORA_RELAY_RX | Pico → hub       | payload received from sensor stick |
| 0x7E | HEARTBEAT     | node → hub       | liveness counters (below) — always-on, fixed cadence |
| 0x7D | PING          | hub → node       | "ya dead, man?" — active health check on a missed beat |
| 0x7C | PONG          | node → hub       | reply to PING (echoes the PING seq) |
| 0x7F | ACK           | both ways        | ack a LORA_RELAY (seq echoed) |

"node" above = any satellite that speaks fh_protocol (XIAO-C5, XIAO-S3, or the
Pico). CAPTURE/SUMMARY come only from the two Wi-Fi sniffers; LoRa relay
only to/from the Pico; HEARTBEAT/PING/PONG from all of them.

### PAYLOAD — CAPTURE (TYPE 0x01), ~34 B

```
Offset  Size  Field        Notes
------  ----  -----------  ----------------------------------------------------
  0      1    src_id       enum: 1=c5-wifi5 2=xs3-wifi24 (maps to SD `source`)
  1      1    band_chan    high nibble = band (2.4/5), low = channel index
  2      1    frame_type   mgmt/data/ctrl + subtype (probe-req, beacon, …)
  3      1    rssi         int8, dBm
  4      6    mac          6-byte device MAC
 10      4    gps_time     u32 epoch seconds — sent as 0 by the node; NO node has
                           GPS, so the HUB fills time from its RTC on arrival
 14      4    lat          int32, deg × 1e7 — sent as 0; HUB stamps last-known fix
 18      4    lon          int32, deg × 1e7 — sent as 0; HUB stamps last-known fix
 22      2    count        u16 occurrences since last summary (dedup count)
 24     10    extra        type-specific (e.g. SSID hash, OUI flags) — var,
                           bounded; LEN covers the real length
```

### PAYLOAD — GPS_FIX (TYPE 0x02) — RESERVED / UNUSED

GPS is HUB-LOCAL in the four-node design: the NEO-6M is wired to the hub and the
hub parses NMEA itself, so no node ever sends a GPS_FIX frame. TYPE 0x02 is kept
RESERVED (do not reuse the code) in case a future node-local GPS is ever added.
The hub caches its own last-known fix and stamps lat/lon (and RTC time) onto
every record as it is aggregated — see the SD-schema section.

### PAYLOAD — LORA_RELAY (TYPE 0x10 / 0x11)

```
  0   1  seq        u8 sequence number (echoed in ACK 0x7F)
  1   1  flags      transport/priority flags
  2   N  data       opaque sensor-stick payload, N = LEN-2
```
LoRa relay is best-effort and is serviced one-at-a-time on the Pico
(per the on-demand contract). ACK (0x7F) echoes `seq` for reliable relay.

### PAYLOAD — HEARTBEAT (TYPE 0x7E), 14 B — liveness, not just presence

Sent by EACH satellite node on a fixed cadence (every 1–2 s) whether or not it
captured anything. A bare "I'm alive" is not enough — a board can be
alive-but-broken (main loop fine, capture task hung), so the heartbeat carries
proof-of-work counters:

```
Offset  Size  Field          Notes
------  ----  -------------  --------------------------------------------------
  0      1    src_id         which node (1=c5, 2=xs3, 3=pico)
  1      4    uptime_s       u32 seconds since boot — RESET TO 0 = silent reboot
  5      4    capture_count  u32 records captured since boot — must keep rising
  9      2    dropped_keys   u16 seen-table evictions (saturation indicator)
 11      1    task_alive     bitmask: per-node task liveness (capture on C5/S3;
                             radio-handler on Pico)
 12      2    free_heap_kb   u16 KB free — catch a slow leak before it crashes
```

### PAYLOAD — PING (0x7D) / PONG (0x7C), 1 B

```
  0   1  seq   u8 — PONG echoes the PING's seq
```

### Quiet-vs-crashed detection (four states, not two)

Silence is ambiguous — a board with nothing to report and a crashed board both
send nothing. The always-on heartbeat removes the ambiguity; the active PING is
the tiebreaker that rules out a single dropped frame (UART can drop bytes):

| State | Signal |
|---|---|
| Healthy | heartbeats arriving, `uptime_s` climbing, `capture_count` rising |
| Quiet-but-alive | heartbeats arriving, `uptime_s` climbing, `capture_count` flat (genuine dead air — fine) |
| Hung (alive-but-broken) | heartbeats arriving but `uptime_s` frozen OR a `task_alive` bit cleared → needs reset |
| Dead / crashed | no heartbeat for N intervals AND no PONG to an active PING |

Rule: heartbeat always on; if the hub MISSES a heartbeat it sends PING once or
twice before declaring the board DOWN (a single lost heartbeat frame ≠ dead).
A `uptime_s` that resets to 0 is how the hub detects a SILENT reboot
(brown-out / watchdog) — ties into failure-mode item #3. What the hub DOES with
a DOWN board (keep logging its own radios, flag on UI, attempt reset) is the
#3 conversation; this section covers DETECTION only.

The per-source `extra` payload contents remain to be finalized during firmware
bring-up (a parked item — see the Open hardware items list).

---

## SD logging — one writer, one schema

The microSD has EXACTLY ONE writer — the Waveshare hub. Satellite nodes never
touch SD; they stream frames. The hub parses all UART feeds (XIAO-C5, XIAO-S3,
Pico) plus its own BLE sniff, and hands every record to the SAME `recon_log`
path, reusing the reboot-logging RTC-staging + batched-flush work unchanged. GPS
(hub-local NMEA) supplies the last-known fix stamped onto every row.

Unified record (one CSV, `source`-tagged):

```
source, band, channel, type, mac, rssi, lat, lon, time, extra
```

| source | origin | geotag | clock |
|---|---|---|---|
| pico-cc1101 | Pico (sub-GHz, on-demand) | last-known (hub GPS) | Waveshare RTC |
| pico-nrf24  | Pico (2.4 GHz, on-demand) | last-known | Waveshare RTC |
| pico-nfc    | Pico (PN532, on-demand) | last-known | Waveshare RTC |
| ws-ble    | Waveshare internal radio | last-known | Waveshare RTC |
| c5-wifi5  | XIAO-C5 over its UART link | last-known (hub GPS) | Waveshare RTC |
| xs3-wifi24| XIAO-S3 over its UART link | last-known | Waveshare RTC |

(LoRa relay traffic is comms, not capture — it is not logged as a capture row
unless we choose to log a sensor-stick event.) GPS is now HUB-LOCAL (NEO-6M on a
hub UART); the hub caches "last known position" and stamps it onto every record
so the whole log is geotagged. No node produces source-exact GPS time anymore —
every row uses the hub's last-known fix + hub RTC clock, which is within the GPS
error floor (see the GPS geotag note up top).

---

## UI requirements backlog (#6 — deferred design, requirements gathered)

The UI is still DEFERRED ("future Joshua," ~6 months runway) — interaction model
(touch vs buttons, screen count, navigation) is NOT decided. But the other
resolved items have already accumulated a near-complete REQUIREMENTS list; this
is that gathered backlog so the design starts from "lay these out," not "figure
out what to show." Guiding philosophy (operator's): **silent when healthy,
specific when broken** — assume everything works unless a specific error says
otherwise. No busy dashboard.

### Status bar — per-radio color chips (assume-good-unless-flagged)
A top-of-screen row of radio indicators, glanceable:
- **GREEN** = healthy & capturing.
- **AMBER/YELLOW** = up but IMPAIRED / interfered — radio is fine, the
  ENVIRONMENT is hostile (e.g. 5 GHz getting fake DFS radar strikes forcing it
  off-channel, jamming, saturation). NOT a reboot situation — at DEF CON this
  is often the INTERESTING data, not a fault. No reboot alert.
- **RED** = a FAULT, something to act on. A red ALWAYS pops a message (below).

### Red-fault message — name WHICH kind of down (this is the key distinction)
Two different "downs," detected differently, and the message must say which so
the operator knows whether a reboot will even help:
- **An actual RADIO MODULE failed** (CC1101 / nRF24 / PN532 / SX1262 — the
  physical modules, not an ESP32): pop **"Time to reboot — an actual radio
  MODULE went down, not an ESP32."** This tells the user it is a hardware
  module fault, distinct from a node crash.
- **A whole NODE is down** (a satellite — XIAO-C5, XIAO-S3, or the Pico —
  hung or dead): the #2 up/down failure → "A capture node was lost — reboot
  everything."
Both are red; the text disambiguates module-fault vs node-down.

### Health — four states internally, UP/DOWN to the human
Heartbeat/ping (#2) tracks four states (healthy / quiet-but-alive / hung /
dead). Only HUNG and DEAD are failures, and from the operator's view they
collapse to ONE thing: node DOWN → "module down, reboot everything." Healthy
and quiet-but-alive are both fine (quiet = nothing to capture, not a problem) —
no alert. The four-state detail stays internal; the human sees up/down.

### Alerts — reuse the canonical #3 taxonomy strings, terse
Use the Failure-modes "Operator alert taxonomy" table as the SINGLE source of
the exact strings (no hand-holding tone). For reference, the set is:
- Node down → "A capture node was lost — reboot everything."
- Radio module down → "Time to reboot — an actual radio MODULE went down, not an ESP32."
- SD → "SD card has an issue — full or toast. Power down and swap it."
- Hub showstopper → "Critical error — reboot needed."
- Unclean hub restart → "Reboot everything to re-sync."
- Buffer saturation → dropped-record counter.

### Swap action (from the power item)
A "prepare to swap / SAFE TO SWAP" control: tap → flush SD, close file, halt
capture → shows SAFE TO SWAP → pull the cell. This is a UI control, not just a
readout.

### Config (#9)
The config list + the "config: STOCK / MODIFIED (N)" indicator. The exposed
settings are DECIDED — 6 settings + 4 maintenance actions, in `CONFIG_MENU.md`
(authoritative). The NVS storage mechanism is locked (see Config management).
Only the UI LAYOUT of this list is deferred to the #6 design.

### At-a-glance operational view (field use)
Per-band capture counts (2.4 / 5 / BLE), GPS-fix status, storage remaining, and
the satellite link-health indicators (C5 / S3 / Pico) — the things you want
while walking a floor.

### NOT part of this UI — antenna tuning firmware (#5)
The antenna-tuning mode is a SEPARATE, standalone firmware flashed onto the
Waveshare purely for RF bring-up ("so we know how to build future units"). It
is a bench/factory tool, NOT wired into the main capture firmware's config or
alerts, and never ships in a field unit. Lives in the repo as its own ESP-IDF
project alongside the node firmwares; version-controlled, not shipped.

---

## Verification summary

| Item | Verdict | Evidence |
|---|---|---|
| Hub UART aggregation (4 links) | PASS | J8 header: 11 free native GPIO vs 8 needed (4 UART pairs), ~3 spare, camera omitted |
| Waveshare 5 GHz capability | N/A (expected) | Waveshare docs: 2.4 GHz Wi-Fi only → XIAO-C5 required for 5 GHz |
| XIAO-C5 5 GHz promiscuous (IDF 6.1) | PASS | esp_wifi WIFI_BAND_MODE_5G_ONLY + set_channel; esp32-mac-collector proves HW |
| Pico holds all 4 on-demand radios | PASS | compile test (arduino-pico): RadioLib 7.8.1 + Adafruit PN532 1.3.4 link, 4% flash (94.6 KB/2 MB), 4% RAM (11.4 KB/256 KB) |
| SX1262 host pins | 8 (not 7) | Wio-SX1262-for-XIAO schematic exposes a host RF_SW1 line; DIO3/TCXO on-module |
| GPS on hub | PASS | NEO-6M on a hub UART; 1 Hz already < GPS error floor on foot |
| LoRa on Waveshare instead? | NO | Waveshare short 1-2 fast native GPIO (BUSY+DIO1); LoRa lives on the Pico |

---

## Open hardware items (none block the design)
1. Hub pin map: fh_pins.h was rewritten for the real ESP32-S3 GPIO range using
   the verified J8 header (the old GPIO 32/33/23 were classic-ESP32 pins that do
   not exist on the S3). The 4 UART links use free J8 pins; confirm each against
   the physical board + the V2 (Rev2.0) schematic at bring-up.
2. SX1262 (on the Pico): the Wio-SX1262-for-XIAO board exposes a HOST
   RF_SW1 pin — it needs 8 host signals (SPI 3 + NSS + RESET + BUSY + DIO1 +
   RF_SW1), NOT 7. DIO3/TCXO is module-managed. Verify RF_SW1 on your module rev.
3. Power: nRF24+PA+LNA and the SX1262 (both on the Pico) are TX brown-out
   risks — dedicate decoupling / supply per radio; do not daisy-chain off one
   marginal regulator. The Pico's 3V3 may need help driving PA/LNA spikes.
4. ~~Define the exact UART frame format + checksum + message-type field and the
   record schema `extra` payload per source type.~~ RESOLVED — see the UART
   frame format and data-path sections. Remaining sub-item: finalize the
   per-source `extra` payload contents during firmware bring-up.
5. XIAO-C5 IDF quirk: on IDF v6.0, a secondary-channel offset in promiscuous
   mode dropped HE (11ax) frame reception — status on 6.1 unconfirmed. Test
   5 GHz at 20 MHz / WIFI_SECOND_CHAN_NONE first, verify HE reception, THEN try
   any secondary-channel config.
6. **Power — DECISION: single 18650, cold-swap 4 cells over a DEF CON day**
   (battery 1 → morning break → battery 2 → lunch → battery 3 → afternoon
   break → battery 4; charge overnight). ~4 h sustained per cell at ~550 mA
   system draw. NOT hot-swap — device is shut down / powered off for each swap,
   so no supercap/OR-ing bridge is needed.
   TODO (later, do not spec yet): the cold-swap plan REQUIRES a safe-shutdown
   path so pulling a cell mid-write cannot corrupt the SD. Three layers to
   build when we do the power/firmware work:
     (a) a "prepare to swap / SAFE TO SWAP" action on the touch UI that flushes
         the SD buffer, closes the file, and halts capture before the pull;
     (b) a brown-out-triggered emergency flush+close (bulk cap buys the ms);
     (c) lean on the existing RTC-staging crash-survival logging as the backstop
         so an unexpected pull replays staged records on next boot.
   Also note: each cold swap resets the in-RAM dedup seen-tables (brief
   duplicate records after a swap — cosmetic) and forces a GPS warm re-acquire.

---

## Failure modes & recovery — "honest, not heroic"

Governing principle: this is an ATTENDED device the operator carries and
watches. It DETECTS failures, ANNOUNCES them specifically on screen, and keeps
capturing best-effort — but every actual RECOVERY action is the human's. No
auto-reset hardware, no silent self-healing, no pretending a degraded state is
fine. Detection comes from the heartbeat/ping system (see frame format).

### Partial failure — keep logging (degrade, don't halt)
If a node dies, the hub KEEPS logging whatever is still alive — its own BLE, the
surviving sniffer(s), and the Pico's on-demand radios. A lost node never
halts the whole device.

### No hardware reset line — announce instead (DECISION)
We deliberately spend NO GPIO on hub→node reset lines. On a detected node loss
the hub shows an on-screen alert and recommends a graceful reboot. The human
reboots; the device does not reset nodes itself.

### Buffers are finite — drop-newest + counter (DECISION)
Each ESP32 buffers in its own RAM (tens of KB) and drains best-effort to UART /
SD. Under DEF CON saturation or an SD stall a buffer WILL fill. When full:
**drop the NEWEST record and bump a dropped-record counter** shown on the UI
(mirrors the seen-table eviction policy). Rationale: the device's job is to
keep up as best it can and report honestly; once the adverse situation is
ANNOUNCED, continued data loss is a HUMAN issue to resolve by rebooting /
addressing it — not a hardware failure to engineer around. "Sampling, not
exhausting" already accepts this.

### Hub reboot (brown-out / showstopper) — full restart, hub owns re-sync
- Brown-out: we do not bridge it — the device just restarts. The RTC-staging
  crash-survival logging (from the reboot-logging work) replays staged records
  on boot, so a hub reset costs at most the unflushed buffer, not the session.
- Satellite nodes are FIRE-AND-FORGET: they do NOT track whether the hub is
  alive and keep sending into a dead/unsynced hub with no handshake. All the
  sync intelligence lives in ONE place — the hub.
- When the hub comes back from an unclean restart it recognizes the restart was
  dirty and ANNOUNCES "everything needs a reboot so it's synced again" rather
  than silently resuming a half-synced session with an unaccounted sync gap.
  The human does the clean full restart.

### Operator alert taxonomy (specific, actionable)
Canonical alert strings — the UI backlog (#6) MUST use these exact strings;
keep the two sections in sync.

| Condition | On-screen message | Action asked of operator |
|---|---|---|
| NODE down (satellite C5/S3/Pico; no heartbeat + no PONG) | "A capture node was lost — reboot everything." | full reboot |
| Radio MODULE down (CC1101/nRF24/PN532/SX1262) | "Time to reboot — an actual radio MODULE went down, not an ESP32." | full reboot |
| SD card failed/full | "SD card has an issue — full or toast. Power down and swap it." | power down + swap SD |
| Generic hub showstopper | "Critical error — reboot needed." | full reboot (backed by error log) |
| Hub came up from unclean restart | "Reboot everything to re-sync." | clean full restart of all nodes |
| Buffer saturation | dropped-record counter rising | human's call to reboot/address |

(UI rendering of these is the deferred #6 work; this defines WHAT is shown. The
node-down vs module-down distinction matters: a node is a whole ESP32, a module
is a physical radio hanging off it — the message names which so the operator
knows whether a reboot will help.)

---

## Antenna plan & RF layout (best-effort placement, tune with data)

~7 antennas share a small volume (2.4 GHz ×2: Waveshare BLE + XIAO-S3 Wi-Fi;
5 GHz: XIAO-C5; sub-GHz ×2: CC1101 + SX1262 on the Pico; nRF24 2.4 GHz on
the Pico; GPS on the hub). Same-band pairs will desensitize each other if
crowded. No spectrum analyzer / anechoic chamber — so this is an iterative,
measured trial-and-error task, done at build time with hardware in hand.

### Physical approach
- Separate antennas as much as the case allows; target ≥ a few inches of
  spacing. The roomy cyberdeck case (#10) exists partly to allow this.
- Orient to minimize coupling: **perpendicular orientation** between same-band
  antennas (the two 2.4 GHz especially) reduces how much they couple, because
  antennas have a radiation pattern — spacing AND orientation, not just spacing.
- Best-effort placement first; tune later with data from the mode below.

### "Antenna tuning" firmware mode (diagnostic build)
A special firmware variant (NOT part of capture firmware — a temporary flash on
the Waveshare, an ESP-IDF build like the other ESP32 nodes) that turns the
invisible RF problem into something readable on the LCD. The point is to measure
**desensitization** (is radio A's activity hurting radio B's hearing), not just
raw strength:
- Per-radio noise floor / RSSI with everything else OFF (baseline).
- Per-radio noise floor / RSSI with neighbors ON (real-world case).
- **The DELTA between those two is the number you tune against** — move/reorient
  an antenna, watch the delta shrink. Raw RSSI alone can't tell placement from a
  quiet moment; the A/B delta can.
- TX strength / reflected-power indication where the chip exposes it (nRF24,
  SX1262) — "as much as the radio gives us," often relative not true SWR.
- Live per-radio bar-graph / dB readout on the LCD for a real-time move-and-watch
  loop.
- CAVEAT: ESP32 RSSI / noise-floor numbers are RELATIVE and good for
  comparative tuning ("B improved when I moved A"), not lab-calibrated absolute
  measurements. Fine for optimizing placement; not a spectrum analyzer.

Loop: best-effort placement (spacing + perpendicular orientation) → boot tuning
firmware → read baseline-vs-neighbors-on deltas → move/reorient → repeat →
lock placement. Accepted as best-effort-with-tuning-later.

---

## Build tooling — ESP-IDF for the 3 ESP32s, arduino-pico for the Pico

### DECISION: ESP-IDF 6.1 for the three ESP32 nodes; arduino-pico for the Pico
- The three ESP32 chips (Waveshare S3 hub, XIAO-C5, XIAO-S3) are first-class
  ESP-IDF 6.1 targets. One `idf.py` workflow; per project
  `idf.py set-target {esp32s3|esp32c5}`.
- The Pico is an RP2040 — it builds with the **arduino-pico core** (Earle
  Philhower), NOT ESP-IDF. This is the one non-ESP-IDF node, which is why it
  lives in its own `firmware-pico-rp2040/` tree. It was chosen as the on-demand
  radio host because its 26 GPIO + two SPI buses comfortably hold all four
  radios (compile-verified: 4% flash, 4% RAM, 250 KB RAM free) while freeing the
  pin-starved hub; the dual core and 264 KB RAM are headroom over what the job
  needs.
- Why not Arduino-as-component for the C5: the arduino-esp32 core that supports
  the C5 is pinned to IDF 5.5, so it cannot reach the IDF 6.1 target — hence
  native ESP-IDF for all three ESP32s.

### Shared frame format = a shared component (drift-proof)
Put the wire format in ONE place all nodes include, so there are not four copies
to drift:
- `components/fh_protocol/` holds `fh_frame.h` — the SYNC/TYPE/LEN/CRC frame,
  the capture/health TYPE codes, and the payload structs (CAPTURE, GPS_FIX,
  SUMMARY, HEARTBEAT, PING, PONG, LORA_RELAY, ACK) + the CRC16 function.
  `include/fh_ondemand.h` adds the hub↔Pico on-demand command set (0x20–0x3F).
- The three ESP-IDF firmwares pull it in via `EXTRA_COMPONENT_DIRS`; the
  Pico (arduino-pico) includes the same headers directly (they are plain C,
  framework-agnostic). Change a struct once → every node rebuilds against it →
  drift is structurally impossible.
- `__attribute__((packed))` on payload structs so no compiler padding misaligns
  the specced byte layout (all chips are little-endian — no byte-swap).
- CRC + framing helpers live in the SAME component so sender and receiver run
  IDENTICAL code, not reimplementations that could disagree.
- The frame-format section above IS the contents of `fh_protocol/fh_frame.h`.

---

## Config management — NVS, defaults + override layer, hard to fiddle

> The decided menu (the 6 exposed settings, the constants, and the 4
> maintenance actions) lives in the companion doc `CONFIG_MENU.md`. This section
> covers the storage MECHANISM; `CONFIG_MENU.md` is the authoritative list of
> WHAT is exposed.

### DECISION: config lives in NVS (internal flash), NEVER on the SD
The SD card holds ONLY capture data. Config does NOT live on it, because the
destructive card-format option (used to clear a full card between sessions)
would wipe the settings with it. Config and capture storage are deliberately
DECOUPLED: a card format wipes logs, never settings.
- NVS = ESP-IDF Non-Volatile Storage, a key-value store in a dedicated
  partition of the ESP32's own internal flash — separate from the SD, survives
  cold-swap power cycling (it is flash, not RAM), native to the all-IDF
  decision (#8), standard `nvs_flash` API.

### Defaults + NVS override layer
- Ship compiled-in DEFAULTS (the 6 exposed settings in `CONFIG_MENU.md` — e.g.
  channel dwell time, GPS fix rate, re-log window — plus all the locked
  constants). These are the SUPPORTED BASELINE — the known-good state we design,
  test, and support against.
- First run / blank NVS: run on defaults; the device is NEVER unconfigured even
  with empty NVS.
- On boot: load defaults, then apply any NVS overrides on top (NVS wins).
- "Reset to defaults" = clear the NVS override keys → back to stock. Because it
  is NVS, this never touches the SD / captures.

### Friction by design (support-sanity, not UX laziness)
The config UI is a deliberately LONG, FLAT, unglamorous list — no tuning
dashboard, no invitation to min/max for sport. You must WANT to find a specific
knob. Rationale: keep a known-good baseline so that "it's broken" support calls
have a meaningful answer; if everyone twiddles freely, the baseline is lost.

### Stock-vs-modified tracking (ALSO in NVS, not the SD)
Since NVS is an override layer, the firmware inherently knows stock vs changed.
Expose and RECORD it — and keep this record in NVS too, so a card format cannot
lose the "did someone touch it" history:
- Indicator: **"config: STOCK"** vs **"config: MODIFIED (N overrides)"** on
  screen and in the log-file HEADER / session-start sidecar record.
- The effective config (or just the list of overridden keys) is written at
  session start so support can read what deviated from baseline FROM THE LOG,
  instead of asking the user "did you change anything?" and trusting the answer.
- First support step becomes "reset to defaults" (clears NVS overrides, stock
  again) — captures untouched.

This turns baseline knowledge into something the device RECORDS and PROVES each
session. (The runtime config-editing UI is deferred #6 work; the NVS plumbing +
defaults + stock/modified record are the architecture decision here, independent
of when the UI lands.)

---

## System-level open questions (status tracker)

Beyond the hardware items above, these system concerns are tracked here so the
context survives the (~6-month) timeline to build. Status as of 2026-10-20.

| # | Topic | Status | Notes |
|---|---|---|---|
| 2 | Boot time & cross-chip time sync | **MOSTLY RESOLVED** | DECIDED: battery-backed RTC on the Waveshare owns system time (survives cold swaps) — satellite nodes need no RTC; the hub stamps time on arrival. GPS is hub-local (NEO-6M on a hub UART) and sets the RTC from UTC on first lock. Quiet-vs-crashed SOLVED: always-on HEARTBEAT (0x7E) with liveness counters + active PING/PONG (0x7D/0x7C) tiebreaker → four-state detection (healthy / quiet-but-alive / hung / dead), see frame-format section. Remaining sub-item: timestamp policy during the GPS cold-start window before first fix (hub RTC covers it — confirm the rebase at bring-up). |
| 3 | Failure modes & recovery | **RESOLVED** | "Honest, not heroic" — device detects + announces, human recovers. Keep-logging-on-partial-failure; NO reset line (announce instead); drop-newest + counter on full buffers; brown-out = full restart w/ RTC-staging replay; satellite nodes fire-and-forget, hub owns re-sync and announces "reboot to re-sync" after an unclean restart; full operator alert taxonomy. See the Failure modes section. UI rendering defers to #6. |
| 4 | SD capacity & data management | **DECIDED** | High-quality 16 GB / 32 GB cards. Dump whole file(s) at end of day. Plan: cap recon file size, roll to a new file when the cap is hit, archive the completed one (file rotation, not one giant file). Size cap TBD at bring-up. |
| 5 | Antenna plan & RF layout | **RESOLVED (approach) — tune at build** | DECIDED approach: spacing ≥ a few inches + perpendicular orientation for same-band pairs; best-effort placement then measured tuning. Antenna-tuning firmware mode reads per-radio baseline-vs-neighbors-on RSSI/noise DELTAS on the LCD to quantify desensitization. Relative not calibrated — fine for comparative tuning. Physical tuning still happens with hardware in hand. See Antenna plan section. |
| 6 | Operator UI (3.5" LCD) | **DEFERRED (design) — requirements GATHERED** | Still "future Joshua," ~6 mo; interaction model undecided. But requirements are now consolidated (see UI requirements backlog): green/amber/red status bar (silent-when-healthy), red always pops a message naming module-fault vs node-down, four-state health shown as up/down, terse #3 alert strings reused, SAFE-TO-SWAP control, config list + STOCK/MODIFIED indicator (which settings = TBD), at-a-glance field view. Antenna-tuning firmware explicitly NOT part of this UI. |
| 7 | Legal / operational scope | **DECIDED (firm)** | DEF CON is a declared adverse environment ("protect ya neck" digitally — everyone is warned). Beyond DEF CON, the device is used ONLY where usage is fully disclosed OR there is explicit permission (e.g. own home-lab). This is a hard operating boundary, not a maybe. Capture is passive; any TX is scoped to the user's own sensor-stick link. |
| 8 | Build tooling (4 firmware projects) | **RESOLVED** | 3 ESP32 nodes on ESP-IDF 6.1 (one `idf.py` workflow, per-project `set-target`); the Pico (RP2040) on arduino-pico. Shared frame format in `components/fh_protocol/` (fh_frame.h + fh_ondemand.h), plain C, included by all four → drift structurally impossible. See Build tooling section. |
| 9 | Config management | **RESOLVED** | DECIDED: config in NVS (internal flash), NEVER on SD — a card format wipes logs, never settings. Compiled-in defaults = supported baseline + NVS override layer (NVS wins); blank NVS still runs on defaults; reset-to-defaults clears NVS keys. Config UI is a deliberately long/flat list to discourage casual tweaking. Stock-vs-modified record ALSO kept in NVS + written to the log header so support knows what deviated from baseline without asking. See Config management section. |
| 10 | Enclosure & thermal | **DECIDED (direction)** | NOT a stacked sandwich — a roomy cyberdeck-style case with the boards spread out (helps antennas too, see #5). Possibly simple fans for the continuous-sniff heat + TX radios. Antennas exit the case. |

Priority when we resume: boot/time-sync (#2) and failure modes (#3) are the two
most likely to turn a good design into one that silently stops logging in the
field — revisit those first. The antenna layout (#5) is the hardest physical
problem and is a together-at-build-time task.

---

## Firmware layout (planned)
- `firmware/src/` — Waveshare HUB firmware (`fh_pins.h` lives here): BLE sniff,
  GPS (NEO-6M), microSD recon_log (reuse reboot-logging RTC-staging path), UI,
  4-UART aggregation, sensor-stick orchestration. ESP-IDF 6.1.
- `firmware-c5/`  — XIAO-C5 firmware (own pin map): 5 GHz sniff only. ESP-IDF 6.1.
- `firmware-xs3/` — XIAO-S3 firmware (own pin map): 2.4 GHz sniff only. ESP-IDF 6.1.
- `firmware-pico-rp2040/` — Raspberry Pi Pico (RP2040) on-demand radio node
  (CC1101 + nRF24 + PN532 + SX1262 LoRa), driven by `fh_ondemand.h` commands
  over UART. arduino-pico. Pin map `fh_pico_pins.h`; 4-radio compile test PASSED
  (4% flash, 4% RAM).
All four are separate projects sharing `components/fh_protocol/` headers.
