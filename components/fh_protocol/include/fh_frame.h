/*
 * fh_frame.h — FHAdvanced shared UART wire protocol (fh_protocol component)
 *
 * SINGLE SOURCE OF TRUTH for the inter-board UART frame format. Included by all
 * three firmwares (hub / XIAO-C5 / XIAO-S3) so the frame definitions cannot
 * drift. Transcribed verbatim from docs/ARCHITECTURE.md "UART frame format".
 *
 * Frame layout (self-synchronizing, length-driven, CRC-checked):
 *   off size field     notes
 *   0   2    SYNC      0xA5 0x5A  frame start marker (resync anchor)
 *   2   1    TYPE      message type (fh_type_t)
 *   3   1    LEN       payload length in bytes (0..255)
 *   4   LEN  PAYLOAD   type-specific body
 *  4+LEN 2   CRC16     CRC-16/CCITT over TYPE..end-of-PAYLOAD (NOT the SYNC)
 *
 * Max frame = 2 + 1 + 1 + 255 + 2 = 261 bytes.
 * Endianness: LITTLE-ENDIAN for all multi-byte fields (native to every ESP32
 * chip here — no byte-swapping). All payload structs are __attribute__((packed))
 * so compiler padding cannot misalign the specced byte layout.
 *
 * This header is plain C and framework-agnostic (no ESP-IDF or Arduino deps),
 * so it compiles identically in every project.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ framing */

#define FH_SYNC0            0xA5u   /* first  SYNC byte */
#define FH_SYNC1            0x5Au   /* second SYNC byte */

#define FH_HDR_LEN          4u      /* SYNC(2) + TYPE(1) + LEN(1)             */
#define FH_CRC_LEN          2u      /* CRC16 trailer                          */
#define FH_MAX_PAYLOAD      255u    /* LEN is a single byte                   */
#define FH_MAX_FRAME        (FH_HDR_LEN + FH_MAX_PAYLOAD + FH_CRC_LEN) /* 261 */

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, no xorout. */
#define FH_CRC16_POLY       0x1021u
#define FH_CRC16_INIT       0xFFFFu

/* -------------------------------------------------------------- TYPE codes */
/* Values are fixed by the wire protocol — never renumber. */
typedef enum {
    FH_TYPE_CAPTURE       = 0x01, /* daughterboard -> hub : capture record    */
    FH_TYPE_GPS_FIX       = 0x02, /* C5 -> hub            : position push      */
    FH_TYPE_SUMMARY       = 0x03, /* daughterboard -> hub : per-device count   */
    FH_TYPE_LORA_RELAY_TX = 0x10, /* hub -> XIAO-S3       : payload to send    */
    FH_TYPE_LORA_RELAY_RX = 0x11, /* XIAO-S3 -> hub       : payload received   */
    FH_TYPE_HEARTBEAT     = 0x7E, /* daughterboard -> hub : liveness counters  */
    FH_TYPE_PING          = 0x7D, /* hub -> daughterboard : health check       */
    FH_TYPE_PONG          = 0x7C, /* daughterboard -> hub : reply to PING      */
    FH_TYPE_ACK           = 0x7F, /* both ways            : ack a LORA_RELAY   */
} fh_type_t;

/* ------------------------------------------------------- src_id enumeration */
/* Maps to the SD `source` column. 1=C5 5GHz, 2=XS3 2.4GHz. */
typedef enum {
    FH_SRC_C5_WIFI5   = 1,
    FH_SRC_XS3_WIFI24 = 2,
} fh_src_id_t;

/* ============================================================ payload structs
 * All little-endian, all packed. Sizes below match ARCHITECTURE.md exactly.
 * ========================================================================== */

/* CAPTURE (TYPE 0x01) — ~34 B. `extra` is bounded; the frame LEN carries the
 * real used length, so a sender MAY emit fewer than sizeof bytes of extra.
 * Receivers must use LEN, not sizeof, to know how many extra bytes are valid. */
typedef struct __attribute__((packed)) {
    uint8_t  src_id;      /* fh_src_id_t                                      */
    uint8_t  band_chan;   /* high nibble = band (2.4/5), low = channel index  */
    uint8_t  frame_type;  /* mgmt/data/ctrl + subtype (probe-req, beacon, …)  */
    int8_t   rssi;        /* dBm                                              */
    uint8_t  mac[6];      /* 6-byte device MAC                                */
    uint32_t gps_time;    /* epoch s; C5 fills real GPS time, else 0          */
    int32_t  lat;         /* degrees x 1e7 (0 if no fix)                      */
    int32_t  lon;         /* degrees x 1e7 (0 if no fix)                      */
    uint16_t count;       /* occurrences since last summary (dedup count)     */
    uint8_t  extra[10];   /* type-specific; bounded, real length from LEN     */
} fh_capture_t;           /* 24 fixed + 10 extra = 34 B                        */

/* Fixed (non-extra) portion of a CAPTURE payload. A sender that emits E bytes
 * of extra sets LEN = FH_CAPTURE_FIXED + E (E in 0..10). */
#define FH_CAPTURE_FIXED   (offsetof(fh_capture_t, extra))   /* 24 */
#define FH_CAPTURE_EXTRA_MAX  10u

/* GPS_FIX (TYPE 0x02) — 16 B. Hub caches this as last-known position. */
typedef struct __attribute__((packed)) {
    uint32_t gps_time;    /* epoch seconds        */
    int32_t  lat;         /* deg x 1e7            */
    int32_t  lon;         /* deg x 1e7            */
    uint8_t  sats;        /* satellites in fix    */
    uint8_t  fix_qual;    /* fix quality / HDOP bucket */
    uint16_t reserved;    /* pad to 16            */
} fh_gps_fix_t;           /* 16 B */

/* SUMMARY (TYPE 0x03) — periodic per-device count. Same identity keying as a
 * CAPTURE row; carries the accumulated count since the last summary. */
typedef struct __attribute__((packed)) {
    uint8_t  src_id;
    uint8_t  band_chan;
    uint8_t  mac[6];
    uint16_t count;       /* occurrences accumulated in this summary window   */
    uint32_t last_seen_s; /* seconds-since-boot of last sighting (sender clk) */
} fh_summary_t;           /* 14 B */

/* LORA_RELAY (TYPE 0x10 / 0x11). `data` is opaque; N = LEN - 2. */
typedef struct __attribute__((packed)) {
    uint8_t  seq;         /* sequence number (echoed in ACK 0x7F)             */
    uint8_t  flags;       /* transport/priority flags                         */
    uint8_t  data[FH_MAX_PAYLOAD - 2]; /* opaque; real length from LEN        */
} fh_lora_relay_t;
#define FH_LORA_RELAY_HDR  2u          /* seq + flags                          */
#define FH_LORA_DATA_MAX   (FH_MAX_PAYLOAD - FH_LORA_RELAY_HDR)

/* HEARTBEAT (TYPE 0x7E) — 14 B. Sent on a fixed cadence (1–2 s) whether or not
 * anything was captured. Carries proof-of-work, not just "alive". */
typedef struct __attribute__((packed)) {
    uint8_t  src_id;        /* which daughterboard                            */
    uint32_t uptime_s;      /* seconds since boot — reset to 0 = silent reboot*/
    uint32_t capture_count; /* records captured since boot — must keep rising */
    uint16_t dropped_keys;  /* seen-table evictions (saturation indicator)    */
    uint8_t  task_alive;    /* bitmask: capture / GPS / LoRa task liveness    */
    uint16_t free_heap_kb;  /* KB free — catch a slow leak before a crash     */
} fh_heartbeat_t;           /* 14 B */

/* task_alive bit assignments (daughterboard reports which tasks are ticking) */
#define FH_TASK_CAPTURE     (1u << 0)
#define FH_TASK_GPS         (1u << 1)   /* C5 only  */
#define FH_TASK_LORA        (1u << 2)   /* XS3 only */

/* PING (0x7D) / PONG (0x7C) — 1 B. PONG echoes the PING's seq. */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
} fh_ping_t;                /* also used for PONG */

/* ACK (0x7F) — 1 B. Echoes the seq of the LORA_RELAY being acked. */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
} fh_ack_t;

/* Compile-time guards: if any struct drifts from the specced size, the build
 * FAILS here rather than silently shipping a wrong wire layout. */
_Static_assert(sizeof(fh_capture_t)   == 34, "fh_capture_t must be 34 bytes");
_Static_assert(sizeof(fh_gps_fix_t)   == 16, "fh_gps_fix_t must be 16 bytes");
_Static_assert(sizeof(fh_summary_t)   == 14, "fh_summary_t must be 14 bytes");
_Static_assert(sizeof(fh_heartbeat_t) == 14, "fh_heartbeat_t must be 14 bytes");
_Static_assert(FH_CAPTURE_FIXED       == 24, "CAPTURE fixed part must be 24 bytes");

/* ===================================================================== CRC */

/* CRC-16/CCITT-FALSE over `len` bytes of `data`. Used over TYPE..PAYLOAD. */
uint16_t fh_crc16(const uint8_t *data, size_t len);

/* ============================================================ encode / decode
 * ========================================================================== */

/* Encode one frame into `out` (must hold >= FH_MAX_FRAME bytes).
 * `payload` may be NULL iff payload_len == 0.
 * Returns the total framed length (header+payload+crc), or 0 on error
 * (payload_len > FH_MAX_PAYLOAD, or out == NULL). */
size_t fh_frame_encode(fh_type_t type,
                       const uint8_t *payload, uint8_t payload_len,
                       uint8_t *out, size_t out_cap);

/* ---- streaming decoder (resync-safe) --------------------------------------
 * Feed bytes as they arrive from the UART; the decoder scans for SYNC, reads
 * TYPE/LEN, collects LEN+2 more bytes, checks CRC16, and hands back one
 * complete, validated frame via the callback. A dropped byte costs at most one
 * frame, never stream alignment (length-driven framing + SYNC re-anchor). */

typedef void (*fh_frame_cb_t)(fh_type_t type,
                              const uint8_t *payload, uint8_t len,
                              void *user);

/* Decoder state. Zero-initialize before first use (fh_decoder_init). */
typedef struct {
    uint8_t  buf[FH_MAX_FRAME]; /* working frame assembly buffer              */
    uint16_t have;              /* bytes currently in buf                     */
    uint8_t  want;              /* payload length once the header is parsed   */
    uint8_t  state;             /* internal parse state                       */
    /* diagnostics — handy on the hub UI */
    uint32_t good_frames;
    uint32_t crc_errors;
    uint32_t resyncs;
} fh_decoder_t;

void fh_decoder_init(fh_decoder_t *d);

/* Push `n` received bytes through the decoder. For every complete, CRC-valid
 * frame found, `cb` is invoked with the payload (a pointer INTO the decoder's
 * buffer — valid only for the duration of the callback; copy if you keep it). */
void fh_decoder_push(fh_decoder_t *d, const uint8_t *bytes, size_t n,
                     fh_frame_cb_t cb, void *user);

#ifdef __cplusplus
}
#endif
