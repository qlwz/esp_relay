#ifdef USE_DIMMING

#include "Dimming.h"
#include "Relay.h"

struct ROTARY
{
    uint8_t pinA;
    uint8_t pinB;
    unsigned long debounce = 0; // Rotary debounce timer
    uint8_t state = 0;
    int16_t position = 0;
    uint8_t interrupts_in_use = 0;
    uint8_t changed = 0;
} _Rotary;

static DRAM_ATTR ROTARY Rotary;
void Dimming::init(Relay *_relay)
{
    relay = _relay;
    pwmstartch = relay->channels;

    uint8_t pin = 0;
    bool pwm_invert = false;
    for (uint8_t ch = 0; ch < MAX_PWM_NUM; ch++)
    {
        if (PWM_BRIGHTNESS_PIN[ch] == 99)
        {
            break;
        }

        // 亮度
        loadPWM(ch, PWM_BRIGHTNESS_PIN[ch]);

        // 色温
        if (PWM_TEMPERATURE_PIN[ch] != 99)
        {
            loadPWM(MAX_PWM_NUM * 2 - 1 - ch, PWM_TEMPERATURE_PIN[ch]);
        }

        // LED
        relay->channels++;
        if (RELAY_LED_PIN[relay->channels - 1] != 99)
        {
            pinMode(RELAY_LED_PIN[relay->channels - 1], OUTPUT); // LED
        }
    }

#ifdef ESP866
    analogWriteRange(1023);
    analogWriteFreq(1600);
#endif

    RotaryInit();
    if (relay->config.max_pwm < 20)
    {
        relay->config.max_pwm = 100;
    }
    pwm_range = PWM_RANGE * relay->config.max_pwm / 100;

#ifdef WIFI_SSID
    pinMode(23, OUTPUT);
    pinMode(13, OUTPUT);
    pinMode(27, OUTPUT);
#endif
}

void Dimming::loadPWM(uint8_t ch, uint8_t pin)
{
    bool pwm_invert = pin > 50;
    uint8_t pin2 = pwm_invert ? pin - 50 : pin;
    pinMode(pin2, OUTPUT);
#ifdef ESP32
    //19531
    ledcSetup(ch, 19531, 12); // 12 kHz PWM, 8-bit resolutionXS
    pinMatrixOutAttach(pin2, LEDC_HS_SIG_OUT0_IDX + ch, pwm_invert, false);
    pwm_invert = false;
#endif
    digitalWrite(pin2, pwm_invert ? HIGH : LOW);
    current_color[ch] = pwm_invert ? 255 : 0;
}

void Dimming::mqttCallback(char *topic, char *payload, char *cmnd)
{
    if (strlen(cmnd) == 11 && strncmp(cmnd, "brightness", 10) == 0) // strlen("power1") = 6
    {
        uint8_t ch = cmnd[10] - 49;
        if (pwmstartch <= ch)
        {
            uint16_t d = atoi(payload);
            if (d < 1)
                d = 0;
            if (d > 100)
                d = 100;
            relay->config.brightness[ch] = d;
            relay->switchRelay(ch, relay->config.brightness[ch] != 0, true);
        }
    }
    else if (strlen(cmnd) == 11 && strncmp(cmnd, "color_temp", 10) == 0) // strlen("power1") = 6
    {
        uint8_t ch = cmnd[10] - 49;
        if (pwmstartch <= ch)
        {
            uint16_t t = atoi(payload);
            if (t < 153)
                t = 153;
            if (t > 500)
                t = 500;
            relay->config.color_temp[ch] = t;
            relay->switchRelay(ch, relay->config.brightness[ch] != 0, true);
        }
    }
}

void Dimming::switchRelayPWM(uint8_t ch, bool isOn, bool isSave)
{
    uint8_t pwmch = ch - pwmstartch;
    if (!isOn)
    {
        Debug::AddInfo(PSTR("PWMRelay %d . . . %s"), ch + 1, isOn ? "ON" : "OFF");

        target_color[pwmch] = 0;
        if (PWM_TEMPERATURE_PIN[pwmch] != 99)
        {
            target_color[MAX_PWM_NUM * 2 - 1 - pwmch] = 0;
        }
        if (!pwmTicker.active())
        {
            pwmTicker.attach_ms(10, []() { ((Relay *)module)->dimming->animate(); });
        }
        return;
    }

#ifdef WIFI_SSID
    digitalWrite(23, 1);
    digitalWrite(13, 1);
    digitalWrite(27, 1);
#endif

    uint8_t brightness = relay->config.brightness[ch];
    if (brightness == 0)
    {
        brightness = 100;
    }

    if (PWM_TEMPERATURE_PIN[pwmch] != 99)
    {
        /* Color Temperature (https://developers.meethue.com/documentation/core-concepts)
        *
        * ct = 153 = 2000K = Warm = CCWW = 00FF
        * ct = 500 = 6500K = Cold = CCWW = FF00
        */
        uint16_t ct = relay->config.color_temp[ch];
        uint16_t my_ct = ct - 153;
        if (my_ct > 347)
        {
            my_ct = 347;
        }
        float dimmer = 100 / (float)brightness;

        uint16_t icold = (100 * (347 - my_ct)) / 136;
        uint16_t iwarm = (100 * my_ct) / 136;

        target_color[pwmch] = (uint16_t)((float)(icold / dimmer) * (float)(pwm_range / 255));
        target_color[MAX_PWM_NUM * 2 - 1 - pwmch] = (uint16_t)((float)(iwarm / dimmer) * (float)(pwm_range / 255));

        Debug::AddInfo(PSTR("Relay %d . . . Color:%d %d  %d  Brightness:%d %d %d"), ch + 1, ct, icold, target_color[MAX_PWM_NUM * 2 - 1 - pwmch], brightness, iwarm, target_color[pwmch]);
    }
    else
    {
        float dimmer = 100 / (float)brightness;
        target_color[pwmch] = 255 / dimmer * (pwm_range / 255);
        Debug::AddInfo(PSTR("Relay %d %d. . . Brightness:%d %d"), ch + 1, pwmch, brightness, target_color[pwmch]);
    }
    if (!pwmTicker.active())
    {
        pwmTicker.attach_ms(10, []() { ((Relay *)module)->dimming->animate(); });
    }
}

IRAM_ATTR void Dimming::animate(void)
{
    for (uint8_t i = 0; i < (sizeof(current_color) / sizeof(current_color[0])); i++)
    {
        if (current_color[i] == target_color[i])
        {
            continue;
        }

        if (current_color[i] < target_color[i])
        {
            current_color[i] = current_color[i] + PWM_SHIFT > target_color[i] ? target_color[i] : current_color[i] + PWM_SHIFT;
        }
        else if (current_color[i] > target_color[i])
        {
            if (current_color[i] < (target_color[i] == 0 ? PWM_SHIFT * 2 : PWM_SHIFT))
            {
                current_color[i] = 0;
            }
            else
            {
                current_color[i] = current_color[i] - (target_color[i] == 0 ? PWM_SHIFT * 2 : PWM_SHIFT) < target_color[i] ? target_color[i] : current_color[i] - (target_color[i] == 0 ? PWM_SHIFT * 2 : PWM_SHIFT);
            }
        }

        //Debug::AddInfo(PSTR("a%d %d %d"), i, target_color[i], current_color[i]);

#ifdef ESP8266
        if (i < MAX_PWM_NUM)
        {
            analogWrite(PWM_BRIGHTNESS_PIN[i] > 50 ? PWM_BRIGHTNESS_PIN[i] - 50 : PWM_BRIGHTNESS_PIN[i], PWM_BRIGHTNESS_PIN[i] > 50 ? PWM_RANGE - current_color[i] : current_color[i]);
        }
        else
        {
            uint8_t ch = MAX_PWM_NUM * 2 - 1 - i;
            analogWrite(PWM_TEMPERATURE_PIN[ch] > 50 ? PWM_TEMPERATURE_PIN[ch] - 50 : PWM_TEMPERATURE_PIN[ch], PWM_TEMPERATURE_PIN[ch] > 50 ? PWM_RANGE - current_color[i] : current_color[i]);
        }
#else
        ledcWrite(i, current_color[i]);
#endif
    }

    for (uint8_t i = 0; i < (sizeof(current_color) / sizeof(current_color[0])); i++)
    {
        if (current_color[i] != target_color[i])
        {
            return;
        }
    }
    pwmTicker.detach();

#ifdef WIFI_SSID
    for (uint8_t i = 0; i < (sizeof(current_color) / sizeof(current_color[0])); i++)
    {
        if (current_color[i] != 0)
        {
            return;
        }
    }
    digitalWrite(23, 0);
    digitalWrite(13, 0);
    digitalWrite(27, 0);
#endif
}

#pragma region 编码器
IRAM_ATTR void update_rotary(void)
{
    uint8_t state = Rotary.state & 3;
    if (digitalRead(Rotary.pinA))
    {
        state |= 4;
    }
    if (digitalRead(Rotary.pinB))
    {
        state |= 8;
    }
    switch (state)
    {
    case 1:
    case 7:
    case 8:
    case 14:
        Rotary.position++;
        break;
    case 2:
    case 4:
    case 11:
    case 13:
        Rotary.position--;
        break;
    case 3:
    case 12:
        Rotary.position = Rotary.position + 2;
        break;
    case 6:
    case 9:
        Rotary.position = Rotary.position - 2;
        break;
    }
    Rotary.state = (state >> 2);
}

void Dimming::RotaryInit(void)
{
    if (ROT_PIN[0] != 99 && !isFlashInterfacePin(ROT_PIN[0]))
    {
        Rotary.pinA = ROT_PIN[0];
        pinMode(Rotary.pinA, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(Rotary.pinA), update_rotary, CHANGE);
        Rotary.interrupts_in_use++;
    }
    if (ROT_PIN[1] != 99 && !isFlashInterfacePin(ROT_PIN[1]))
    {
        Rotary.pinB = ROT_PIN[1];
        pinMode(Rotary.pinB, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(Rotary.pinB), update_rotary, CHANGE);
        Rotary.interrupts_in_use++;
    }
}

bool Dimming::RotaryButtonPressed(void)
{
    if (Rotary.changed)
    {
        Rotary.changed = 0; // Color temp changed, no need to turn of the light
        return true;
    }
    return false;
}
void Dimming::RotaryHandler(void)
{
    if (Rotary.interrupts_in_use < 2)
    {
        noInterrupts();
        update_rotary();
    }
    else
    {
        noInterrupts();
    }
    if (Rotary.position != 0)
    {
        uint8_t ch = relay->channels - 1;
        bool pressed = !digitalRead(BOTTON_PIN[ch]);
        if (pressed)
        {
            Rotary.changed = 1;
            int16_t t = relay->config.color_temp[ch];
            t = t + Rotary.position;
            if (t < 153)
            {
                t = 153;
            }
            if (t > 500)
            {
                t = 500;
            }
            Debug::AddInfo(PSTR("SetColorTemp:  %d"), Rotary.position);
            relay->config.color_temp[ch] = t;
            relay->switchRelay(ch, relay->config.brightness[ch] != 0, true);
        }
        else
        {
            int16_t d = relay->config.brightness[ch];
            d = d + Rotary.position;
            if (d < 1)
            {
                d = 0;
            }
            if (d > 100)
            {
                d = 100;
            }
            Debug::AddInfo(PSTR("SetBrightness:  %d"), Rotary.position);
            relay->config.brightness[ch] = d;
            relay->switchRelay(ch, relay->config.brightness[ch] != 0, true);
        }
        Rotary.position = 0;
    }
    interrupts();
}

void Dimming::RotaryLoop(void)
{
    if (ROT_PIN[0] != 99)
    {
        if (((int32_t)(millis() - Rotary.debounce)) > 0)
        {
            Rotary.debounce = millis() + BUTTON_DEBOUNCE_TIME;
            RotaryHandler();
        }
    }
}
#pragma endregion

void Dimming::httpSetBrightness(WEB_SERVER_REQUEST)
{
    uint8_t ch = server->arg(F("ch")).toInt() - 1;
    if (ch > relay->channels || pwmstartch > ch)
    {
        server->send_P(200, PSTR("application/json"), PSTR("{\"code\":0,\"msg\":\"不支持PWM\"}"));
        return;
    }
    if (server->hasArg(F("c")))
    {
        int16_t c = server->arg(F("c")).toInt();
        relay->config.color_temp[ch] = c;
        relay->switchRelay(ch, relay->config.brightness[ch] != 0, true);
    }
    if (server->hasArg(F("b")))
    {
        int16_t b = server->arg(F("b")).toInt();
        relay->config.brightness[ch] = b;
        relay->switchRelay(ch, relay->config.brightness[ch] != 0, true);
    }

    snprintf_P(tmpData, sizeof(tmpData), PSTR("{\"code\":1,\"msg\":\"操作成功\",\"data\":{%s}}"), relay->httpGetStatus(server).c_str());
    server->send_P(200, PSTR("application/json"), tmpData);
}

IRAM_ATTR void Dimming::loop()
{
    RotaryLoop();
}

void Dimming::httpHtml(WEB_SERVER_REQUEST)
{
    for (size_t ch = pwmstartch; ch < relay->channels; ch++)
    {
        if (PWM_TEMPERATURE_PIN[ch - pwmstartch] != 99)
        {
            snprintf_P(tmpData, sizeof(tmpData),
                       PSTR("</td></tr><tr><td>色温%d</td><td><input type='range' min='153' max='500' id='color%d' value='%d' onchange='ajaxPost(\"/set_brightness\", \"ch=%d&c=\"+this.value);this.nextSibling.nextSibling.innerHTML=this.value'/>&nbsp;<span>%d</span>"),
                       ch - pwmstartch + 1, ch + 1, relay->config.color_temp[ch], ch + 1, relay->config.color_temp[ch]);
            server->sendContent_P(tmpData);
        }

        snprintf_P(tmpData, sizeof(tmpData),
                   PSTR("</td></tr><tr><td>亮度%d</td><td><input type='range' min='0' max='100' id='brightness%d' value='%d' onchange='ajaxPost(\"/set_brightness\", \"ch=%d&b=\"+this.value);this.nextSibling.nextSibling.innerHTML=this.value+\"%\"'/>&nbsp;<span>%d%</span>"),
                   ch - pwmstartch + 1, ch + 1, relay->config.brightness[ch], ch + 1, relay->config.brightness[ch]);
        server->sendContent_P(tmpData);
    }
}

void Dimming::httpHa(WEB_SERVER_REQUEST, uint8_t ch)
{
    char brightnessCmndTopic[100];
    strcpy(brightnessCmndTopic, Mqtt::getCmndTopic(F("brightness1")).c_str());
    char brightnessStatTopic[100];
    strcpy(brightnessStatTopic, Mqtt::getStatTopic(F("brightness1")).c_str());

    char color_tempCmndTopic[100];
    strcpy(color_tempCmndTopic, Mqtt::getCmndTopic(F("color_temp1")).c_str());
    char color_tempStatTopic[100];
    strcpy(color_tempStatTopic, Mqtt::getStatTopic(F("color_temp1")).c_str());

    // 亮度
    brightnessCmndTopic[strlen(brightnessCmndTopic) - 1] = ch + 49; // 48 + 1 + ch
    brightnessStatTopic[strlen(brightnessStatTopic) - 1] = ch + 49; // 48 + 1 + ch
    snprintf_P(tmpData, sizeof(tmpData),
               PSTR("    brightness_state_topic: \"%s\"\r\n"
                    "    brightness_command_topic: \"%s\"\r\n"
                    "    brightness_scale: 100\r\n"),
               brightnessStatTopic, brightnessCmndTopic);
    server->sendContent_P(tmpData);

    // 色温
    if (PWM_TEMPERATURE_PIN[ch - pwmstartch] != 99)
    {
        color_tempCmndTopic[strlen(color_tempCmndTopic) - 1] = ch + 49; // 48 + 1 + ch
        color_tempStatTopic[strlen(color_tempStatTopic) - 1] = ch + 49; // 48 + 1 + ch
        snprintf_P(tmpData, sizeof(tmpData),
                   PSTR("    color_temp_state_topic: \"%s\"\r\n"
                        "    color_temp_command_topic: \"%s\"\r\n"),
                   color_tempStatTopic, color_tempCmndTopic);
        server->sendContent_P(tmpData);
    }
}

String Dimming::httpGetStatus(WEB_SERVER_REQUEST)
{
    String data;
    for (size_t ch = pwmstartch; ch < relay->channels; ch++)
    {
        if (PWM_TEMPERATURE_PIN[ch - pwmstartch] != 99)
        {
            data += ",\"color" + String(ch + 1) + "\":";
            data += String(relay->config.color_temp[ch]);
        }
        data += ",\"brightness" + String(ch + 1) + "\":";
        data += String(relay->config.brightness[ch]);
    }
    return data.substring(1);
}
#endif