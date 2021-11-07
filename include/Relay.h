// Relay.h
#ifndef _RELAY_h
#define _RELAY_h

#include "Module.h"
#include "RelayConfig.pb.h"
#include "Template.h"

#define RELAY_CFG_VERSION 1001 //1001 - 1500
#define RELAY_CONFIG "/relay.config"

#define MAX_STUDY_RECEIVER_NUM 10 // 遥控最大学习数

static const uint8_t DEBOUNCED_STATE = 0b00000001;
static const uint8_t UNSTABLE_STATE = 0b00000010;
static const uint8_t BUTTON_DEBOUNCE_TIME = 50; // 消抖时间

#ifdef USE_DIMMING
class Dimming;
#endif

#ifdef USE_RCSWITCH
class RadioReceive;
#endif

class Relay : public Module
{
private:
    char powerStatTopic[80];

    // 按键
    // 等待开关再次切换的时间（以毫秒为单位）。
    // 300对我来说效果很好，几乎没有引起注意。 如果您不想使用此功能，请设置为0。
    uint16_t specialFunctionTimeout = 300;
    unsigned long buttonTimingStart[MAX_RELAY_NUM + MAX_PWM_NUM];
    unsigned long buttonIntervalStart[MAX_RELAY_NUM + MAX_PWM_NUM];
    uint8_t buttonStateFlag[MAX_RELAY_NUM + MAX_PWM_NUM];
    uint8_t switchCount[MAX_RELAY_NUM + MAX_PWM_NUM];
    unsigned long lastTime[MAX_RELAY_NUM + MAX_PWM_NUM];
    void checkButton(uint8_t ch);

    // PWM
    uint16_t ledLevel = 0;
    uint16_t ledLight = 1023;
    bool ledUp = true;
    bool canLed = true;
    void led(uint8_t ch, bool isOn);
    void ledPWM(uint8_t ch, bool isOn);
    void ledTickerHandle();
    bool checkCanLed(bool re = false);

    void httpDo(WebServer *server);
    void httpSetting(WebServer *server);
    void httpHa(WebServer *server);
#ifdef USE_RCSWITCH
    void httpRadioReceive(WebServer *server);
#endif

    void loadModule(uint8_t module);
    void reportPower();
    void reportChannel(uint8_t ch);

public:
    uint8_t operationFlag = 0; // 0每秒
    RelayConfigMessage config;
    uint16_t lastState = 0;
    uint8_t channels = 0;
    Ticker ledTicker;

#ifdef USE_RCSWITCH
    RadioReceive *radioReceive = NULL;
#endif

#ifdef USE_DIMMING
    Dimming *dimming = NULL;
    char brightnessStatTopic[100];
    char color_tempStatTopic[100];
#endif

    void init();
    String getModuleName() { return F("relay"); }
    String getModuleCNName();
    String getModuleVersion() { return F("2021.01.12.2200"); }
    String getModuleAuthor() { return F("情留メ蚊子"); }
    bool moduleLed();

    void loop();
    void perSecondDo();

    void readConfig();
    void resetConfig();
    void saveConfig(bool isEverySecond);

    bool mqttCallback(char *topic, char *payload, char *cmnd);
    void mqttConnected();
    void mqttDiscovery(bool isEnable = true);

    void httpAdd(WebServer *server);
    void httpHtml(WebServer *server);
    String httpGetStatus(WebServer *server);

    void switchRelay(uint8_t ch, bool isOn, bool isSave = true);
};

#endif
