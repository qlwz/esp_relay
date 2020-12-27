#ifdef USE_WEILE
#include "Relay.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define TEMPERATURE_1_PIN 9
#define TEMPERATURE_2_PIN 4
#define FLOW_SENSOR_PIN 2
#define RELAY_CH 0

OneWire oneWire1(TEMPERATURE_1_PIN);
OneWire oneWire2(TEMPERATURE_2_PIN);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);
DeviceAddress insideThermometer1;
DeviceAddress insideThermometer2;

// YF-S402 频率：F=73*Q(Q 为流量 L/min) 误差：±10 流完一升水输出 4380 个脉冲
float calibrationFactor1 = 550;
float frequencyFactor = 73;
volatile byte pulseCount = 0;

// 当前温度
float currentTemperature = DEVICE_DISCONNECTED_C;
bool weileLastState = 0;
unsigned long weileRunTime = 0;
unsigned long weileStopTime = 0;

int flowCount = 0;

ICACHE_RAM_ATTR void pulseCounter1()
{
    pulseCount++;
}

unsigned long previousTime = 0;
unsigned long isflowOn = false;
ICACHE_RAM_ATTR void checkFlow()
{
    unsigned long elapsedTime = millis() - previousTime;
    if (elapsedTime >= 1000)
    {
        // 计算前禁用中断
        detachInterrupt(FLOW_SENSOR_PIN);
        /*
        // 计算流量
        float litresFlowed1 = pulseCount1 / calibrationFactor1;
        // 计算脉冲频率
        float pulseFrequency1 = pulseCount1 / (elapsedTime / 1000.0);
        // 计算流速(单位：L/min)
        float flowRate1 = pulseFrequency1 / frequencyFactor;
        */
        if (pulseCount > 1)
        {
            if (flowCount < 0)
            {
                flowCount = 0;
            }
            flowCount++;
            if (flowCount == 3)
            {
                isflowOn = true;
                Log::Info(PSTR("send flow on"));
                Mqtt::publish(Mqtt::getStatTopic(F("flow")), "on", globalConfig.mqtt.retain);
            }
        }
        else
        {
            if ((flowCount > 3 && flowCount <= 6) && !bitRead(((Relay *)module)->lastState, RELAY_CH)) // 脉冲2-6秒 开启回水器
            {
                Log::Info(PSTR("flowCount:%d temp:%f start"), flowCount, currentTemperature);
                if (millis() - weileStopTime > 600 * 1000 && currentTemperature < 40.0f)
                {
                    ((Relay *)module)->switchRelay(RELAY_CH, true, true);
                }
            }
            if (flowCount > 0)
            {
                flowCount = 0;
            }
            flowCount--;
            if (flowCount == -5 && isflowOn)
            {
                isflowOn = false;
                Log::Info(PSTR("send flow off"));
                Mqtt::publish(Mqtt::getStatTopic(F("flow")), "off", globalConfig.mqtt.retain);
            }
        }
        //Log::Info("Flow: %d %d", pulseCount, flowCount);
        // 重置脉冲计数器
        pulseCount = 0;
        previousTime = millis();
        // 恢复中断
        attachInterrupt(FLOW_SENSOR_PIN, pulseCounter1, FALLING);
    }
}

void getTemperatures()
{
    sensors1.requestTemperatures();
    sensors2.requestTemperatures();

    float temp1 = sensors1.getTempC(insideThermometer1);
    float temp2 = sensors2.getTempC(insideThermometer2);
    if (temp1 == DEVICE_DISCONNECTED_C && temp2 == DEVICE_DISCONNECTED_C)
    {
        return;
    }

    float temp = temp1 > temp2 ? temp1 : temp2;
    if (abs(currentTemperature - temp) > 0.5f)
    {
        Log::Info("temp: %f, %f", temp1, temp2);
        currentTemperature = temp;

        Mqtt::publish(Mqtt::getStatTopic(F("temp")), String(currentTemperature).c_str(), globalConfig.mqtt.retain);
    }
    if (currentTemperature >= 40.0f && bitRead(((Relay *)module)->lastState, RELAY_CH)) // 大于40°停机
    {
        Log::Info(PSTR("temp stop"));
        weileStopTime = millis();
        ((Relay *)module)->switchRelay(RELAY_CH, false, true);
    }
}

void weileInit()
{
    sensors1.begin();
    sensors2.begin();

    if (!sensors1.getAddress(insideThermometer1, 0))
    {
        Log::Info(PSTR("Unable to find address for Device 0"));
    }

    if (!sensors2.getAddress(insideThermometer2, 0))
    {
        Log::Info(PSTR("Unable to find address for Device 1"));
    }

    sensors1.setResolution(insideThermometer1, 9);
    sensors2.setResolution(insideThermometer2, 9);

    pinMode(FLOW_SENSOR_PIN, INPUT);
    attachInterrupt(FLOW_SENSOR_PIN, pulseCounter1, FALLING);

    weileLastState = bitRead(((Relay *)module)->lastState, RELAY_CH);
}

void weileLoop()
{
    if (weileLastState != bitRead(((Relay *)module)->lastState, RELAY_CH)) // 继电器状态变更
    {
        weileLastState = bitRead(((Relay *)module)->lastState, RELAY_CH);
        if (weileLastState)
        {
            weileRunTime = millis();
        }
        else
        {
            weileRunTime = 0;
        }
    }

    if (weileRunTime > 0 && millis() - weileRunTime > 180 * 1000) // 开机3分钟后关机
    {
        Log::Info(PSTR("weileRunTime timeout stop"));
        weileStopTime = millis();
        ((Relay *)module)->switchRelay(RELAY_CH, false, true);
    }

    checkFlow();
}

bool callModule_WeiLe(uint8_t function)
{
    switch (function)
    {
    case FUNC_INIT:
        weileInit();
        break;
    case FUNC_EVERY_SECOND:
        if (perSecond % 6 == 0) // 6秒读取一次温度
        {

            getTemperatures();
        }
        break;
    case FUNC_LOOP:
        weileLoop();
        break;
    }
    return false;
}
#endif