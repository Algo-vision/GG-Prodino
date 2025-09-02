#include "KMPProDinoMKRZero.h"
#include "KMPCommon.h"
#include "gg_hal.hpp"
byte _mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};
IPAddress _ip(192, 168, 1, 198);

const uint16_t LOCAL_PORT = 80;

EthernetServer _server(LOCAL_PORT);
GG_HAL _gg_hal;
// Client.
EthernetClient _client;

void setup()
{
    Serial.begin(115200);

    KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);

    Ethernet.begin(_mac, _ip);
    _server.begin();
    // give the Ethernet shield a second to initialize:
    delay(1000);
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
    _server.begin();
    Serial.print("Server is at ");
    Serial.print(Ethernet.localIP());
    Serial.print(":");
    Serial.println(LOCAL_PORT);

    //init HArdware Abstraction Layer
    _gg_hal.init();
    _gg_hal.status_led_on();

}

void loop(){
    _client = _server.available();
}