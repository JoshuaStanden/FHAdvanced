/*
 * fh_ondemand.h — FHAdvanced on-demand radio command set (hub <-> ItsyBitsy M0)
 *
 * Extends the shared UART wire protocol (fh_frame.h) with the TYPE codes and
 * payloads the HUB uses to PROMPT the ItsyBitsy M0 on-demand radio node, and
 * that the ItsyBitsy uses to RETURN results. The ItsyBitsy hosts four radios,
 * all driven ONE AT A TIME (it is single-core):
 *     CC1101 (sub-GHz)  nRF24L01+ (2.4 GHz)  PN532 (NFC)  SX1262 (LoRa)
 *
 * These TYPE codes occupy the 0x20..0x3F band, which is UNUSED by fh_frame.h
 * (that file defines 0x01-0x03, 0x10-0x11, 0x7C-0x7F). No collision.
 *
 * Plain C, framework-agnostic, little-endian, packed — same rules as fh_frame.h
 * so the hub (ESP-IDF) and the ItsyBitsy (Arduino/SAMD) share ONE definition.
 *
 * FLOW (all carried in fh_frame.h frames; this only defines TYPE + PAYLOAD):
 *   HUB --REQ--> ItsyBitsy      (a *_REQ TYPE with a request payload + seq)
 *   ItsyBitsy --RESP--> HUB     (the matching *_RESP TYPE, same seq, + status)
 * Every request carries a `seq`; the response echoes it so the hub can match
 * reply to request and time out a radio that never answers.
 */

#pragma once

#include <stdint.h>
#include "fh_frame.h"   /* for packing conventions + fh_type_t band note */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------- on-demand TYPE codes
 * 0x20..0x3F band. _REQ = hub->itsy, _RESP = itsy->hub. */
typedef enum {
    /* generic */
    FH_OD_PING_REQ      = 0x20,  /* "are you alive / ready?"                   */
    FH_OD_PING_RESP     = 0x21,  /* readiness + which radios initialised OK    */

    /* CC1101 sub-GHz */
    FH_OD_CC1101_REQ    = 0x22,  /* do a sub-GHz action (listen/burst)         */
    FH_OD_CC1101_RESP   = 0x23,  /* result (rssi, bytes captured, status)      */

    /* nRF24 2.4 GHz proprietary */
    FH_OD_NRF24_REQ     = 0x24,  /* scan/sniff a channel set                   */
    FH_OD_NRF24_RESP    = 0x25,  /* result                                     */

    /* PN532 NFC */
    FH_OD_NFC_REQ       = 0x26,  /* attempt an NFC read                        */
    FH_OD_NFC_RESP      = 0x27,  /* tag UID/type/payload or "no tag"           */

    /* SX1262 LoRa — sensor-stick relay (the promptable LoRa link) */
    FH_OD_LORA_TX_REQ   = 0x28,  /* transmit this payload to the sensor stick  */
    FH_OD_LORA_TX_RESP  = 0x29,  /* TX done / failed (+ time-on-air ms)        */
    FH_OD_LORA_RX_IND   = 0x2A,  /* UNSOLICITED: a LoRa frame arrived from the
                                  * sensor stick (itsy->hub, no matching req)  */
    FH_OD_LORA_LISTEN_REQ  = 0x2B, /* enter/leave LoRa receive window          */
    FH_OD_LORA_LISTEN_RESP = 0x2C, /* ack the listen-state change              */
} fh_od_type_t;

/* --------------------------------------------------------- status codes */
typedef enum {
    FH_OD_OK          = 0x00,
    FH_OD_BUSY        = 0x01,  /* another radio op in progress (single-core)   */
    FH_OD_NO_RESULT   = 0x02,  /* ran OK but found nothing (no tag, no frame)  */
    FH_OD_RADIO_FAIL  = 0x03,  /* the radio module did not respond / init fail */
    FH_OD_BAD_REQ     = 0x04,  /* malformed request payload                    */
    FH_OD_TIMEOUT     = 0x05,  /* op exceeded its deadline                     */
} fh_od_status_t;

/* radio id (for PING_RESP readiness bitmap + logging) */
#define FH_OD_RADIO_CC1101  (1u << 0)
#define FH_OD_RADIO_NRF24   (1u << 1)
#define FH_OD_RADIO_PN532   (1u << 2)
#define FH_OD_RADIO_SX1262  (1u << 3)

/* ============================================================ payloads
 * Every REQ starts with a seq byte; every RESP echoes seq + carries status.
 * ==================================================================== */

/* PING_REQ (0x20) — 1 B */
typedef struct __attribute__((packed)) {
    uint8_t seq;
} fh_od_ping_req_t;

/* PING_RESP (0x21) — 3 B */
typedef struct __attribute__((packed)) {
    uint8_t seq;
    uint8_t status;        /* fh_od_status_t */
    uint8_t radios_ready;  /* bitmap of FH_OD_RADIO_* that initialised OK */
} fh_od_ping_resp_t;

/* CC1101_REQ (0x22) — pick a sub-GHz action */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
    uint8_t  action;       /* 0=listen-window, 1=rx-one-packet, 2=raw-ook ...  */
    uint32_t freq_hz_khz;  /* frequency in kHz (e.g. 433920 = 433.92 MHz)      */
    uint16_t dwell_ms;     /* how long to listen before giving up              */
} fh_od_cc1101_req_t;

/* CC1101_RESP (0x23) — up to a bounded capture snippet */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
    uint8_t  status;       /* fh_od_status_t */
    int8_t   rssi;         /* dBm of what was heard (if any)                   */
    uint8_t  len;          /* bytes in `data`                                  */
    uint8_t  data[32];     /* bounded raw payload snippet (LEN covers real len)*/
} fh_od_cc1101_resp_t;

/* NRF24_REQ (0x24) */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
    uint8_t  chan_lo;      /* first 2.4 GHz channel to scan                    */
    uint8_t  chan_hi;      /* last channel                                     */
    uint16_t dwell_ms;     /* per-channel dwell                                */
} fh_od_nrf24_req_t;

/* NRF24_RESP (0x25) */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
    uint8_t  status;
    uint8_t  active_chan;  /* channel where something was seen (0xFF = none)   */
    int8_t   rssi;
} fh_od_nrf24_resp_t;

/* NFC_REQ (0x26) */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
    uint16_t timeout_ms;   /* how long to wait for a tag in the field          */
} fh_od_nfc_req_t;

/* NFC_RESP (0x27) */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
    uint8_t  status;       /* NO_RESULT if no tag presented                    */
    uint8_t  uid_len;      /* 4, 7, or 10 for typical tags                     */
    uint8_t  uid[10];
    uint8_t  tag_type;     /* implementation enum (Mifare/NTAG/etc.)           */
} fh_od_nfc_resp_t;

/* LORA_TX_REQ (0x28) — relay a payload to the sensor stick */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
    uint8_t  flags;        /* priority / ack-request bits                      */
    uint8_t  len;          /* bytes in `data`                                  */
    uint8_t  data[200];    /* opaque sensor-stick payload (LEN covers real len)*/
} fh_od_lora_tx_req_t;

/* LORA_TX_RESP (0x29) */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
    uint8_t  status;
    uint16_t toa_ms;       /* time-on-air of the transmit, ms                  */
} fh_od_lora_tx_resp_t;

/* LORA_RX_IND (0x2A) — UNSOLICITED inbound LoRa from the sensor stick.
 * seq here is the ItsyBitsy's own running counter (hub just logs/acks). */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
    int8_t   rssi;
    int8_t   snr;
    uint8_t  len;
    uint8_t  data[200];
} fh_od_lora_rx_ind_t;

/* LORA_LISTEN_REQ (0x2B) / LORA_LISTEN_RESP (0x2C) */
typedef struct __attribute__((packed)) {
    uint8_t  seq;
    uint8_t  enable;       /* 1 = enter RX window, 0 = leave                   */
    uint16_t window_ms;    /* 0 = continuous until told to stop                */
} fh_od_lora_listen_req_t;

typedef struct __attribute__((packed)) {
    uint8_t  seq;
    uint8_t  status;
    uint8_t  listening;    /* current state after the change                   */
} fh_od_lora_listen_resp_t;

/* ---- single-core contract (documented so both ends agree) ------------------
 * The ItsyBitsy M0 is single-core: it services ONE radio action at a time.
 * If a REQ arrives while another op is running, it answers FH_OD_BUSY rather
 * than queueing deeply. The hub should serialize its prompts and wait for the
 * RESP (or a timeout) before issuing the next. A long LoRa TX (high SF) can
 * hold the node busy for up to ~1 s; the hub must tolerate that latency on the
 * other on-demand radios. None of these radios is a CONTINUOUS capture stream
 * (those live on the C5/S3/hub), so serialization is acceptable by design. */

#ifdef __cplusplus
}
#endif
