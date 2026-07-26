# Why this library is vendored (not in lib_deps)

OPEnSLab-OSU/SSLClient@1.6.11, vendored with ONE local change:

- `src/SSLClient.h`: `m_iobuf` enlarged 2048 -> 8192 bytes. AWS IoT does not
  support TLS max-fragment-length negotiation (MFLN), so its multi-KB
  certificate records don't fit the upstream 2048-byte buffer and the TLS
  handshake fails. The library's own comment says ~8000 bytes are needed
  when MFLN is unavailable.

If you update this library, re-apply that change or AWS IoT TLS will break.
