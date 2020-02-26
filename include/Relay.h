// Relay.h
#ifndef _RELAY_h
#define _RELAY_h

#include "Module.h"
#include "RelayConfig.pb.h"
#include "Template.h"

#define MODULE_CFG_VERSION 1001 //1001 - 1500

#define MAX_STUDY_RECEIVER_NUM 10 // 遥控最大学习数

const char HASS_DISCOVER_RELAY[] PROGMEM =
    "{\"name\":\"%s_%d\","
    "\"cmd_t\":\"%s\","
    "\"stat_t\":\"%s\","
    "\"pl_off\":\"OFF\","
    "\"pl_on\":\"ON\","
    "\"avty_t\":\"%s\","
    "\"pl_avail\":\"online\","
    "\"pl_not_avail\":\"offline\"}";

enum LightColor
{
    None,
    WhiteColor,
    YellowColor,
    YellowWhiteColor
};

#ifdef USE_RCSWITCH
class RadioReceive;
#endif
class RelayButton;
class Relay : public Module
{
private:
    uint8_t GPIO_PIN[MAX_GPIO_PIN - MIN_FLASH_PINS];
    uint8_t operationFlag = 0; // 0每秒

    char powerStatTopic[80];
    RelayButton *btns;

    // PWM
    Ticker *ledTicker;
    int ledLevel = 0;
    int ledLight = 2023;
    bool ledUp = true;
    bool canLed = true;
    void led(uint8_t ch, bool isOn);
    void ledPWM(uint8_t ch, bool isOn);
    void ledTickerHandle();
    bool checkCanLed(bool re = false);

    void httpDo(ESP8266WebServer *server);
    void httpSetting(ESP8266WebServer *server);
    void httpHa(ESP8266WebServer *server);
#ifdef USE_RCSWITCH
    void httpRadioReceive(ESP8266WebServer *server);
#endif

    void loadModule(uint8_t module);
    void reportPower();

#ifdef USE_TRICOLOR
    void httpDownlightSetting(ESP8266WebServer *server);
    unsigned long colorOffTime = 0;
    void colorOnOff(uint8_t ch, bool isOn);
#endif

public:
    RelayConfigMessage config;
    uint8_t lastState = 0;
    uint8_t channels = 0;

#ifdef USE_RCSWITCH
    RadioReceive *radioReceive = NULL;
#endif

    void init();
    String getModuleName() { return F("relay"); }
    String getModuleCNName();
    String getModuleVersion() { return F("2020.02.26.2100"); }
    String getModuleAuthor() { return F("情留メ蚊子"); }
    bool moduleLed();

    void loop();
    void perSecondDo();

    void readConfig();
    void resetConfig();
    void saveConfig(bool isEverySecond);

    void mqttCallback(String topicStr, String str);
    void mqttConnected();
    void mqttDiscovery(bool isEnable = true);

    void httpAdd(ESP8266WebServer *server);
    void httpHtml(ESP8266WebServer *server);
    String httpGetStatus(ESP8266WebServer *server);

    void switchRelay(uint8_t ch, bool isOn, bool isSave = true);
};

#endif
