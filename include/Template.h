#ifndef _TEMPLATE_h
#define _TEMPLATE_h

#include "Arduino.h"

#define MAX_GPIO_PIN 17  // Number of supported GPIO
#define MIN_FLASH_PINS 4 // Number of flash chip pins unusable for configuration (GPIO6, 7, 8 and 11)

typedef struct MYTMPLT
{
    char name[15];
    uint8_t io[MAX_GPIO_PIN - MIN_FLASH_PINS];
} mytmplt;

enum UserSelectablePins
{
    GPIO_NONE, // Not used
    GPIO_REL1, // Relays
    GPIO_REL2,
    GPIO_REL3,
    GPIO_REL4,
    GPIO_KEY1, // Button usually connected to GPIO0
    GPIO_KEY2,
    GPIO_KEY3,
    GPIO_KEY4,
    GPIO_LED_POWER,     // Power Led (1 = On, 0 = Off)
    GPIO_LED_POWER_INV, // Power Led (0 = On, 1 = Off)
    GPIO_LED1,          // Leds
    GPIO_LED2,
    GPIO_LED3,
    GPIO_LED4,
    GPIO_RFRECV, // RF receiver
};

enum SupportedModules
{
    SONOFF_BASIC,
    CH1,
    CH2,
    CH3,
    iciness_CH3,

    END // 占位
};

const mytmplt Modules[] PROGMEM = {
    {
        "Sonoff Basic",     // Sonoff Basic (ESP8266)
        GPIO_KEY1,          // GPIO00 Button
        GPIO_NONE,          // GPIO01
        GPIO_NONE,          // GPIO02
        GPIO_NONE,          // GPIO03
        GPIO_NONE,          // GPIO04
        0,                  // GPIO05
                            // GPIO06 (SD_CLK   Flash)
                            // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
                            // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
        0,                  // GPIO09 (SD_DATA2 Flash QIO or ESP8285)
        0,                  // GPIO10 (SD_DATA3 Flash QIO or ESP8285)
                            // GPIO11 (SD_CMD   Flash)
        GPIO_REL1,          // GPIO12 Red Led and Relay (0 = Off, 1 = On)
        GPIO_LED_POWER_INV, // GPIO13 Green Led (0 = On, 1 = Off) - Link and Power status
        GPIO_NONE,          // GPIO14
        0,                  // GPIO15
        0                   // GPIO16
    },
    {
        "1 Channel",       // 3 Channel (ESP8285)
        GPIO_LED1,         // GPIO00
        GPIO_NONE,         // GPIO01
        GPIO_NONE,         // GPIO02
        GPIO_NONE,         // GPIO03
        GPIO_KEY1,         // GPIO04
        GPIO_NONE,         // GPIO05
                           // GPIO06 (SD_CLK   Flash)
                           // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
                           // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
        GPIO_NONE,         // GPIO09 (SD_DATA2 Flash QIO or ESP8285)
        GPIO_NONE,         // GPIO10 (SD_DATA3 Flash QIO or ESP8285)
                           // GPIO11 (SD_CMD   Flash)
        GPIO_NONE,         // GPIO12
        GPIO_RFRECV,       // GPIO13
        GPIO_REL1,         // GPIO14
        GPIO_NONE,         // GPIO15
        GPIO_LED_POWER_INV // GPIO16 Led (1 = On, 0 = Off) - Link and Power status
    },
    {
        "2 Channel",       // 3 Channel (ESP8285)
        GPIO_LED1,         // GPIO00
        GPIO_NONE,         // GPIO01
        GPIO_LED2,         // GPIO02
        GPIO_NONE,         // GPIO03
        GPIO_KEY1,         // GPIO04
        GPIO_NONE,         // GPIO05
                           // GPIO06 (SD_CLK   Flash)
                           // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
                           // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
        GPIO_KEY2,         // GPIO09 (SD_DATA2 Flash QIO or ESP8285)
        GPIO_NONE,         // GPIO10 (SD_DATA3 Flash QIO or ESP8285)
                           // GPIO11 (SD_CMD   Flash)
        GPIO_REL2,         // GPIO12
        GPIO_RFRECV,       // GPIO13
        GPIO_REL1,         // GPIO14
        GPIO_NONE,         // GPIO15
        GPIO_LED_POWER_INV // GPIO16 Led (1 = On, 0 = Off) - Link and Power status
    },
    {
        "3 Channel",       // 3 Channel (ESP8285)
        GPIO_LED1,         // GPIO00
        GPIO_NONE,         // GPIO01
        GPIO_LED2,         // GPIO02
        GPIO_NONE,         // GPIO03
        GPIO_KEY1,         // GPIO04
        GPIO_REL3,         // GPIO05
                           // GPIO06 (SD_CLK   Flash)
                           // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
                           // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
        GPIO_KEY2,         // GPIO09 (SD_DATA2 Flash QIO or ESP8285)
        GPIO_KEY3,         // GPIO10 (SD_DATA3 Flash QIO or ESP8285)
                           // GPIO11 (SD_CMD   Flash)
        GPIO_REL2,         // GPIO12
        GPIO_RFRECV,       // GPIO13
        GPIO_REL1,         // GPIO14
        GPIO_LED3,         // GPIO15
        GPIO_LED_POWER_INV // GPIO16 Led (1 = On, 0 = Off) - Link and Power status
    },
    {
        "iciness CH3", // 3 Channel (ESP8285)
        GPIO_NONE,     // GPIO00
        GPIO_NONE,     // GPIO01
        GPIO_KEY3,     // GPIO02
        GPIO_NONE,     // GPIO03
        GPIO_LED1,     // GPIO04
        GPIO_LED2,     // GPIO05
                       // GPIO06 (SD_CLK   Flash)
                       // GPIO07 (SD_DATA0 Flash QIO/DIO/DOUT)
                       // GPIO08 (SD_DATA1 Flash QIO/DIO/DOUT)
        GPIO_KEY1,     // GPIO09 (SD_DATA2 Flash QIO or ESP8285)
        GPIO_KEY2,     // GPIO10 (SD_DATA3 Flash QIO or ESP8285)
                       // GPIO11 (SD_CMD   Flash)
        GPIO_REL3,     // GPIO12
        GPIO_NONE,     // GPIO13
        GPIO_REL1,     // GPIO14
        GPIO_REL2,     // GPIO15
        GPIO_LED3      // GPIO16 Led (1 = On, 0 = Off) - Link and Power status
    },
};

#endif