#ifndef _TEMPLATE_h
#define _TEMPLATE_h

#include "Arduino.h"

#define MAX_GPIO_PIN 50 // Number of supported GPIO

#define MAX_RELAY_NUM 4
#define MAX_PWM_NUM 4

#ifndef USE_DIMMING
#undef MAX_PWM_NUM
#define MAX_PWM_NUM 0
#endif

extern uint8_t LED_PIN;
extern uint8_t RFRECV_PIN;
extern uint8_t RELAY_PIN[MAX_RELAY_NUM];
extern uint8_t BOTTON_PIN[MAX_RELAY_NUM + MAX_PWM_NUM];
extern uint8_t RELAY_LED_PIN[MAX_RELAY_NUM + MAX_PWM_NUM];

#ifdef USE_DIMMING
extern uint8_t PWM_BRIGHTNESS_PIN[MAX_PWM_NUM];
extern uint8_t PWM_TEMPERATURE_PIN[MAX_PWM_NUM];
extern bool PWM_INVERT;
extern uint8_t ROT_PIN[2];
#endif

typedef struct MYTMPLT
{
    char name[26];
    uint8_t io[MAX_GPIO_PIN];
} mytmplt;

enum SupportedModules
{
    SONOFF_BASIC,
    CH1,
    CH2,
    CH3,
    iciness_CH3,

    Yeelight,

    MAXMODULE // 占位
};

// TLV 结构 1：LED  2：RELAY  3：BUTTON  4：RELAY LED  5：433  6：PWM1  7：PWM2  8：ROT  99：结束  IO>50 为反
const mytmplt Modules[MAXMODULE] PROGMEM = {
    {
        "Sonoff Basic", // Sonoff Basic (ESP8266)
        2, 1, 12,       // RELAY IO
        3, 1, 0,        // BUTTON IO

        99 // END
    },
    {
        "1 Channel",   // 1 Channel (ESP8285)
        1, 1, 16 + 50, // LED IO
        2, 1, 14,      // RELAY IO
        3, 1, 4,       // BUTTON IO
        4, 1, 0,       // RELAY LED IO
        5, 1, 13,      // 433 IO

        99 // END
    },
    {
        "2 Channel",   // 2 Channel (ESP8285)
        1, 1, 16 + 50, // LED IO
        2, 2, 14, 12,  // RELAY IO
        3, 2, 4, 9,    // BUTTON IO
        4, 2, 0, 2,    // RELAY LED IO
        5, 1, 13,      // 433 IO

        99 // END
    },
    {
        "3 Channel",     // 3 Channel (ESP8285)
        1, 1, 16 + 50,   // LED IO
        2, 3, 14, 12, 5, // RELAY IO
        3, 3, 4, 9, 10,  // BUTTON IO
        4, 3, 0, 2, 15,  // RELAY LED IO
        5, 1, 13,        // 433 IO

        99 // END
    },
    {
        "iciness CH3",    // 3 Channel (ESP8285)
        2, 3, 14, 15, 12, // RELAY IO
        3, 3, 9, 10, 2,   // BUTTON IO
        4, 3, 4, 5, 16,   // RELAY LED IO

        99 // END
    },
    {
        "Yeelight",    // 3 Channel (ESP32)
        1, 1, 16 + 50, // LED IO
        3, 1, 4,       // BUTTON IO
        5, 1, 13,      // 433 IO

        6, 1, 5,  // PWM1 IO
        7, 1, 12, // PWM2 IO

        99 // END
    },
};

#endif