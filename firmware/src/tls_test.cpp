/**
 * @file tls_test.cpp
 * @brief CLEAN debug firmware: ONLY MQTT-over-TLS to AWS IoT. No HTTP, no config,
 *        no status manager, no OCU, no sensors.
 *
 * Purpose: measure whether the ProDino board (SAMD21, 32 KB RAM) can sustain a
 * direct mutual-TLS MQTT connection to AWS IoT and PUBLISH at 1 Hz with nothing
 * else running - i.e. the best-case for board-only Option A (no gateway).
 *
 * Build/flash:  pio run -e tls_test -t upload   (from firmware/)
 * Watch:        pio device monitor -e tls_test
 *
 * Certs come from the gitignored aws_certs_store.h (device cert/key + endpoint)
 * and aws_root_ca.h (Amazon Root CA as a BearSSL trust anchor).
 */
#include <KMPProDinoMKRZero.h>
#include <KMPCommon.h>
#include <Ethernet.h>
#include <Dns.h>
#include <SSLClient.h>
#include <PubSubClient.h>
#include <ArduinoECCX08.h>   // to detect/use the ATECC508/608 hardware crypto chip
#include "aws_certs_store.h"   // AWS_ENDPOINT, AWS_PORT, AWS_DEV_CERT (RSA-2048)
#include "aws_ecc_certs.h"     // ECC_CERT, ECC_KEY (P-256, for the handshake benchmark)
#include "aws_root_ca.h"       // TAs, TAs_NUM

// Flip this to compare handshake time: 0 = RSA-2048 client cert, 1 = ECC P-256.
#define USE_ECC 1
#if USE_ECC
  #define CERT_TYPE "ECC-P256"
#else
  #define CERT_TYPE "RSA-2048"
#endif

// ---- network: laptop shares its WiFi internet to the board over a DIRECT
//      Ethernet cable (laptop = gateway 192.168.100.1, NATs us out to AWS). ----
static byte      s_mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};
static IPAddress s_ip(192, 168, 100, 50);
static IPAddress s_dns(8, 8, 8, 8);
static IPAddress s_gw(192, 168, 100, 1);       // the laptop = our way out to the internet
static IPAddress s_subnet(255, 255, 255, 0);

// ---- TLS stack: EthernetClient -> SSLClient(BearSSL) -> PubSubClient ----
static EthernetClient s_eth;
static SSLClient      s_ssl(s_eth, TAs, TAs_NUM, A5);   // A5 = analog pin for RNG seed

// SSLClient buffers writes and PubSubClient never flushes -> wrap it so every
// write is flushed out as a TLS record immediately (needed for CONNECT/PUBLISH).
class FlushingClient : public Client {
    SSLClient& c;
public:
    FlushingClient(SSLClient& s) : c(s) {}
    int connect(IPAddress ip, uint16_t p) override { return c.connect(ip, p); }
    int connect(const char* h, uint16_t p) override { return c.connect(h, p); }
    size_t write(uint8_t b) override { size_t n = c.write(b); c.flush(); return n; }
    size_t write(const uint8_t* b, size_t s) override { size_t n = c.write(b, s); c.flush(); return n; }
    int available() override { return c.available(); }
    int read() override { return c.read(); }
    int read(uint8_t* b, size_t s) override { return c.read(b, s); }
    int peek() override { return c.peek(); }
    void flush() override { c.flush(); }
    void stop() override { c.stop(); }
    uint8_t connected() override { return c.connected(); }
    operator bool() override { return (bool)c; }
};
static FlushingClient s_flushing(s_ssl);
static PubSubClient   s_mqtt(s_flushing);

// mutual-auth params (device cert + private key) - RSA or ECC per USE_ECC
#if USE_ECC
static SSLClientParameters s_mTLS =
    SSLClientParameters::fromPEM(ECC_CERT, strlen(ECC_CERT), ECC_KEY, strlen(ECC_KEY));
#else
static SSLClientParameters s_mTLS =
    SSLClientParameters::fromPEM(AWS_DEV_CERT, strlen(AWS_DEV_CERT),
                                 AWS_DEV_KEY,  strlen(AWS_DEV_KEY));
#endif

extern "C" char* sbrk(int);
static int freeRam() { char top; return &top - reinterpret_cast<char*>(sbrk(0)); }

static unsigned long s_lastPub = 0;
static int s_pubCount = 0;

void setup() {
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && millis() - t0 < 3000) {}
    Serial.println("\n[TLS] ===== clean MQTT+TLS-only debug firmware =====");
    Serial.print("[TLS] freeRam at boot: "); Serial.println(freeRam());

    KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);

    // ---- Probe for the ATECC508/608 hardware crypto chip ----
    // If present, TLS crypto can be offloaded to hardware (ms, not seconds) and
    // the private key stored securely. This is the deciding factor for a
    // non-blocking board-only TLS design.
    Serial.println("[TLS] --- crypto chip probe ---");
    // Full I2C scan: proves the bus works (MSB sensors 0x6A/0x6B/0x42 should show)
    // and whether anything sits at the ATECC crypto address 0x60.
    Wire.begin();
    Serial.print("[TLS] I2C devices:");
    for (uint8_t a = 1; a < 127; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) { Serial.print(" 0x"); Serial.print(a, HEX); }
    }
    Serial.println("  (0x60 = ATECC crypto chip if present)");
    if (ECCX08.begin()) {
        Serial.print("[TLS] *** HW CRYPTO CHIP FOUND ***  serial=");
        Serial.println(ECCX08.serialNumber());
        Serial.print("[TLS] config locked="); Serial.print(ECCX08.locked() ? "YES" : "no");
        Serial.println("  (locked+provisioned = ready for TLS offload)");
    } else {
        Serial.println("[TLS] NO crypto chip detected (ECCX08.begin failed)");
        Serial.println("[TLS] -> board likely has no HW crypto; TLS stays on the CPU");
    }
    Serial.println("[TLS] --- end probe ---");

    // Try DHCP first (works when plugged into the home router - it hands us an IP,
    // gateway, DNS and an internet route). Fall back to the static laptop-share IP.
    Serial.println("[TLS] trying DHCP (plug into router for internet)...");
    if (Ethernet.begin(s_mac) == 0) {
        Serial.println("[TLS] no DHCP -> static fallback (laptop-share 192.168.100.50)");
        Ethernet.begin(s_mac, s_ip, s_dns, s_gw, s_subnet);
    }
    delay(1500);
    Serial.print("[TLS] IP: "); Serial.println(Ethernet.localIP());
    Serial.print("[TLS] gateway: "); Serial.println(Ethernet.gatewayIP());
    Serial.print("[TLS] dns: "); Serial.println(Ethernet.dnsServerIP());
    // W5500 self-view: hardwareStatus (5=W5500), linkStatus (1=ON,2=OFF,0=unknown)
    Serial.print("[TLS] W5500 hwStatus="); Serial.print((int)Ethernet.hardwareStatus());
    Serial.print("  linkStatus="); Serial.print((int)Ethernet.linkStatus());
    Serial.println("  (link 1=ON 2=OFF 0=unknown)");
    Serial.print("[TLS] endpoint: "); Serial.print(AWS_ENDPOINT);
    Serial.print(":"); Serial.println(AWS_PORT);

    // --- network reachability diagnostic: is it DNS or TCP that fails? ---
    IPAddress resolved;
    DNSClient dns; dns.begin(Ethernet.dnsServerIP());
    unsigned long td = millis();
    int r = dns.getHostByName(AWS_ENDPOINT, resolved);
    Serial.print("[TLS] DNS getHostByName r="); Serial.print(r);
    Serial.print(" ("); Serial.print(millis() - td); Serial.print(" ms) -> ");
    Serial.println(resolved);
    if (r == 1) {
        EthernetClient probe;
        unsigned long tc = millis();
        int c = probe.connect(resolved, AWS_PORT);
        Serial.print("[TLS] raw TCP connect to endpoint:8883 = "); Serial.print(c);
        Serial.print(" ("); Serial.print(millis() - tc); Serial.println(" ms)");
        probe.stop();
    }

    s_ssl.setMutualAuthParams(s_mTLS);
    s_mqtt.setServer(AWS_ENDPOINT, AWS_PORT);
    s_mqtt.setBufferSize(512);
    s_mqtt.setKeepAlive(60);        // AWS is happier with a longer keepalive than the 15s default
    s_mqtt.setSocketTimeout(15);
    Serial.print("[TLS] freeRam after init: "); Serial.println(freeRam());
}

static int s_connAttempts = 0;

static void connectMqtt() {
    // Unique client id per attempt: if a previous TCP session lingers at AWS,
    // reconnecting with the SAME id makes AWS kick one of them (DUPLICATE_CLIENTID),
    // which can sustain a connect/drop flap. A fresh id each time rules that out.
    String cid = "grk-tls-" + String(s_connAttempts++);
    Serial.print("[TLS] connecting as "); Serial.print(cid);
    Serial.print(" (TLS handshake ~10s)... freeRam="); Serial.println(freeRam());
    unsigned long t0 = millis();
    if (s_mqtt.connect(cid.c_str())) {
        Serial.print("[TLS] *** CONNECTED *** in "); Serial.print(millis() - t0);
        Serial.print(" ms  freeRam="); Serial.println(freeRam());
        // Don't publish the instant the handshake ends - let the session settle
        // and mqtt.loop() service it for a beat first.
        s_lastPub = millis();
    } else {
        Serial.print("[TLS] connect FAILED rc="); Serial.print(s_mqtt.state());
        Serial.print(" after "); Serial.print(millis() - t0);
        Serial.print(" ms  freeRam="); Serial.println(freeRam());
        s_ssl.stop();          // clear SSLClient's latched write-error
        delay(3000);
    }
}

// ============================================================================
// HANDSHAKE-TIMING BENCHMARK
// Times ONLY the TLS handshake (the crypto that blocks the CPU), so we can
// compare RSA-2048 vs ECC-P256 on this exact board. Flip USE_ECC and reflash.
// The MQTT/publish doesn't matter here - we measure s_ssl.connect() directly.
// ============================================================================
void loop() {
    // --- (a) NETWORK-ONLY baseline: raw TCP connect, zero crypto ---
    IPAddress ip;
    DNSClient d; d.begin(Ethernet.dnsServerIP()); d.getHostByName(AWS_ENDPOINT, ip);
    EthernetClient probe;
    unsigned long n0 = millis();
    probe.connect(ip, AWS_PORT);
    unsigned long net_ms = millis() - n0;
    probe.stop();
    Serial.print("\n[BENCH] network only (raw TCP connect, NO crypto) = ");
    Serial.print(net_ms); Serial.println(" ms");

    // --- (b) the TLS handshake: the CPU-bound crypto ---
    Serial.print("[BENCH] >>>>> START crypto ("  CERT_TYPE ")  millis=");
    Serial.println(millis());
    unsigned long t0 = millis();
    int r = s_ssl.connect(AWS_ENDPOINT, AWS_PORT);   // BLOCKS here doing crypto
    unsigned long dt = millis() - t0;
    Serial.print("[BENCH] <<<<< STOP  crypto        millis=");
    Serial.println(millis());

    Serial.print("[BENCH] *** " CERT_TYPE ": total handshake=");
    Serial.print(dt); Serial.print(" ms  |  network~"); Serial.print(net_ms);
    Serial.print(" ms  |  CPU crypto (blocked) ~"); Serial.print((long)dt - (long)net_ms);
    Serial.println(" ms ***");
    s_ssl.stop();
    delay(4000);
}
