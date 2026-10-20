<!-- ========================================================================
  ⚠️  SUPERSEDED IN PART — READ THIS FIRST (updated 2026-10-20)
  ========================================================================
  This document describes the ORIGINAL THREE-node design. On 2026-10-20 the
  HARDWARE SPLIT WAS REDESIGNED to FOUR nodes. The system behaviour, wire
  protocol, dedup model, CSV schema, failure taxonomy, DEF CON data-path
  reasoning, config model and UI requirements below are ALL STILL VALID and
  authoritative. BUT every statement about WHICH NODE hosts WHICH radio is now
  OUTDATED. The authoritative current design is docs/FIRMWARE_BRIEF.md +
  docs/OPEN_DECISIONS.md (read the RESOLVED banner at its top).

  WHAT CHANGED (old doc  →  current design):
   • THREE nodes  →  FOUR nodes (added an Adafruit ItsyBitsy M0 Express).
   • Waveshare hub hosted CC1101 / nRF24 / PN532  →  the hub hosts NO wired
     radios. All three moved to the ItsyBitsy.
   • XIAO-S3 = 2.4 GHz + SX1262 LoRa gateway  →  XIAO-S3 = 2.4 GHz Wi-Fi ONLY.
     The SX1262 LoRa moved to the ItsyBitsy (all on-demand radios live there).
   • XIAO-C5 = 5 GHz + GPS  →  XIAO-C5 = 5 GHz Wi-Fi ONLY. GPS moved to the hub.
   • "All THREE firmwares are ESP-IDF"  →  the 3 ESP32 nodes are ESP-IDF 6.1;
     the ItsyBitsy is Arduino/SAMD. FOUR firmwares.
   • SX1262 "7 host pins, ANT_SW on-module via DIO2, no host pin"  →  WRONG for
     the Wio-SX1262-for-XIAO board: it needs a host RF_SW1 pin = 8 host pins.
   • "Verification summary: LoRa on Waveshare? NO" still holds, but LoRa is now
     on the ItsyBitsy, not the XIAO-S3.
   • The camera was dropped (its J8 pins are the hub's 4 UART links: C5, S3,
     ItsyBitsy, GPS).
   • fh_pins.h was fully REWRITTEN (the old GPIO 32/33/23 were classic-ESP32
     pins that do not exist on the S3). See firmware/src/fh_pins.h.

  RESULT — current node roles:
     XIAO-C5    : 5 GHz Wi-Fi (continuous)
     XIAO-S3    : 2.4 GHz Wi-Fi (continuous)
     Hub (WS)   : BLE (continuous) + GPS + microSD + UI + aggregate 4 UART feeds
     ItsyBitsy  : CC1101 + nRF24 + PN532 + SX1262 LoRa (all on-demand)

  The on-demand command set (hub↔ItsyBitsy) is components/fh_protocol/include/
  fh_ondemand.h. Everything below is kept for its still-valid system design.
  ======================================================================== -->

# FHAdvanced — Architecture Notes (locked three-node design)

Experimental branch of FootHoled. Builds on top of the manually-applied
FootHoled-fixes work (reboot fix + reboot-logging).

Status: three-node HARDWARE TOPOLOGY locked + verified against real pinouts
(2026-10-19). System-level design (power, failure modes, config, UI, antenna)
is captured in the status tracker near the end — several items DECIDED but not
yet built, and #6 (UI) deferred. Nothing built or flashed yet. Waveshare pin
assignments live in `firmware/src/fh_pins.h`; this file is the system-level
design, data-flow, and SD-schema contract.

---

## Naming convention (use these names everywhere)

| Name | Board | Role |
|---|---|---|
| **Waveshare** | ESP32-S3-Touch-LCD-3.5 (mainboard) | UI, SD, wired radios, BLE, orchestration |
| **XIAO-C5** | Seeed XIAO ESP32-C5 (daughterboard) | 5 GHz Wi-Fi sniffing + GPS |
| **XIAO-S3** | Seeed XIAO ESP32-S3 (daughterboard) | 2.4 GHz Wi-Fi sniffing + LoRa |

---

## Division of labor (locked)

### Waveshare — mainboard / orchestrator
Handles each of its radio tasks ONE AT A TIME (by design — the use case never
needs two of these Waveshare radios active simultaneously):
- CC1101 (SPI) — sub-GHz raw/OOK-ASK
- nRF24L01+PA+LNA (SPI) — 2.4 GHz transceiver
- PN532 V3 (SPI) — NFC
- **BLE sniffing** — on the Waveshare's own internal 2.4 GHz radio
- **Sensor-stick communication** — dual-transport (see below)
- microSD — the SINGLE log writer for the whole system
- 3.5" LCD + touch UI
- Aggregates both daughterboard UART feeds into one log

### XIAO-C5 — dedicated 5 GHz + geo/time
- **5 GHz Wi-Fi sniffing, continuous** — its sole radio job. Being 5 GHz-only
  sidesteps the C5's single-band-at-a-time limit entirely (it never touches
  2.4 GHz, so no band-hopping compromise).
- **GPS, continuous** — provides position + GPS time; geotags its own 5 GHz
  captures at the source and periodically pushes its fix to the Waveshare so
  every other record can be geotagged too.

**GPS fix rate + geotag strategy (every capture gets a location):**
Two independent mechanisms; use both.
- CACHE LAST-KNOWN FIX (always, free): captures arrive far faster than GPS
  updates (hundreds of frames/sec vs a few fixes/sec), so NEVER gate a capture
  on a fresh fix. Keep the most recent fix in memory and stamp it onto every
  capture as it is recorded. Zero extra GPS/CPU/RF cost. This alone geotags
  every signal.
- TARGET 5 Hz GPS (for the vehicle case): default consumer modules run 1 Hz;
  configure the module to 5 Hz at startup. Rationale — how far you move
  between fixes vs the ~2.5-3 m GPS error floor:
    walking ~1.4 m/s  -> 1 Hz already < GPS error (0.28 m @ 5 Hz)
    running ~5 m/s     -> ~1 m per fix @ 5 Hz
    30 mph  ~13 m/s    -> 2.6 m per fix @ 5 Hz (vs 13 m @ 1 Hz)
    65 mph  ~30 m/s    -> 6 m per fix @ 5 Hz
  Walking/running are already covered at 1 Hz (movement < GPS error); 5 Hz is
  really about keeping the DRIVING case under the GPS error floor. 5 Hz is the
  sweet spot over 10 Hz: adequate for vehicle speed without the UART/parse
  overhead.
- REQUIRED CONFIG: 5 Hz NMEA will NOT fit 9600 baud — raise the GPS UART to
  38400 or 115200 baud. Parsing 5 fixes/sec is still trivial for the C5 and
  does not affect 5 GHz sniffing.
- HONEST LIMIT: the geotag is "position at the most recent fix," not at the
  exact instant of capture. Worst-case staleness at 5 Hz is 0.2 s
  (sub-meter walking, ~1 m running, ~2.6 m @ 30 mph) — at or below the GPS
  hardware error floor, so chasing tighter is not meaningful.

**GPS does NOT interfere with 5 GHz Wi-Fi sniffing** — the C5 is the least
stressed board in the system:
- RF: GPS is RECEIVE-ONLY at ~1.575 GHz (L1); it never transmits, so there is
  nothing to collide with the 5 GHz sniffer. No shared airtime, no TX bursts.
- CPU: GPS is a trickle — NMEA over UART at ~9600 baud, ~1 sentence/second.
  Parsing costs almost nothing beside the Wi-Fi capture interrupt load. No
  tight polling (unlike the SX1262 BUSY line); at most an optional 1 Hz PPS.
- Scheduling: a once-a-second bounded parse cannot stall a channel-hop timer
  the way a long LoRa TX could. No jitter concern.
- BUILD NOTE (reverse concern): GPS signals sit below the noise floor, so the
  risk runs the OTHER way — board switching noise / RF can desensitize the GPS
  receiver. Give the GPS antenna a clear sky view and isolation from noisy
  supplies and digital lines. Relevant for the enclosed 3D-printed case: GPS
  behind plain plastic is fine; GPS crammed next to regulators/RF shielding is
  not. This affects GPS fix quality, never the Wi-Fi capture.

### XIAO-S3 — 2.4 GHz sniffing + LoRa gateway
- **2.4 GHz Wi-Fi sniffing — THIS IS THE PRIORITY TASK.** The XIAO-S3's primary
  job is continuous 2.4 GHz capture; it must not drop frames for LoRa.
- **Wio-SX1262 LoRa (SPI) — LESSER, BEST-EFFORT priority.** LoRa is occasional
  (only when relaying sensor-stick traffic at distance). It must yield to the
  2.4 GHz sniffer. See the firmware-priority contract below.

---

## Band coverage (why this split is clean)

Wi-Fi capture is split across two bands on two separate chips:
- 2.4 GHz Wi-Fi → XIAO-S3 (internal radio)
- 5 GHz Wi-Fi  → XIAO-C5 (dedicated)
- BLE (2.4 GHz) → Waveshare (internal radio)

Three different chips, so 2.4 Wi-Fi, 5 GHz Wi-Fi, and BLE run in genuine
parallel with no RF contention. Sub-GHz (CC1101) and NFC (PN532) are wired on
the Waveshare. LoRa (sub-GHz) is on the XIAO-S3's external SX1262.

---

## XIAO-S3 firmware priority contract (2.4 GHz > LoRa)

LoRa and 2.4 GHz Wi-Fi do NOT contend at RF level — different radios, different
bands (SX1262 sub-GHz vs internal 2.4 GHz), different antennas. They CAN
contend for CPU. Rules to keep sniffing first-class:

1. **Dual-core split:** pin the 2.4 GHz Wi-Fi promiscuous capture + its
   callback to one core; run SX1262 SPI/DIO1/BUSY handling on the other.
2. **2.4 GHz sniffing is the priority task.** LoRa TX/RX is best-effort and
   must never starve the Wi-Fi capture callback or stall the channel-hop timer.
3. **Protect the channel-hop timer:** a long LoRa op (high spreading factor =
   hundreds of ms to >1 s time-on-air) must not delay channel hops, or the
   sniffer dwells too long on one channel and misses others.
4. LoRa is OCCASIONAL (bursty sensor-stick comms), not continuous — this is
   close to a best case; the rest of the time the XIAO-S3 is 100% on Wi-Fi.
5. During a LoRa transmit/receive window the Wi-Fi radio keeps listening on its
   current channel; only the hop cadence is at risk, never the capture itself.

---

## Sensor-stick link — dual transport (BLE + LoRa)

Mirrors the sensor-node project's dual-transport handoff: use **BLE when in
range**, **LoRa when at distance**. Transport selection logic lives on the
**Waveshare** (the orchestrator).

- **BLE path:** Waveshare talks to the sensor stick directly on its own radio.
- **LoRa path:** the Waveshare has NO LoRa radio. The only SX1262 is on the
  XIAO-S3, so the XIAO-S3 is the **system LoRa gateway**. The Waveshare sends
  "relay this to the sensor stick over LoRa" across the UART link; the XIAO-S3
  transmits on the SX1262 and relays replies back. This is why the
  Waveshare↔XIAO-S3 UART is BIDIRECTIONAL and carries two traffic types:
  (a) XIAO-S3 capture records going up, (b) sensor-stick LoRa payloads both ways.

---

## Topology

```
   GPS (UART) ─┐
               ▼
   ┌────────────────────────────┐
   │ XIAO-C5                    │  UART link B   ┌───────────────────────────────┐
   │  5 GHz Wi-Fi sniff (cont.) │───────────────►│ Waveshare ESP32-S3 (mainboard)│
   │  GPS geotag + GPS time     │  GPIO32/33     │  CC1101 ┐                     │
   └────────────────────────────┘                │  nRF24  ├ SPI radios + NFC    │
                                                  │  PN532  ┘  (one at a time)    │
   ┌────────────────────────────┐  UART link A   │  BLE sniff (internal radio)   │
   │ XIAO-S3                    │◄──────────────►│  sensor-stick orchestration   │
   │  2.4 GHz Wi-Fi sniff (PRI) │  TXD/RXD 25/27 │  3.5" LCD + touch UI          │
   │  SX1262 LoRa (best-effort) │  bidirectional │  2× UART aggregator           │
   │  = system LoRa gateway     │                │         ▼                     │
   └────────────────────────────┘                │  recon_log → microSD (1 CSV)  │
                                                  └───────────────────────────────┘
```

---

## Interconnect — two UART links into the Waveshare (verified)

| Link | Daughterboard | Waveshare pins | Controller | Direction |
|---|---|---|---|---|
| A | XIAO-S3 (2.4 GHz + LoRa) | TXD/RXD = hdr 25/27 (GPIO43/44) | hardware UART | bidirectional (records up + LoRa relay both ways) |
| B | XIAO-C5 (5 GHz + GPS) | GPIO33/32 = hdr 12/14 | 2nd UART, remapped | mainly C5→Waveshare (records + GPS fixes) |

Console stays on USB-CDC so it does not fight link A. Frames: length-prefixed
with a message-type field (capture-record vs LoRa-relay vs GPS-fix) + checksum
(UART can drop bytes). Full frame spec below.

**Link baud (DEF CON-class load):** run both capture links at **2 Mbaud**
(2000000). Rationale in the data-path section below — 115200 drowns instantly,
921600 is only a floor in a hostile RF environment, 2 Mbaud carries a
dedup'd dual-band load with burst headroom over the short board-to-board
traces. (This is the inter-board link baud; the GPS→C5 UART is a separate,
slower link at 38400–115200 as noted in the XIAO-C5 section.)

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

## UART frame format (binary, framed, checked) — link A & B

All inter-board UART traffic uses this frame. It is self-synchronizing (a
dropped byte resyncs on the next SYNC marker rather than poisoning the stream)
and type-tagged so one bidirectional link carries capture records, GPS fixes,
and LoRa-relay payloads.

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
- **CRC16/CCITT** (poly 0x1021, init 0xFFFF) — cheap on all three chips, catches
  the single/double-bit and burst errors UART actually produces.
- **Endianness:** little-endian for all multi-byte fields (native to all three
  ESP32 chips — no byte-swapping).

### TYPE codes

| TYPE | Name          | Direction        | Payload |
|------|---------------|------------------|---------|
| 0x01 | CAPTURE       | daughterboard→WS | capture record (below) |
| 0x02 | GPS_FIX       | C5→WS            | position push for last-known cache |
| 0x03 | SUMMARY       | daughterboard→WS | periodic per-device count summary |
| 0x10 | LORA_RELAY_TX | WS→XIAO-S3       | payload to transmit to sensor stick |
| 0x11 | LORA_RELAY_RX | XIAO-S3→WS       | payload received from sensor stick |
| 0x7E | HEARTBEAT     | daughterboard→WS | liveness counters (below) — always-on, fixed cadence |
| 0x7D | PING          | WS→daughterboard | "ya dead, man?" — active health check on a missed beat |
| 0x7C | PONG          | daughterboard→WS | reply to PING (echoes the PING seq) |
| 0x7F | ACK           | both ways        | ack a LORA_RELAY (seq echoed) |

### PAYLOAD — CAPTURE (TYPE 0x01), ~34 B

```
Offset  Size  Field        Notes
------  ----  -----------  ----------------------------------------------------
  0      1    src_id       enum: 1=c5-wifi5 2=xs3-wifi24 (maps to SD `source`)
  1      1    band_chan    high nibble = band (2.4/5), low = channel index
  2      1    frame_type   mgmt/data/ctrl + subtype (probe-req, beacon, …)
  3      1    rssi         int8, dBm
  4      6    mac          6-byte device MAC
 10      4    gps_time     u32 epoch seconds (C5 fills real GPS time; else 0 =
                           "WS stamps on arrival with RTC")
 14      4    lat          int32, degrees × 1e7 (0 if no fix)
 18      4    lon          int32, degrees × 1e7 (0 if no fix)
 22      2    count        u16 occurrences since last summary (dedup count)
 24     10    extra        type-specific (e.g. SSID hash, OUI flags) — var,
                           bounded; LEN covers the real length
```

### PAYLOAD — GPS_FIX (TYPE 0x02), 16 B

```
  0   4  gps_time   u32 epoch seconds
  4   4  lat        int32, deg × 1e7
  8   4  lon        int32, deg × 1e7
 12   1  sats       u8 satellites in fix
 13   1  fix_qual   u8 fix quality / HDOP bucket
 14   2  reserved
```
The Waveshare caches the latest GPS_FIX as "last-known position" and stamps it
onto ws-* and xs3-wifi24 records (which have no local GPS), geotagging the
whole log.

### PAYLOAD — LORA_RELAY (TYPE 0x10 / 0x11)

```
  0   1  seq        u8 sequence number (echoed in ACK 0x7F)
  1   1  flags      transport/priority flags
  2   N  data       opaque sensor-stick payload, N = LEN-2
```
LoRa relay is best-effort and MUST yield to 2.4 GHz sniffing on the XIAO-S3
(per the priority contract). ACK (0x7F) echoes `seq` for reliable relay.

### PAYLOAD — HEARTBEAT (TYPE 0x7E), 14 B — liveness, not just presence

Sent by EACH daughterboard on a fixed cadence (every 1–2 s) whether or not it
captured anything. A bare "I'm alive" is not enough — a board can be
alive-but-broken (main loop fine, capture task hung), so the heartbeat carries
proof-of-work counters:

```
Offset  Size  Field          Notes
------  ----  -------------  --------------------------------------------------
  0      1    src_id         which daughterboard (1=c5, 2=xs3)
  1      4    uptime_s       u32 seconds since boot — RESET TO 0 = silent reboot
  5      4    capture_count  u32 records captured since boot — must keep rising
  9      2    dropped_keys   u16 seen-table evictions (saturation indicator)
 11      1    task_alive     bitmask: capture / GPS / LoRa task liveness bits
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

This largely closes open hardware item #4 — only the per-source `extra` payload
contents remain, to be finalized during firmware bring-up.

---

## SD logging — one writer, one schema

The microSD has EXACTLY ONE writer — the Waveshare. Daughterboards never touch
SD; they stream. The Waveshare parses both UART links and hands every record to
the SAME `recon_log` path used for its local radios, reusing the reboot-logging
RTC-staging + batched-flush work unchanged.

Unified record (one CSV, `source`-tagged):

```
source, band, channel, type, mac, rssi, lat, lon, time, extra
```

| source | origin | geotag | clock |
|---|---|---|---|
| ws-cc1101 | Waveshare SPI | last-known (from C5) | Waveshare RTC |
| ws-nrf24  | Waveshare SPI | last-known | Waveshare RTC |
| ws-nfc    | Waveshare SPI (PN532) | last-known | Waveshare RTC |
| ws-ble    | Waveshare internal radio | last-known | Waveshare RTC |
| c5-wifi5  | XIAO-C5 over link B | GPS (exact) | GPS time |
| xs3-wifi24| XIAO-S3 over link A | last-known | Waveshare RTC |

(LoRa relay traffic is comms, not capture — it is not logged as a capture row
unless we choose to log a sensor-stick event.) The XIAO-C5 periodically pushes
its GPS fix to the Waveshare, which caches "last known position" to stamp onto
non-GPS records so the whole log is geotagged.

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
- **An ESP32 NODE is down** (a whole daughterboard hung or dead): the #2
  up/down failure → "A capture node was lost — reboot everything."
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
Per-band capture counts (2.4 / 5 / BLE), GPS-fix status, storage remaining,
both link-health indicators — the things you want while walking a floor.

### NOT part of this UI — antenna tuning firmware (#5)
The antenna-tuning mode is a SEPARATE, standalone firmware flashed onto the
Waveshare purely for RF bring-up ("so we know how to build future units"). It
is a bench/factory tool, NOT wired into the main capture firmware's config or
alerts, and never ships in a field unit. Lives in the repo as its own IDF
project alongside the three node firmwares; version-controlled, not shipped.

---

## Verification summary (2026-10-19)

| Item | Verdict | Evidence |
|---|---|---|
| Waveshare UART aggregation | PASS | header: TXD/RXD + GPIO32/33 confirmed broken out |
| Waveshare 5 GHz capability | N/A (expected) | Waveshare docs: 2.4 GHz Wi-Fi only → XIAO-C5 required |
| XIAO-C5 5 GHz promiscuous | PASS | ESP-IDF v6.1 C5 `esp_wifi_set_promiscuous()`; esp32-mac-collector |
| XIAO-C5 GPS + Waveshare UART | PASS w/ note | 1 HW UART on header; GPS + link need a 2nd UART remapped to free GPIO |
| XIAO-S3 for SX1262 + 2.4 Wi-Fi + UART | PASS (comfortable) | ANT_SW handled on-module via SX1262 DIO2 — host needs only 7 pins; 9/11 GPIO used, ~2 spare |
| LoRa on Waveshare instead? | NO | Waveshare short 1-2 fast native GPIO (BUSY+DIO1); LoRa stays on XIAO-S3 |

---

## Open hardware items (none block the design)
1. ~~Waveshare GPIO26 (header pin 10): may be tied to R8 PSRAM — verify vs
   schematic before use.~~ RESOLVED by DECISION (2026-10-19): treat GPIO26 as
   RESERVED / DO-NOT-USE. Nothing in the design needs it (the UART links are on
   GPIO43/44 and GPIO32/33, both confirmed clean), so we simply never touch it
   and the PSRAM question never has to be answered. Removes the only
   hardware-in-hand check that was not strictly necessary.
2. ~~XIAO-S3: confirm whether the Wio-SX1262 board handles ANT_SW internally
   (frees a pin if so).~~ RESOLVED (2026-10-19, Wio-SX1262 datasheet + Seeed
   wiki): it is a "LoRa Pure RF module" — the SX1262 drives its own antenna
   switch via DIO2 (`SetDIO2AsRfSwitchCtrl`) and the TCXO via DIO3, both
   on-module. NO host GPIO for ANT_SW. Host drives only SPI (SCK/MOSI/MISO) +
   NSS + RESET + BUSY + DIO1 = 7 pins. With the UART link (2) that is 9 of the
   XIAO-S3's 11 GPIO → ~2 SPARE. The tightest board has breathing room, not
   zero slack.
3. Power: nRF24+PA+LNA (Waveshare) and SX1262 (XIAO-S3) are TX brown-out risks
   — dedicate decoupling / supply per radio; do not daisy-chain off one
   marginal regulator.
4. ~~Define the exact UART frame format + checksum + message-type field and the
   record schema `extra` payload per source type.~~ RESOLVED — see the UART
   frame format and data-path sections. Remaining sub-item: finalize the
   per-source `extra` payload contents during firmware bring-up.
5. XIAO-C5 IDF quirk: on IDF v6.0, a secondary-channel offset in promiscuous
   mode dropped HE (11ax) frame reception — watch during 5 GHz bring-up.
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
If a daughterboard dies, the hub KEEPS logging its own radios
(CC1101/nRF24/PN532/BLE) and the surviving daughterboard. A lost capture node
never halts the whole device.

### No hardware reset line — announce instead (DECISION)
We deliberately spend NO GPIO on hub→daughterboard reset lines. On a detected
daughterboard loss the hub shows an on-screen alert and recommends a graceful
reboot. The human reboots; the device does not reset nodes itself.

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
- Daughterboards are FIRE-AND-FORGET: they do NOT track whether the hub is
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
| ESP32 NODE down (daughterboard; no heartbeat + no PONG) | "A capture node was lost — reboot everything." | full reboot |
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
5 GHz: XIAO-C5; sub-GHz ×2: CC1101 + SX1262; nRF24 2.4 GHz; GPS). Same-band
pairs will desensitize each other if crowded. No spectrum analyzer / anechoic
chamber — so this is an iterative, measured trial-and-error task, done at build
time with hardware in hand.

### Physical approach
- Separate antennas as much as the case allows; target ≥ a few inches of
  spacing. The roomy cyberdeck case (#10) exists partly to allow this.
- Orient to minimize coupling: **perpendicular orientation** between same-band
  antennas (the two 2.4 GHz especially) reduces how much they couple, because
  antennas have a radiation pattern — spacing AND orientation, not just spacing.
- Best-effort placement first; tune later with data from the mode below.

### "Antenna tuning" firmware mode (diagnostic build)
A special firmware variant (NOT part of capture firmware — a temporary flash,
fits the all-IDF multi-build story in #8) that turns the invisible RF problem
into something readable on the LCD. The point is to measure **desensitization**
(is radio A's activity hurting radio B's hearing), not just raw strength:
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

## Build tooling — all ESP-IDF, one shared protocol component

### DECISION: target ESP-IDF for ALL THREE node firmwares (no Arduino)
- All three chips (ESP32-S3, C5, S3) are first-class ESP-IDF targets; the C5
  needs recent IDF (v6.x) for 5 GHz promiscuous anyway, so we are on modern IDF
  regardless.
- One toolchain, one build system (CMake / `idf.py`), one flash pattern.
  Per project: `idf.py set-target {esp32s3|esp32c5}` then identical workflow.
  The #8 "build story" is "same commands, different target."
- Keeps the existing C/C++ reboot-logging work native — no Arduino-compat shim.

### Shared frame format = a shared IDF COMPONENT (drift-proof)
Same framework ALLOWS a single shared header; sharing it is still a deliberate
choice. Put the wire format in ONE component all three projects include, so
there are not three copies to drift:
- `components/fh_protocol/` holds `fh_frame.h` — the SYNC/TYPE/LEN/CRC frame,
  all TYPE codes, and the payload structs (CAPTURE, GPS_FIX, SUMMARY,
  HEARTBEAT, PING, PONG, LORA_RELAY, ACK) + the CRC16 function.
- All three firmwares pull it in (git submodule or `EXTRA_COMPONENT_DIRS`).
  Change the struct once → all three rebuild against it → drift is structurally
  impossible.
- `__attribute__((packed))` on payload structs so no compiler padding misaligns
  the specced byte layout (all three are little-endian ESP32 — no byte-swap).
- CRC + framing helpers live in the SAME component so sender and receiver run
  IDENTICAL code, not two reimplementations that could disagree.
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
context survives the (~6-month) timeline to build. Status as of 2026-10-19.

| # | Topic | Status | Notes |
|---|---|---|---|
| 2 | Boot time & cross-chip time sync | **MOSTLY RESOLVED** | DECIDED: battery-backed RTC on the Waveshare owns system time (survives cold swaps) — daughterboards need no RTC; hub rebases/stamps. Quiet-vs-crashed SOLVED: always-on HEARTBEAT (0x7E) with liveness counters + active PING/PONG (0x7D/0x7C) tiebreaker → four-state detection (healthy / quiet-but-alive / hung / dead), see frame-format section. Remaining sub-item: timestamp policy during the GPS cold-start window before first fix (hub RTC covers it — just confirm the rebase at bring-up). |
| 3 | Failure modes & recovery | **RESOLVED** | "Honest, not heroic" — device detects + announces, human recovers. Keep-logging-on-partial-failure; NO reset line (announce instead); drop-newest + counter on full buffers; brown-out = full restart w/ RTC-staging replay; daughterboards fire-and-forget, hub owns re-sync and announces "reboot to re-sync" after an unclean restart; full operator alert taxonomy. See the Failure modes section. UI rendering defers to #6. |
| 4 | SD capacity & data management | **DECIDED** | High-quality 16 GB / 32 GB cards. Dump whole file(s) at end of day. Plan: cap recon file size, roll to a new file when the cap is hit, archive the completed one (file rotation, not one giant file). Size cap TBD at bring-up. |
| 5 | Antenna plan & RF layout | **RESOLVED (approach) — tune at build** | DECIDED approach: spacing ≥ a few inches + perpendicular orientation for same-band pairs; best-effort placement then measured tuning. Antenna-tuning firmware mode reads per-radio baseline-vs-neighbors-on RSSI/noise DELTAS on the LCD to quantify desensitization. Relative not calibrated — fine for comparative tuning. Physical tuning still happens with hardware in hand. See Antenna plan section. |
| 6 | Operator UI (3.5" LCD) | **DEFERRED (design) — requirements GATHERED** | Still "future Joshua," ~6 mo; interaction model undecided. But requirements are now consolidated (see UI requirements backlog): green/amber/red status bar (silent-when-healthy), red always pops a message naming module-fault vs node-down, four-state health shown as up/down, terse #3 alert strings reused, SAFE-TO-SWAP control, config list + STOCK/MODIFIED indicator (which settings = TBD), at-a-glance field view. Antenna-tuning firmware explicitly NOT part of this UI. |
| 7 | Legal / operational scope | **DECIDED (firm)** | DEF CON is a declared adverse environment ("protect ya neck" digitally — everyone is warned). Beyond DEF CON, the device is used ONLY where usage is fully disclosed OR there is explicit permission (e.g. own home-lab). This is a hard operating boundary, not a maybe. Capture is passive; any TX is scoped to the user's own sensor-stick link. |
| 8 | Build tooling (3 firmware projects) | **RESOLVED** | DECIDED: all-ESP-IDF (no Arduino) across all three nodes; one `idf.py` workflow, per-project `set-target`. Shared frame format lives in ONE IDF component `components/fh_protocol/` (packed structs + shared CRC) included by all three → drift structurally impossible. See Build tooling section. |
| 9 | Config management | **RESOLVED** | DECIDED: config in NVS (internal flash), NEVER on SD — a card format wipes logs, never settings. Compiled-in defaults = supported baseline + NVS override layer (NVS wins); blank NVS still runs on defaults; reset-to-defaults clears NVS keys. Config UI is a deliberately long/flat list to discourage casual tweaking. Stock-vs-modified record ALSO kept in NVS + written to the log header so support knows what deviated from baseline without asking. See Config management section. |
| 10 | Enclosure & thermal | **DECIDED (direction)** | NOT a stacked sandwich — a roomy cyberdeck-style case with the boards spread out (helps antennas too, see #5). Possibly simple fans for the continuous-sniff heat + TX radios. Antennas exit the case. |

Priority when we resume: boot/time-sync (#2) and failure modes (#3) are the two
most likely to turn a good design into one that silently stops logging in the
field — revisit those first. The antenna layout (#5) is the hardest physical
problem and is a together-at-build-time task.

---

## Firmware layout (planned)
- `firmware/src/` — Waveshare firmware (`fh_pins.h` lives here), builds on the
  reboot-logging recon_log path.
- `firmware-c5/`  — XIAO-C5 firmware (own pin map): 5 GHz sniff + GPS.
- `firmware-xs3/` — XIAO-S3 firmware (own pin map): 2.4 GHz sniff (priority) +
  SX1262 LoRa gateway (best-effort).
The daughterboard firmwares are separate projects, not part of the Waveshare
build. To be added when we start them.
