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
 *   0    1     version = 0x01
 *   1    16    serial (ASCII, NUL-padded)
 *   17   12    nonce  = boot_epoch(4 BE) || msg_seq(8 BE)
 *   29   N     ciphertext (ChaCha20 of the status JSON)
 *   29+N 16    Poly1305 tag
 * AAD = bytes 0..28 (header authenticated but not encrypted: the server needs
 * the serial in clear to select the key).
 *
 * NONCE SAFETY: reusing a nonce with the same key is catastrophic for this
 * cipher. boot_epoch increments once per boot (persisted to flash) and msg_seq
 * increments per message within a boot, so a pair can never repeat.
 */
#ifndef SECURE_TELEMETRY_HPP
#define SECURE_TELEMETRY_HPP

#include <Arduino.h>

/** Packet layout constants (shared with the server implementation). */
constexpr uint8_t  TELEM_VERSION      = 0x01;
constexpr size_t   TELEM_SERIAL_LEN   = 16;
constexpr size_t   TELEM_NONCE_LEN    = 12;
constexpr size_t   TELEM_HEADER_LEN   = 1 + TELEM_SERIAL_LEN + TELEM_NONCE_LEN;  // 29
constexpr size_t   TELEM_TAG_LEN      = 16;

/**
 * @brief Initialise the telemetry module (bumps and persists the boot epoch).
 * Call once from setup(), after configLoad()/serialNumberLoad().
 */
void telemetryInit();

/**
 * @brief Build, encrypt and POST one status packet. Non-blocking discipline:
 *        the whole attempt is hard-capped (~300 ms) and never retries in-loop.
 * @return true if the server accepted the packet (2xx).
 */
bool telemetrySendStatus();

/** @return true if a device key has been provisioned on this board. */
bool telemetryHasKey();

#endif  // SECURE_TELEMETRY_HPP
