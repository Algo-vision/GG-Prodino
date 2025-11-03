#define LED_GREEN_LEG 26//gsm_TX - 7
#define LED_RED_LEG   27//GSM_RX - 6
#define BUTTON1_LEG  28// GSM_RTS - 5
struct gps_data
{
    double latitude;
    double longitude;
    double altitude;
    char time_str[20]; // YYYY-MM-DD hh:mm:ss
    float speed_north; // km/hr
    float speed_east;  // km/hr
    float speed_down;  // km/hr
    float ground_speed; // km/hr
    float heading;     // degrees
    bool valid;
};
enum LED_STATES
{
    GREEN,
    RED,
    ORANGE,
    OFF
};
class GG_HAL 
{
public:
    GG_HAL(/* args */){}
    ~GG_HAL(){}
    void init();
    void get_gps_data(gps_data &data);
    bool get_accel_data(float &ax, float &ay, float &az);
    bool get_gyro_data(float &gx, float &gy, float &gz);
    void get_relay_status(int relay_id);
    void set_relay(int relay_id, bool state);
    void set_indicator_led(LED_STATES state);
    void status_led_on();
    void status_led_off();
    bool get_button1_state();
    bool get_optoin_state(int opto_id);
LED_STATES get_indicator_led_state();
};

