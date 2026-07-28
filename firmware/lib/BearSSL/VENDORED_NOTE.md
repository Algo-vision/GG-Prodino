# Why this library is vendored

This is the BearSSL crypto library, taken from
[OPEnSLab-OSU/SSLClient](https://github.com/OPEnSLab-OSU/SSLClient) 1.6.11,
which vendors BearSSL to implement TLS on constrained boards.

**Only the crypto is kept.** The firmware uses exactly three primitives from it:

| used for | symbol |
|---|---|
| encrypt the telemetry payload | `br_chacha20_ct_run` |
| authenticate it (AEAD tag)    | `br_poly1305_ctmul32_run` |
| hash entropy into a boot id   | `br_sha256_*` |

The SSLClient TLS wrapper (`SSLClient.*`, `SSLClientParameters.*`, `SSLSession.h`,
`TLS12_only_profile.c`) was **deleted**: the board no longer performs TLS
handshakes. A TLS handshake cost this SAMD21 ~9.8 s of blocked CPU and ~1.6 KB
of heap; the AEAD it was replaced with costs ~16 ms and no heap. The measurements
are in `docs/TELEMETRY_BENCHMARK.md`, and the full TLS implementation is
preserved on the `board-only-tls` branch if it is ever needed again.
