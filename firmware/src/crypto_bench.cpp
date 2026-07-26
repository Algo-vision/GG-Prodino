/**
 * @file crypto_bench.cpp
 * @brief Test: can a 50 ms hardware-timer interrupt preempt the crypto?
 *
 * A TC timer fires an ISR every 50 ms. We run the full ~1.6 s ECC handshake
 * crypto in the main loop and count how often the ISR fired DURING it, plus the
 * max gap between firings. If the ISR keeps firing every ~50 ms while the crypto
 * runs, then the crypto IS being preempted every 50 ms - short real-time work
 * (sensor sampling, watchdog, safety pin) can run in that ISR at ~zero RAM cost.
 * (This is NOT a full RTOS - no task stacks - so no RAM wall.)
 *
 * Build/flash:  pio run -e crypto_bench -t upload
 */
#include <Arduino.h>
#include <bearssl.h>

extern "C" char* sbrk(int);
static int freeRam() { char t; return &t - reinterpret_cast<char*>(sbrk(0)); }

static const unsigned char SCALAR[32] = {
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xF0,0x01,
    0x12,0x23,0x34,0x45,0x56,0x67,0x78,0x89,
    0x9A,0xAB,0xBC,0xCD,0xDE,0xEF,0xF0,0x01
};
static const br_ec_impl* EC = &br_ec_p256_m15;

static unsigned long onePointMul() {
    size_t glen; const unsigned char* G = EC->generator(BR_EC_secp256r1, &glen);
    unsigned char pt[80]; memcpy(pt, G, glen);
    return (EC->mul(pt, glen, SCALAR, sizeof(SCALAR), BR_EC_secp256r1), 0);
}
static void oneSign() {
    br_ec_private_key sk; sk.curve = BR_EC_secp256r1;
    sk.x = (unsigned char*)SCALAR; sk.xlen = sizeof(SCALAR);
    unsigned char h[32]; memset(h, 0xAB, 32); unsigned char sig[80];
    br_ecdsa_i31_sign_raw(EC, &br_sha256_vtable, h, &sk, sig);
}

// ---- 50 ms timer ISR: the "preemption". Runs short real-time work. ----
volatile uint32_t g_isrCount = 0;
volatile uint32_t g_lastIsrMs = 0;
volatile uint32_t g_maxGapMs  = 0;
volatile uint32_t g_isrWork   = 0;   // stand-in for sensor read / watchdog / safety pin

extern "C" void TC4_Handler() {
    if (TC4->COUNT16.INTFLAG.bit.MC0) {
        TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;   // clear flag
        uint32_t now = millis();
        uint32_t gap = now - g_lastIsrMs;
        if (gap > g_maxGapMs) g_maxGapMs = gap;
        g_lastIsrMs = now;
        g_isrCount++;
        g_isrWork++;                                  // <- short real-time task here
        digitalWrite(LED_BUILTIN, g_isrCount & 1);    // visible: LED toggles every 50 ms
    }
}

static void startTimer50ms() {
    // GCLK0 (48 MHz) -> TC4
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TC4_TC5;
    while (GCLK->STATUS.bit.SYNCBUSY);
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC4->COUNT16.CTRLA.bit.SWRST);
    // 48 MHz / 1024 = 46875 Hz; MFRQ resets on CC0 -> 50 ms => CC0 = 2343
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_PRESCALER_DIV1024;
    TC4->COUNT16.CC[0].reg = 2343;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);
    TC4->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
    NVIC_SetPriority(TC4_IRQn, 0);
    NVIC_EnableIRQ(TC4_IRQn);
    TC4->COUNT16.CTRLA.bit.ENABLE = 1;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);
}

void setup() {
    Serial.begin(115200);
    unsigned long t = millis(); while (!Serial && millis() - t < 3000) {}
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println("\n[PREEMPT] === 50 ms timer-interrupt preemption test (offline) ===");
    startTimer50ms();
    Serial.print("[PREEMPT] freeRam after timer setup = "); Serial.println(freeRam());
    g_lastIsrMs = millis();
}

void loop() {
    // reset counters, then run the FULL ~1.6 s crypto uninterrupted-looking
    noInterrupts(); g_isrCount = 0; g_maxGapMs = 0; g_lastIsrMs = millis(); interrupts();

    Serial.println("\n[PREEMPT] running full ECC handshake crypto (~1.6s) with 50ms ISR live...");
    uint32_t t0 = millis();
    onePointMul();      // ephemeral keygen
    onePointMul();      // shared secret
    oneSign();          // client signature
    uint32_t dt = millis() - t0;

    uint32_t cnt = g_isrCount, gap = g_maxGapMs, work = g_isrWork;
    Serial.print("[PREEMPT] crypto took "); Serial.print(dt); Serial.println(" ms of solid CPU work");
    Serial.print("[PREEMPT] during it, the 50ms ISR fired "); Serial.print(cnt);
    Serial.print(" times  (expected ~"); Serial.print(dt / 50); Serial.println(")");
    Serial.print("[PREEMPT] >>> MAX gap between ISR firings = "); Serial.print(gap);
    Serial.println(" ms  <<<  (this is the real 'max blocked' for the ISR task)");
    Serial.print("[PREEMPT] total real-time ticks served = "); Serial.println(work);
    Serial.print("[PREEMPT] freeRam = "); Serial.print(freeRam());
    Serial.println("  (timer ISR cost ~0 RAM - no task stacks, no RTOS)");
    if (gap <= 60)
        Serial.println("[PREEMPT] VERDICT: crypto WAS preempted every ~50ms. Concept works.");
    else
        Serial.println("[PREEMPT] VERDICT: ISR was delayed - crypto blocked it. Concept limited.");
    delay(5000);
}
