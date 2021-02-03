
#ifdef USE_ZIGBEE
#include "Module.h"
#include "Util.h"
#include "Zigbee2Mqtt.h"
#include <HardwareSerial.h>

const uint32_t SERIAL_POLLING = 50; // Serial receive polling in ms
const uint8_t BUFFER_SIZE = 256;
WiFiServer *ser2netServer = nullptr;
WiFiClient *ser2netClient = nullptr;
unsigned long ser_polling_window = 0;
unsigned long net_polling_window = 0;
uint8_t ser_buffer[BUFFER_SIZE]; // Receive buffer
uint8_t net_buffer[BUFFER_SIZE];
int ser_buffer_count = 0;
int net_buffer_count = 0;

void resetZigbee()
{
    digitalWrite(5, LOW);
    delay(500);
    digitalWrite(5, HIGH);
}

void serialBridgeCreateTCPServer(int port)
{
    if (port > 0)
    {
        Serial1.begin(115200, SERIAL_8N1, 32, 33);
        ser2netServer = new WiFiServer(port);
        ser2netServer->begin();
        ser2netServer->setNoDelay(true);
        Log::Info(PSTR("Starting TCP Server Port: %d"), port);
    }
    else
    {
        ser2netServer->stop();
        delete ser2netServer;
        ser2netServer = nullptr;
        Log::Info(PSTR("Stop TCP Server"));
    }

    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);
    resetZigbee();
}

void serialBridgeOutput(void)
{
    if (ser2netClient && !ser2netClient->connected())
    {
        ser2netClient->stop();
        delete ser2netClient;
        ser2netClient = nullptr;
        Log::Info(PSTR("Client disconnected"));
    }
    if (ser2netServer && ser2netServer->hasClient())
    {
        if (ser2netClient && !ser2netClient->connected())
        {
            ser2netClient->stop();
            delete ser2netClient;
            ser2netClient = nullptr;
            Log::Info(PSTR("Client disconnected"));
        }
        ser2netClient = new WiFiClient;
        *ser2netClient = ser2netServer->available();
        Log::Info(PSTR("New client for COM"));
        resetZigbee();
    }
    if (!ser2netClient)
    {
        return;
    }
    while (ser2netClient && ser2netClient->connected() && ser2netClient->available())
    {
        yield();
        net_buffer[net_buffer_count++] = ser2netClient->read();

        if (net_buffer_count >= BUFFER_SIZE)
        {
            net_polling_window = 0; // Publish now
            break;
        }
        else
        {
            net_polling_window = millis(); // Wait for more data
        }
    }

    if (net_buffer_count && millis() - net_polling_window > SERIAL_POLLING)
    {
        Serial1.write(net_buffer, net_buffer_count);
        char strHex[BUFFER_SIZE * 3 + 1];
        Util::hex2Str(net_buffer, net_buffer_count, strHex, true);
        Log::Info(PSTR("TX(%d): %s"), net_buffer_count, strHex);
        net_buffer_count = 0;
    }
}

void serialBridgeInput(void)
{
    while (Serial1.available())
    {
        yield();
        ser_buffer[ser_buffer_count++] = Serial1.read();

        if (ser_buffer_count >= BUFFER_SIZE)
        {
            ser_polling_window = 0; // Publish now
            break;
        }
        else
        {
            ser_polling_window = millis(); // Wait for more data
        }
    }

    if (ser_buffer_count && millis() - ser_polling_window > SERIAL_POLLING)
    {
        if (ser2netClient)
        {
            ser2netClient->write(ser_buffer, ser_buffer_count);
            char strHex[BUFFER_SIZE * 3 + 1];
            Util::hex2Str(ser_buffer, ser_buffer_count, strHex, true);
            Log::Info(PSTR("RX(%d): %s"), ser_buffer_count, strHex);
        }
        ser_buffer_count = 0;
    }
}

void loopTask2(void *pvParameters)
{
    const char mouids[] PROGMEM = "1_ting_pwm|1_fan_pwm|1_s_pwm|2_ting_pwm|2_fan_pwm|2_m_pwm|2_s_pwm|2_b_pwm|3_ting_pwm|3_shu_pwm|3_m_pwm|3_s_pwm|3_b_pwm";
    char command[30];
    uint64_t x = GetCommandCode(command, sizeof(command), UID, mouids);
    if (x >= 0)
    {
        x += 16;
        Settings.zb_pan_id = x << 8 | x;
        Settings.zb_ext_panid = x << 56 | x << 48 | x << 40 | x << 32 | x << 24 | x << 16 | x << 8 | x;
        Settings.zb_precfgkey_l = x << 56 | x << 48 | x << 40 | x << 32 | x << 24 | x << 16 | x << 8 | x;
        Settings.zb_precfgkey_h = x << 56 | x << 48 | x << 40 | x << 32 | x << 24 | x << 16 | x << 8 | x;
        Settings.zb_channel = 15;
        Settings.zb_txradio_dbm = USE_ZIGBEE_TXRADIO_DBM;
        Settings.remove_zbreceived = true;
    }
    else
    {
    }

    sprintf(Settings.topic, PSTR("z2m/%s"), UID);
    Xdrv23(FUNC_PRE_INIT);
    for (;;)
    {
        uint32_t my_sleep = millis();
        Xdrv23(FUNC_LOOP);
        delay(1);
    }
    /*
    serialBridgeCreateTCPServer(8880);
    for (;;)
    {
        if (ser2netServer)
        {
            serialBridgeOutput();
            serialBridgeInput();
        }
        delay(1);
    }
    */
}

void zigBeeInit()
{
#ifdef DUSE_DIMMING
    if (((Relay *)module)->config.module_type == CH1_PWM1)
#else
    if (1 == 1)
#endif
    {
        xTaskCreatePinnedToCore(loopTask2, "loopTask2", 8192, NULL, 2, NULL, CONFIG_ARDUINO_RUNNING_CORE == 1 ? 0 : 1);
    }
    else
    {
        Log::Error(PSTR("no support ZigBee IO"));
    }
}

bool callModule_ZigBee(uint8_t function)
{
    switch (function)
    {
    case FUNC_INIT:
        zigBeeInit();
        break;
    case FUNC_WEB:
        Xdrv23(FUNC_WEB);
        break;
    }
    return false;
}
#endif
