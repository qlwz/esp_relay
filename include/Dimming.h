#ifdef USE_DIMMING

#ifndef _DIMMING_h
#define _DIMMING_h

#include "Relay.h"

#ifdef ESP8266
#define PWM_RANGE 1023
#define PWM_SHIFT 1
#else
#define PWM_RANGE 4095
#define PWM_SHIFT 4
#endif

class Dimming
{
protected:
    Relay *relay;
    uint16_t target_color[MAX_PWM_NUM * 2] = {0};
    uint16_t current_color[MAX_PWM_NUM * 2] = {0};
    bool isFirst = true;
    Ticker pwmTicker;

public:
    uint8_t pwmstartch = 99;
    uint16_t pwm_range = PWM_RANGE;
    void loadPWM(uint8_t ch, uint8_t pin);

    void mqttCallback(char *topic, char *payload, char *cmnd);

    void init(Relay *_relay);
    void switchRelayPWM(uint8_t ch, bool isOn, bool isSave = true);
    void animate(void);

    void RotaryInit(void);
    bool RotaryButtonPressed(void);
    void RotaryHandler(void);
    void RotaryLoop(void);

    void loop();

    void httpSetBrightness(WEB_SERVER_REQUEST);
    void httpHtml(WEB_SERVER_REQUEST);
    void httpHa(WEB_SERVER_REQUEST, uint8_t ch);
    String httpGetStatus(WEB_SERVER_REQUEST);
};

#endif

#endif