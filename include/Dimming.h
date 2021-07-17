#ifdef USE_DIMMING

#ifndef _DIMMING_h
#define _DIMMING_h

#include "Relay.h"

#ifdef ESP8266
#define PWM_RANGE 1023
#ifndef PWM_SHIFT
#define PWM_SHIFT 1
#endif
#else
#define PWM_RANGE 4095
#ifndef PWM_SHIFT
#define PWM_SHIFT 4
#endif
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
    bool isFast = false;
    uint8_t pwmstartch = 99;
    uint16_t pwm_range = PWM_RANGE;
    
    unsigned long stateTime = 0;
    uint8_t state[MAX_PWM_NUM]; // 0: 是否下降亮度， 1：进入调光模式  2：已经调光

    void loadPWM(uint8_t ch, uint8_t pin);

    bool mqttCallback(char *topic, char *payload, char *cmnd);

    void init(Relay *_relay);
    void switchRelayPWM(uint8_t ch, bool isOn, bool isSave = true);
    void animate(void);

    void RotaryInit(void);
    bool RotaryButtonPressed(void);
    void RotaryHandler(void);
    void RotaryLoop(void);

    void loop();

    void httpSetBrightness(WebServer *server);
    void httpHtml(WebServer *server);
    void httpHa(WebServer *server, uint8_t ch);
    void mqttDiscovery(char *message, uint8_t ch);
    String httpGetStatus(WebServer *server);
};

#endif

#endif