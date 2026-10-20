# FHAdvanced — Config Menu (decided)

Companion to `ARCHITECTURE.md` (#6 UI + #9 config). This WAS a candidate list to
prune; it is now the DECIDED config spec. The final menu and constants are in
the "PRUNED RESULT" section below; the original keep/maybe/probably-not tier
tables are retained at the end as a historical reference only. Captured /
decided 2026-10-19.

Guiding rule that drove the pruning: a setting earns a menu slot only if (a) a
plausible real scenario needs it changed in the field or between sessions, AND
(b) the default cannot simply be "right." Everything else is a COMPILE-TIME
CONSTANT. This also serves the #9 "friction by design" intent — a lean menu,
not a knob for everything.

---

## PRUNED RESULT (2026-10-19) — final MENU vs CONSTANT

Pruning pass complete. Two buckets now: **MENU** (exposed, NVS-backed override)
and **CONSTANT** (compiled-in, not user-reachable).

### MENU — exposed settings (6)
1. **Channel dwell time** — how long each sniffer sits on a channel before
   moving to the next. Default **~110 ms** (just over the ~102 ms Wi-Fi beacon
   interval, so every AP's beacon is caught each pass). Longer = more per
   channel; shorter = faster full-band cycle.
2. **Per-band enable/disable (2.4 / 5 / BLE)** — default ALL ON; also a power
   lever.
3. **BLE scan mode** — default **PASSIVE**; **active is OPT-IN** (user turns it
   on, liability theirs). Active only probes devices already advertising to be
   probed. Show an in-UI note on enable: "Active scan transmits probe requests."
4. **Seen-table re-log policy** — radio choice:
   (a) "Re-log after [N] seconds" (numeric fill-in, DEFAULT **60 s**) — a device
       re-logs as a fresh sighting once quiet / lingering past N seconds; or
   (b) "Per session (log each device once)" — one row per device for the whole
       session, never re-logs until reboot (infinite eviction). Default = (a)/60s.
5. **GPS fix rate (1 / 5 / 10 Hz)** — default 5 Hz.
6. **Display** — brightness + sleep timeout:
   - Brightness: user scale 0–100% mapped to a HARDWARE CEILING of 85% (we never
     drive the backlight past 85%; the user still sees "100%"). Default **50%**.
     Protects backlight LED longevity, lowers heat, saves power (the 3.5" is an
     LCD/IPS, so this is backlight life + power, not literal OLED burn-in).
   - Sleep timeout: seconds, default **60 s**. DISPLAY-ONLY sleep — capture keeps
     running when the screen blanks; tap/button to wake.

Plus the 4 DECIDED maintenance actions (Reset to Defaults, Wipe SD,
Safe Battery Swap, Config status view) — see below.

### Dedup model (DECIDED 2026-10-19) — identity-based, "seen is seen"
- **RSSI is NOT a dedup trigger.** Identity is what matters, not signal strength.
  Once a device (by MAC / SSID / identity) is logged, hearing it again at a
  different RSSI is NOT a new row. RSSI is RECORDED on the sighting row as a
  field, but never re-triggers logging. The old ±6 dB RSSI-change threshold
  setting is REMOVED entirely.
- A device gets ONE row per its re-log window (setting #4). This builds a
  PRESENCE dataset ("what was here"), not a movement-tracking dataset.
- The periodic count-summary still ticks how many times a device was seen, so
  "it was here a lot" is preserved in the count, not in duplicate rows.
- Per-session mode (#4b) holds every unique device for the whole session (no
  eviction); if the seen-table hits its max size it falls back to the existing
  evict-oldest + dropped-key counter. No new failure mode.

### Removed from the menu (now constants / auto / dropped)
- **Wi-Fi hop lists (2.4 & 5 GHz)** → always FULL SWEEP (see Channel coverage).
- **RSSI-change threshold** → dropped (dedup is identity-based, above).
- **Recon file roll threshold** → FIXED 128 MB (see Storage).
- **Device name / operator tag** → DROPPED. The eFuse MAC in the log header is
  the sole device identity ("which unit"); we do not care who held it, so no
  operator tag.
- **Set clock / timezone** → DROPPED as a setting. The RTC is set AUTOMATICALLY
  from GPS UTC on first lock; the battery-backed RTC covers the pre-lock window.
  All time is UTC. No user action.

### Channel coverage (DECIDED 2026-10-19)
- **Always FULL SWEEP — no user-selectable hop list.** A single Wi-Fi radio can
  only listen on one channel at a time, so "sweep all channels" IS channel
  hopping — the radio visits each channel in turn. We always cover the full
  channel set (2.4 GHz and 5 GHz); the user does NOT pick a subset. The two old
  "hop list" menu items are REMOVED → they are now a constant (full sweep).
- The one exposed knob for coverage is **channel dwell time** (above), tied to
  the ~102 ms beacon interval; 110 ms default reliably catches every AP's
  beacon per pass.

### Storage / logging format (DECIDED 2026-10-19)
- **Recon file roll threshold → 128 MB, FIXED (not configurable).** Close the
  current file and start a fresh one at 128 MB. Under the FAT32 4 GB ceiling
  with huge margin; big enough not to roll often, small enough a corrupt file
  is not catastrophic. A CONSTANT, not a menu knob.
- **Log files are CSV on the SD card** (NOT binary). Binary stays on the WIRE
  only (daughterboard→hub); the hub expands to CSV on arrival and writes CSV.
  Rationale: pull the card and open it anywhere forever — no decoder tool, no
  "what format was this." Accepted cost: CSV is ~2× the bytes of binary, so the
  card fills ~2× faster and at peak DEF CON saturation the SD buffer fills
  sooner → hits the already-designed drop-newest-and-count behavior (graceful,
  not a crash). The CPU to format CSV is cheap (`snprintf`, microseconds) and
  lands on the HUB, which does no Wi-Fi capture — so it does not steal from the
  sniffers. Worth it for never needing a decoder.
- **CSV header line per file** (cheap, first line / comment): format version,
  originating node eFuse MAC, session start (UTC), config STOCK/MODIFIED. Gives
  provenance with zero binary format.
- **Logging split — recon stream vs function/event radios:**
  - **Recon STREAM (Wi-Fi 2.4, Wi-Fi 5, BLE)** — continuous passive listeners;
    share the unified CSV schema (`source, band, channel, type, mac, rssi, lat,
    lon, time, extra`).
  - **Function/EVENT radios (CC1101, nRF24, PN532/NFC)** — NOT continuous; each
    is a deliberate action (sub-GHz burst, 2.4 GHz probe, NFC read) whose RESULT
    is recorded as an event. Their meaningful fields differ per function (an NFC
    read has tag UID/type; a sub-GHz burst has freq + payload snippet) and do
    not fit the Wi-Fi schema cleanly. NOT user-configurable — per-function
    record formats are TBD, to define WHEN those features are built. Same CSV
    file is fine; the row shape differs by function.

### Hardware provenance + time base (DECIDED 2026-10-19)
- **eFuse MAC = hardware provenance.** Every ESP32 (S3 / C5) has a factory-burned,
  globally-unique 48-bit MAC in eFuse (`esp_efuse_mac_get_default()` /
  `esp_read_mac()`) — permanent, unforgeable, zero-config. Each of the three
  nodes has its own. Write the originating node's eFuse MAC into the LOG HEADER
  at session start (NOT every row), so a file is provably tied to specific
  silicon ("this data came from THIS Waveshare / node"). It is the SOLE device
  identity — the human operator tag was DROPPED (we do not care who held the
  unit). KEEP the device's own eFuse MAC in header/metadata only, never mixed
  into capture rows, so "my hardware" is never confused with "a sniffed device."
- **RTC set automatically from GPS on first lock.** Battery-backed RTC (held in
  UTC) covers the pre-lock / no-sky window; GPS corrects it on first fix. Not a
  user setting.
- **All logging in UTC, period.** GPS time is natively UTC; the Waveshare RTC is
  kept in UTC. No on-device timezone/DST handling — whoever analyzes the files
  converts to local if they care. Keeps multi-day / multi-location captures
  coherent and cross-node correlation trivial. (This is why timezone is a locked
  CONSTANT, not a menu item.)

### CONSTANT — compiled-in, deliberately NOT exposed
- Channel coverage → ALWAYS FULL SWEEP (no user-selectable hop list; see
  Channel coverage section). Dwell time is the one exposed coverage knob.
- Periodic summary interval (5–10 s default)
- Recon file roll threshold → FIXED 128 MB (see Storage section)
- Log format → CSV-on-SD, binary on the wire only, expand on the hub (see
  Storage section) — one format, no toggle
- Per-source logging → fixed schemas: unified CSV for the recon stream
  (Wi-Fi/BLE); function/event record formats (CC1101/nRF24/PN532) TBD at build,
  not user-configurable
- "Down" declaration timeout (good default; wrong value misses failures or
  false-alarms)
- Seen-table max size (RAM-bounded)
- Geotag staleness limit (last-known-fix model handles it)
- Heartbeat interval (1–2 s)
- **UART baud → LOCKED 2 Mbaud** — brick-the-link risk; never user-reachable

Rationale summary: **6 settings + 4 actions** is tight and defensible, every one
with a real use case. Everything else is constant — which also serves
friction-by-design and brick-proofs the link + the legal/TX posture.

---

## Original candidate tiers (pre-prune, kept for reference)

Tier key: KEEP / MAYBE / PROBABLY-NOT.

---

## Capture / radio behavior
| Setting | Tier | Why |
|---|---|---|
| Wi-Fi channel hop list — 2.4 GHz | KEEP | "1/6/11 only" vs all, region-specific |
| Wi-Fi channel hop list — 5 GHz | KEEP | many channels; include/exclude DFS channels (DFS = the fake-radar-strike problem) |
| Channel dwell time | KEEP | longer = more per channel, shorter = more coverage; tune per environment |
| Per-band enable/disable (2.4 / 5 / BLE) | MAYBE | focused captures; or just leave all on |
| BLE scan: active vs passive, window/interval | MAYBE | passive is the safe default; active transmits — may be a LOCKED default given the passive-leaning legal stance |

## Dedup / data volume (the DEF CON knobs)
| Setting | Tier | Why |
|---|---|---|
| RSSI-change threshold (±6 dB default) | KEEP | directly controls record volume; tune when the card fills too fast/slow |
| Seen-table window (eviction timeout) | KEEP | how long a quiet device is remembered; dedup aggressiveness |
| Periodic summary interval (5–10 s) | MAYBE | fine default, rarely touched |
| Seen-table max size | PROBABLY-NOT | RAM-bounded; compile-time constant |

## GPS
| Setting | Tier | Why |
|---|---|---|
| GPS fix rate (1 / 5 / 10 Hz) | KEEP | walking vs driving is a real reason; higher costs UART/parse |
| Geotag staleness limit | PROBABLY-NOT | last-known-fix model handles it; compile-time |

## Storage / logging
| Setting | Tier | Why |
|---|---|---|
| Recon file size cap (roll threshold) | KEEP | when to close a file and start a new one (named as a real decision) |
| Log format: binary vs CSV-on-device | MAYBE | binary-on-wire is fixed; SD-stores-raw vs expanded could be a toggle or just fixed |
| What gets logged per source | PROBABLY-NOT | risks infinite knobs; keep a sensible fixed schema |

## System / health
| Setting | Tier | Why |
|---|---|---|
| Screen brightness / sleep timeout | KEEP | real power-saving knob; the LCD backlight is a big draw |
| "Down" declaration timeout (missed beats) | MAYBE | mostly a default; useful when debugging a flaky link |
| Heartbeat interval | PROBABLY-NOT | 1–2 s default is fine; no reason to expose |
| UART baud | PROBABLY-NOT | locked at 2 Mbaud; wrong value BRICKS the link — compile-time unless bench-debugging |

## Identity / session
| Setting | Tier | Why |
|---|---|---|
| Device name / operator tag | KEEP | stamped into the log to tell datasets apart across units/sessions |
| Clock set / timezone | MAYBE | GPS gives UTC; local-time-in-log is a preference; RTC needs setting pre-GPS-lock |

## Maintenance actions — DECIDED (2026-10-19)
These four are locked (the rest of this file is still candidate/pruning).

| Action | Behavior (decided) |
|---|---|
| **Reset to Defaults** | Confirmation prompt ("Reset all settings to defaults? [Cancel] [Reset]"), then clears the NVS override keys → back to stock. Destructive to overrides only; never touches captures. |
| **Format / Wipe SD** | Keep as-is. Confirmation prompt; terse "erases all CAPTURES, not settings" so there is no doubt. Wipes capture data only — config is in NVS, so this physically cannot touch settings. |
| **Safe Battery Swap** | Tap → graceful shutdown on the Waveshare: flush SD buffer, close the current log file cleanly, HALT capture. Then PARK in an idle "SAFE TO POWER OFF — swap battery" state and STAY there — do NOT auto-reboot, do NOT resume capturing (resuming would reopen the file just safely closed and re-expose the mid-write-pull risk). Operator flips the physical power switch, swaps the 18650(s), powers back on → fresh boot. |
| **Config status view** | Show the DELTAS at the TOP of the config screen: "Config: STOCK" when nothing changed, else "Modified: <keys>" (e.g. "Modified: GPS rate, dwell time, BLE scan mode"). The "did someone touch it" answer is the FIRST thing visible — no scrolling to reconstruct it. Full editable list sits below. |

---

## Maintenance actions (menu items, not "settings") — original candidates
| Action | Tier | Why |
|---|---|---|
| Reset to defaults | KEEP | universal support step; clears NVS overrides (never touches SD) |
| Format / wipe SD | KEEP | the destructive-format option — deliberately kept AWAY from config storage |
| "Safe to swap" / prepare shutdown | KEEP | the battery-swap flush action (flush SD, close file, halt capture) |
| Config status view (STOCK / MODIFIED) | KEEP | the "did someone touch it" indicator |

---

## Final shape
The decided MENU is **6 settings + 4 maintenance actions** (see PRUNED RESULT at
the top). Everything else is a compiled-in CONSTANT — which keeps the menu lean
(friction-by-design) and brick-proofs the link (UART baud) and the legal/TX
posture (BLE active is opt-in, capture otherwise passive).

The 6 menu settings are the NVS-backed override set from `ARCHITECTURE.md` #9,
surfaced through the deferred #6 UI. The tier tables ABOVE this line are the
pre-prune historical reference — the PRUNED RESULT section is authoritative
where they disagree.
