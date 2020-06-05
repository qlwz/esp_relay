// Relay.h
#ifndef _RELAY_h
#define _RELAY_h
#include "Module.h"
#include "RelayConfig.pb.h"
#include "Template.h"
#include "../customize.ini"

const char HASS_DISCOVER_RELAY[] PROGMEM =
    "{\"name\":\"%s_%d\","
    "\"cmd_t\":\"%s\","
    "\"stat_t\":\"%s\","
    "\"pl_off\":\"off\","
    "\"pl_on\":\"on\","
    "\"avty_t\":\"%s\","
    "\"pl_avail\":\"online\","
    "\"pl_not_avail\":\"offline\"}";

static const uint8_t DEBOUNCED_STATE = 0b00000001;
static const uint8_t UNSTABLE_STATE = 0b00000010;

#ifdef USE_RCSWITCH
class RadioReceive;
#endif
class Relay : public Module
{
private:
    uint8_t operationFlag = 0; // 0每秒

    char powerStatTopic[80];

    // 按键
    // 等待开关再次切换的时间（以毫秒为单位）。
    // 300对我来说效果很好，几乎没有引起注意。 如果您不想使用此功能，请设置为0。
    unsigned long specialFunctionTimeout = 300;
    uint8_t buttonDebounceTime = 50;
    unsigned long buttonTimingStart[4];
    unsigned long buttonIntervalStart[4];
    uint8_t buttonStateFlag[4];
    uint8_t switchCount[4];
    int lastTime[4];
    void cheackButton(uint8_t ch);

    // PWM
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

public:
    uint8_t GPIO_PIN[GPIO_MAX];
    //自定义 备注：开关参数变量实例
    RelayConfigMessage config;
    uint8_t lastState = 0;
    uint8_t channels = 0;
    Ticker ledTicker;

#ifdef USE_RCSWITCH
    RadioReceive *radioReceive = NULL;
#endif

    void init();
    String getModuleName() { return F("relay"); }
    String getModuleCNName();
    String getModuleVersion() { return F("2020.02.29.2100"); }
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
