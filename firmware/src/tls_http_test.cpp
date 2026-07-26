/**
 * @file tls_http_test.cpp
 * @brief Prove: HTTP stays ~50 ms responsive WHILE the TLS crypto runs.
 *
 * Architecture that makes it work on a single-core MCU with ~0 extra RAM:
 *   - The heavy TLS handshake crypto runs in the MAIN LOOP (background).
 *   - A fast TIMER ISR (every 5 ms) serves any pending HTTP request - so an
 *     incoming request PREEMPTS the crypto, is answered in a few ms, and the
 *     crypto resumes. No RTOS, no task stacks.
 *
 * Test from the laptop: hammer http://<board-ip>/ and watch the latency stay
 * low even though the board is grinding ECC crypto non-stop.
 *
 * Build/flash:  pio run -e tls_http_test -t upload
 */
#include <Arduino.h>
#include <Ethernet.h>
#include <bearssl.h>

extern "C" char* sbrk(int);
static int freeRam() { char t; return &t - reinterpret_cast<char*>(sbrk(0)); }

// ---- network (LAN only - no internet needed for this test) ----
static byte      s_mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};
static IPAddress s_ip(192, 168, 1, 199);
static IPAddress s_dns(192, 168, 1, 1);
static IPAddress s_gw(192, 168, 1, 1);
static IPAddress s_subnet(255, 255, 255, 0);
static EthernetServer s_server(80);

// ---- crypto (the background handshake work) ----
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

volatile uint32_t g_httpServed = 0;
volatile uint32_t g_cryptoRounds = 0;

// serve one pending HTTP request FAST (called from the 5 ms ISR)
static void serveHttpFast() {
    EthernetClient c = s_server.available();
    if (!c) return;
    c.setConnectionTimeout(2);           // don't let stop() block the ISR
    int guard = 0;
    while (c.available() && guard++ < 800) c.read();   // drain request, no waiting
    char body[48];
    int bl = snprintf(body, sizeof(body), "{\"http\":%lu,\"crypto\":%lu}",
                      (unsigned long)(g_httpServed + 1), (unsigned long)g_cryptoRounds);
    char hdr[120];
    int hl = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: %d\r\n\r\n", bl);
    c.write((const uint8_t*)hdr, hl);
    c.write((const uint8_t*)body, bl);
    c.stop();
    g_httpServed++;
}

extern "C" void TC4_Handler() {
    if (TC4->COUNT16.INTFLAG.bit.MC0) {
        TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
        serveHttpFast();                 // HTTP preempts the crypto
    }
}

static void startTimer5ms() {
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TC4_TC5;
    while (GCLK->STATUS.bit.SYNCBUSY);
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC4->COUNT16.CTRLA.bit.SWRST);
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV1024;
    TC4->COUNT16.CC[0].reg = 234;        // 46875 Hz * 0.005 s = ~234 -> 5 ms
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);
    TC4->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
    NVIC_SetPriority(TC4_IRQn, 0);
    NVIC_EnableIRQ(TC4_IRQn);
    TC4->COUNT16.CTRLA.bit.ENABLE = 1;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);
}

void setup() {
    Serial.begin(115200);
    unsigned long t = millis(); while (!Serial && millis() - t < 2000) {}
    Serial.println("\n[HTTP+TLS] === HTTP-during-crypto test ===");
    Ethernet.begin(s_mac, s_ip, s_dns, s_gw, s_subnet);
    s_server.begin();
    delay(800);
    Serial.print("[HTTP+TLS] board IP = "); Serial.println(Ethernet.localIP());
    Serial.print("[HTTP+TLS] freeRam = "); Serial.println(freeRam());
    startTimer5ms();
    Serial.println("[HTTP+TLS] HTTP served from 5ms ISR; crypto runs in main loop.");
    Serial.println("[HTTP+TLS] hit http://192.168.1.199/ and watch latency while crypto grinds.");
}

void loop() {
    // background: never-ending TLS handshake crypto (~1.6 s per round)
    oneMul(); oneMul(); oneSign();
    g_cryptoRounds++;
    if (g_cryptoRounds % 3 == 0) {
        Serial.print("[HTTP+TLS] crypto rounds="); Serial.print(g_cryptoRounds);
        Serial.print("  http served="); Serial.print(g_httpServed);
        Serial.print("  freeRam="); Serial.println(freeRam());
    }
}
