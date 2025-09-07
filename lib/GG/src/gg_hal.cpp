#include "gg_hal.hpp"
#include "i2c_imu_gps.hpp"
#include <KMPProDinoMKRZero.h>

void GG_HAL::init()
{
    // Serial.begin(115200);

    // KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);
    Wire.begin();
    pinMode(LED_GREEN_LEG, OUTPUT);
    pinMode(LED_RED_LEG, OUTPUT);
    pinMode(BUTTON1_LEG, INPUT);
    initIMU();
}
void GG_HAL::get_gps_data(gps_data &data)
{

    bool valid = readGPSCoords(data.latitude, data.longitude, data.altitude);
    data.valid = valid;
}
bool GG_HAL::get_accel_data(float &ax, float &ay, float &az)
{
    return readAccelerometer(ax, ay, az);
}
void GG_HAL::get_relay_status(int relay_id)
{
    KMPProDinoMKRZero.GetRelayState(relay_id);
}
void GG_HAL::set_relay(int relay_id, bool state)
{
    KMPProDinoMKRZero.SetRelayState(relay_id, state);
}
void GG_HAL::set_indicator_led(LED_STATES state)
{
    switch (state)
    {
    case GREEN:
        digitalWrite(LED_GREEN_LEG, HIGH);
        digitalWrite(LED_RED_LEG, LOW);
        break;
    case RED:
        digitalWrite(LED_GREEN_LEG, LOW);
        digitalWrite(LED_RED_LEG, HIGH);
        break;
    case ORANGE:
        digitalWrite(LED_GREEN_LEG, HIGH);
        digitalWrite(LED_RED_LEG, HIGH);
        break;
    case OFF:
        digitalWrite(LED_GREEN_LEG, LOW);
        digitalWrite(LED_RED_LEG, LOW);
        break;
    default:
        break;
    }
}
void GG_HAL::status_led_on()
{
    KMPProDinoMKRZero.OnStatusLed();
}
void GG_HAL::status_led_off()
{
    KMPProDinoMKRZero.OffStatusLed();
}

bool GG_HAL::get_button1_state()
{
    return digitalRead(BUTTON1_LEG);
}

LED_STATES GG_HAL::get_indicator_led_state()
{
    if (digitalRead(LED_GREEN_LEG) == HIGH && digitalRead(LED_RED_LEG) == LOW)
        return GREEN;
    else if (digitalRead(LED_GREEN_LEG) == LOW && digitalRead(LED_RED_LEG) == HIGH)
        return RED;
    else if (digitalRead(LED_GREEN_LEG) == HIGH && digitalRead(LED_RED_LEG) == HIGH)
        return ORANGE;
    else
        return OFF;
}

bool GG_HAL::get_optoin_state(int opto_id)
{
    return KMPProDinoMKRZero.GetOptoInState(opto_id);
}
