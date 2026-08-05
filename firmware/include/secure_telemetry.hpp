/**
 * @file secure_telemetry.hpp
 * @brief Encrypted + authenticated + replay-protected telemetry over plain HTTP.
 *
 * Replaces board-side TLS for the cloud uplink. Instead of a TLS handshake
 * (9.7 s RSA / 3.3 s ECC of asymmetric crypto on this 48 MHz CPU, plus ~1.6 KB
 * of session heap that collides with HTTP), each message is protected with
 * ChaCha20-Poly1305 AEAD using a per-board 32-byte key: ~milliseconds of CPU and
 * no heap. Same approach LoRaWAN uses for constrained devices.
 *
 * Wire format (one POST body):
 *   off  size  field
 *   0    1     version = 0x02
 *   1    16    serial (ASCII, NUL-padded)
 *   17   12    nonce  = boot_id(8, random per boot) || msg_seq(4 BE)
 *   29   N     ciphertext (ChaCha20 of the status JSON)
 *   29+N 16    Poly1305 tag
 * AAD = bytes 0..28 (header authenticated but not encrypted: the server needs
 * the serial in clear to select the key).
 *
 * NONCE SAFETY: reusing a nonce with the same key is catastrophic for this
 * cipher - it leaks the XOR of the two plaintexts and undermines the tag. The
 * nonce is therefore boot_id || msg_seq, where msg_seq increments per message
 * within a boot and boot_id is 64 fresh random bits per boot.
 *
 * WHY RANDOM AND NOT A STORED COUNTER (v0x01 did that, and it was wrong):
 * FlashStorage keeps its data inside the sketch image, so every firmware upload
 * erased the counter. It restarted at 1, the server saw a sequence it had
 * already accepted, and correctly rejected the board as a replay - a technician
 * reflashing a board would silence it until the backend was restarted.
 * 64 random bits per boot removes the persistence requirement entirely: the
 * chance of two boots colliding is negligible (~1 in 10^13 after 1000 boots),
 * and a collision would only cost that one boot's messages, not the board.
 */
#ifndef SECURE_TELEMETRY_HPP
#define SECURE_TELEMETRY_HPP

#include <Arduino.h>

/** Packet layout constants (shared with the server implementation). */
constexpr uint8_t  TELEM_VERSION      = 0x02;
constexpr size_t   TELEM_SERIAL_LEN   = 16;
constexpr size_t   TELEM_BOOT_ID_LEN  = 8;
constexpr size_t   TELEM_NONCE_LEN    = 12;
constexpr size_t   TELEM_HEADER_LEN   = 1 + TELEM_SERIAL_LEN + TELEM_NONCE_LEN;  // 29
constexpr size_t   TELEM_TAG_LEN      = 16;

/**
 * @brief Initialise the telemetry module (loads the key, draws a fresh boot id).
 * Call once from setup(), after configLoad()/serialNumberLoad() and after the
 * network is up - network timing jitter is one of the entropy sources.
 */
void telemetryInit();

/**
 * @brief Build, encrypt and POST one status packet.
 *
 * Only the head of the send blocks (build + encrypt + TCP connect + write,
 * ~95 ms, of which ~80 ms is the connect round trip). Waiting for the HTTP
 * reply and closing the socket are two further ~80 ms round trips that are
 * handed to telemetryPump() instead of spinning here.
 *
 * @return true if the request went out; the accept/reject verdict is recorded
 *         later by telemetryPump().
 */
bool telemetrySendStatus();

/**
 * @brief Finish any in-flight send. Call every loop() iteration; costs ~0 when
 *        nothing is pending.
 */
void telemetryPump();

/**
 * @brief Print min/avg/max blocking time across every send since boot.
 *
 * A load test must run with no serial monitor attached, so the per-send
 * [PROFILE] lines are lost. This survives to be read afterwards and shows
 * whether the worst send was typical or an outlier.
 */
void telemetryPrintBlockStats();

/** @return true if a device key has been provisioned on this board. */
bool telemetryHasKey();

#endif  // SECURE_TELEMETRY_HPP
