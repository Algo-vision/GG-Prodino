/**
 * @file roundrobin_test.cpp
 * @brief 50 ms round-robin + TLS crypto, with free-RAM and timing prints.
 *
 * Layout that mirrors a real design:
 *   - The TLS stack is ALLOCATED (an SSLClient with its 8 KB buffer) so the
 *     free-RAM number is realistic - not the empty ~29 KB of a bare sketch.
 *   - The heavy handshake crypto runs in the MAIN LOOP.
 *   - A 50 ms hardware-timer ISR is the "other task" slice - it preempts the
 *     crypto every 50 ms to do short real-time work (sensor/watchdog/safety).
 *
 * Every round it prints: free RAM, how long the crypto took, and whether the
 * 50 ms slice held (max gap between ISR firings).
 *
 * Build/flash:  pio run -e roundrobin_test -t upload
 */
#include <Arduino.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include <bearssl.h>
#include "aws_root_ca.h"     // TAs, TAs_NUM (BearSSL trust anchor)

extern "C" char* sbrk(int);
static int freeRam() { char t; return &t - reinterpret_cast<char*>(sbrk(0)); }

// ---- allocate the TLS stack so free-RAM reflects a TLS-capable firmware ----
static EthernetClient s_eth;
static SSLClient      s_ssl(s_eth, TAs, TAs_NUM, A5);   // reserves the 8 KB TLS buffer

// ---- the handshake crypto (CPU work) ----
static const unsigned char SCALAR[32] = {
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88, 0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xF0,0x01,
    0x12,0x23,0x34,0x45,0x56,0x67,0x78,0x89, 0x9A,0xAB,0xBC,0xCD,0xDE,0xEF,0xF0,0x01
};
static const br_ec_impl* EC = &br_ec_p256_m15;
static void oneMul() {
    size_t glen; const unsigned char* G = EC->generator(BR_EC_secp256r1, &glen);
    unsigned char pt[80]; memcpy(pt, G, glen);
    EC->mul(pt, glen, SCALAR, sizeof(SCALAR), BR_EC_secp256r1);
}
static void oneSign() {
    br_ec_private_key sk; sk.curve = BR_EC_secp256r1; sk.x = (unsigned char*)SCALAR; sk.xlen = 32;
    unsigned char h[32]; memset(h, 0xAB, 32); unsigned char sig[80];
    br_ecdsa_i31_sign_raw(EC, &br_sha256_vtable, h, &sk, sig);
}

// ---- 50 ms round-robin slice (the "other task") ----
volatile uint32_t g_isrCount = 0, g_lastIsrMs = 0, g_maxGapMs = 0, g_isrWork = 0;
extern "C" void TC4_Handler() {
    if (TC4->COUNT16.INTFLAG.bit.MC0) {
        TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
        uint32_t now = millis(), gap = now - g_lastIsrMs;
        if (gap > g_maxGapMs) g_maxGapMs = gap;
        g_lastIsrMs = now; g_isrCount++; g_isrWork++;      // short real-time work
        digitalWrite(LED_BUILTIN, g_isrCount & 1);         // visible 50 ms blink
    }
}
static void startTimer50ms() {
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TC4_TC5;
    while (GCLK->STATUS.bit.SYNCBUSY);
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_SWRST; while (TC4->COUNT16.CTRLA.bit.SWRST);
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV1024;
    TC4->COUNT16.CC[0].reg = 2343;                          // 50 ms
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);
    TC4->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
    NVIC_SetPriority(TC4_IRQn, 0); NVIC_EnableIRQ(TC4_IRQn);
    TC4->COUNT16.CTRLA.bit.ENABLE = 1; while (TC4->COUNT16.STATUS.bit.SYNCBUSY);
}

void setup() {
    Serial.begin(115200);
    unsigned long t = millis(); while (!Serial && millis() - t < 2000) {}
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println("\n[RR] === 50ms round-robin + TLS crypto ===");
    Serial.print("[RR] free RAM with TLS stack (8KB buffer) reserved = ");
    Serial.print(freeRam()); Serial.println(" bytes");
    Serial.println("[RR] (the FULL production firmware would leave ~5KB LESS - HTTP+sensors+status)");
    startTimer50ms();
    g_lastIsrMs = millis();
}

void loop() {
    noInterrupts(); g_isrCount = 0; g_maxGapMs = 0; g_lastIsrMs = millis(); interrupts();

    int ramBefore = freeRam();
    uint32_t t0 = millis();
    oneMul(); oneMul(); oneSign();          // one full ECC handshake's worth of crypto
    uint32_t dt = millis() - t0;
    int ramAfter = freeRam();

    Serial.println();
    Serial.print("[RR] crypto TIME       = "); Serial.print(dt); Serial.println(" ms");
    Serial.print("[RR] free RAM (before/after) = "); Serial.print(ramBefore);
    Serial.print(" / "); Serial.print(ramAfter); Serial.println(" bytes");
    Serial.print("[RR] 50ms slice fired  = "); Serial.print(g_isrCount);
    Serial.print(" times during the crypto  (MAX gap = "); Serial.print(g_maxGapMs);
    Serial.println(" ms)");
    Serial.print("[RR] real-time ticks served while crypto ran = "); Serial.println(g_isrWork);
    delay(4000);
}
