// Simple blink + OTA example for ProDino MKR Zero Ethernet
#include "KMPProDinoMKRZero.h"
#include "gg_hal.hpp"
#include <ArduinoOTA.h>

GG_HAL gg_hal;
// Enter a MAC address and IP address for your controller below.
byte _mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};
// The IP address will be dependent on your local network.
IPAddress _ip(192, 168, 1, 198);
const uint16_t LOCAL_PORT = 80;

EthernetServer _server(LOCAL_PORT);
void setup() {
  Serial.begin(115200);
  while (!Serial);

  KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);
  gg_hal.init();

  Ethernet.begin(_mac, _ip);
  _server.begin();
  Serial.println("Server started");

  // Always enable OTA
  ArduinoOTA.begin(Ethernet.localIP(), "prodino", "", InternalStorage);
  Serial.println("OTA update enabled.");
}

void loop() {
  static unsigned long last_blink = 0;
  static bool led_on = false;

  // Handle OTA always
  ArduinoOTA.poll();

  // Blink internal LED every 500ms
  if (millis() - last_blink > 500) {
    led_on = !led_on;
    KMPProDinoMKRZero.SetStatusLed(led_on);
    last_blink = millis();
  }
}
