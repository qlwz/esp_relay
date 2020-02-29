#include "Relay.h"
#include "RadioReceive.h"
#include "Rtc.h"

#pragma region 继承

String Relay::getModuleCNName()
{
    return String(channels) + F("路开关模块");
}

void Relay::init()
{
    if (config.led_light == 0)
    {
        config.led_light = 100;
    }
    if (config.led_time == 0)
    {
        config.led_time = 2;
    }
    ledLight = config.led_light * 10 + 23;

    loadModule(config.module_type);
    if (GPIO_PIN[GPIO_LED_POWER] != 99)
    {
        Led::init(GPIO_PIN[GPIO_LED_POWER], HIGH);
    }
    else if (GPIO_PIN[GPIO_LED_POWER_INV] != 99)
    {
        Led::init(GPIO_PIN[GPIO_LED_POWER_INV], LOW);
    }

#ifdef USE_RCSWITCH
    if (GPIO_PIN[GPIO_RFRECV] != 99)
    {
        radioReceive = new RadioReceive();
        radioReceive->init(this, GPIO_PIN[GPIO_RFRECV]);
    }
#endif

    channels = 0;
    for (uint8_t ch = 0; ch < 4; ch++)
    {
        if (GPIO_PIN[GPIO_REL1 + ch] == 99)
        {
            continue;
        }
        channels++;

        pinMode(GPIO_PIN[GPIO_REL1 + ch], OUTPUT); // 继电器
        if (GPIO_PIN[GPIO_LED1 + ch] != 99)
        {
            pinMode(GPIO_PIN[GPIO_LED1 + ch], OUTPUT); // LED
        }
    }

    strcpy(powerStatTopic, Mqtt::getStatTopic(F("POWER1")).c_str());

    for (uint8_t ch = 0; ch < channels; ch++)
    {
        if (GPIO_PIN[GPIO_KEY1 + ch] != 99)
        {
            pinMode(GPIO_PIN[GPIO_KEY1 + ch], INPUT_PULLUP);
            if (digitalRead(GPIO_PIN[GPIO_KEY1 + ch]))
            {
                buttonStateFlag[ch] |= DEBOUNCED_STATE | UNSTABLE_STATE;
            }
        }

        // 0:开关通电时断开  1:开关通电时闭合  2:开关通电时状态与断电前相反  3:开关通电时保持断电前状态
        if (config.power_on_state == 2)
        {
            switchRelay(ch, !bitRead(config.last_state, ch), false); // 开关通电时状态与断电前相反
        }
        else if (config.power_on_state == 3)
        {
            switchRelay(ch, bitRead(config.last_state, ch), false); // 开关通电时保持断电前状态
        }
        else
        {
            switchRelay(ch, config.power_on_state == 1, false); // 开关通电时闭合
        }
    }

    checkCanLed(true);
}

bool Relay::moduleLed()
{
#ifdef USE_RCSWITCH
    if (radioReceive && radioReceive->studyCH > 0)
    {
        Led::on();
        return true;
    }
#endif

    if (WiFi.status() == WL_CONNECTED && Mqtt::mqttClient.connected())
    {
        if (config.led == 0)
        {
            Led::on();
            return true;
        }
        else if (config.led == 1)
        {
            Led::off();
            return true;
        }
    }
    return false;
}

void Relay::loop()
{
    for (size_t ch = 0; ch < channels; ch++)
    {
        cheackButton(ch);
    }

#ifdef USE_RCSWITCH
    if (radioReceive)
    {
        radioReceive->loop();
    }
#endif

    if (bitRead(operationFlag, 0))
    {
        bitClear(operationFlag, 0);
        if (perSecond % 60 == 0)
        {
            checkCanLed();
        }
        if (config.report_interval > 0 && (perSecond % config.report_interval) == 0)
        {
            reportPower();
        }
    }
}

void Relay::perSecondDo()
{
    bitSet(operationFlag, 0);
}
#pragma endregion

#pragma region 配置

void Relay::readConfig()
{
    Config::moduleReadConfig(MODULE_CFG_VERSION, sizeof(RelayConfigMessage), RelayConfigMessage_fields, &config);
}

void Relay::resetConfig()
{
    Debug::AddInfo(PSTR("moduleResetConfig . . . OK"));
    memset(&config, 0, sizeof(RelayConfigMessage));
    config.module_type = SupportedModules::CH3;
    config.led_light = 50;
    config.led_time = 3;
}

void Relay::saveConfig(bool isEverySecond)
{
    Config::moduleSaveConfig(MODULE_CFG_VERSION, RelayConfigMessage_size, RelayConfigMessage_fields, &config);
}
#pragma endregion

#pragma region MQTT

void Relay::mqttCallback(String topicStr, String str)
{
    if (channels >= 1 && topicStr.endsWith("/POWER1"))
    {
        switchRelay(0, (str == "ON" ? true : (str == "OFF" ? false : !bitRead(lastState, 0))));
    }
    else if (channels >= 2 && topicStr.endsWith("/POWER2"))
    {
        switchRelay(1, (str == "ON" ? true : (str == "OFF" ? false : !bitRead(lastState, 1))));
    }
    else if (channels >= 3 && topicStr.endsWith("/POWER3"))
    {
        switchRelay(2, (str == "ON" ? true : (str == "OFF" ? false : !bitRead(lastState, 2))));
    }
    else if (channels >= 4 && topicStr.endsWith("/POWER4"))
    {
        switchRelay(3, (str == "ON" ? true : (str == "OFF" ? false : !bitRead(lastState, 3))));
    }
    else if (topicStr.endsWith("/report"))
    {
        reportPower();
    }
}

void Relay::mqttConnected()
{
    strcpy(powerStatTopic, Mqtt::getStatTopic(F("POWER1")).c_str());
    if (globalConfig.mqtt.discovery)
    {
        mqttDiscovery(true);
    }
    reportPower();
}

void Relay::mqttDiscovery(bool isEnable)
{
    char topic[50];
    char message[500];

    String availability = Mqtt::getTeleTopic(F("availability"));
    char cmndTopic[80];
    strcpy(cmndTopic, Mqtt::getCmndTopic(F("POWER1")).c_str());
    for (size_t ch = 0; ch < channels; ch++)
    {
        sprintf(topic, "%s/light/%s_l%d/config", globalConfig.mqtt.discovery_prefix, UID, (ch + 1));
        if (isEnable)
        {
            cmndTopic[strlen(cmndTopic) - 1] = ch + 49;           // 48 + 1 + ch
            powerStatTopic[strlen(powerStatTopic) - 1] = ch + 49; // 48 + 1 + ch
            sprintf(message, HASS_DISCOVER_RELAY, UID, (ch + 1),
                    cmndTopic,
                    powerStatTopic,
                    availability.c_str());
            //Debug::AddInfo(PSTR("discovery: %s - %s"), topic, message);
            Mqtt::publish(topic, message, true);
        }
        else
        {
            Mqtt::publish(topic, "", true);
        }
    }
    if (isEnable)
    {
        Mqtt::availability();
    }
}
#pragma endregion

#pragma region Http

void Relay::httpAdd(ESP8266WebServer *server)
{
    server->on(F("/relay_do"), std::bind(&Relay::httpDo, this, server));
    server->on(F("/relay_setting"), std::bind(&Relay::httpSetting, this, server));
    server->on(F("/ha"), std::bind(&Relay::httpHa, this, server));
#ifdef USE_RCSWITCH
    server->on(F("/rf_do"), std::bind(&Relay::httpRadioReceive, this, server));
#endif
#ifdef USE_TRICOLOR
    server->on(F("/downlight_setting"), std::bind(&Relay::httpDownlightSetting, this, server));
#endif
}

String Relay::httpGetStatus(ESP8266WebServer *server)
{
    String data;
    for (size_t ch = 0; ch < channels; ch++)
    {
        data += ",\"relay_" + String(ch + 1) + "\":";
        data += bitRead(lastState, ch) ? 1 : 0;
    }
    return data.substring(1);
}

void Relay::httpHtml(ESP8266WebServer *server)
{
    String radioJs = F("<script type='text/javascript'>");
    radioJs += F("function setDataSub(data,key){if(key.substr(0,5)=='relay'){var t=id(key);var v=data[key];t.setAttribute('class',v==1?'btn-success':'btn-info');t.innerHTML=v==1?'开':'关';return true}return false}");
    String page = F("<table class='gridtable'><thead><tr><th colspan='2'>开关状态</th></tr></thead><tbody>");
    page += F("<tr colspan='2' style='text-align:center'><td>");
    for (size_t ch = 0; ch < channels; ch++)
    {
        page += F(" <button type='button' style='width:50px' onclick=\"ajaxPost('/relay_do', 'do=T&c={ch}');\" id='relay_{ch}' ");
        page.replace(F("{ch}"), String(ch + 1));
        if (bitRead(lastState, ch))
        {
            page += F("class='btn-success'>开</button>");
        }
        else
        {
            page += F("class='btn-info'>关</button>");
        }
    }
    page += F("</td></tr></tbody></table>");

    page += F("<form method='post' action='/relay_setting' onsubmit='postform(this);return false'>");
    page += F("<table class='gridtable'><thead><tr><th colspan='2'>开关设置</th></tr></thead><tbody>");
    if (SupportedModules::END > 1)
    {
        page += F("<tr><td>模块类型</td><td>");
        page += F("<select id='module_type' name='module_type' style='width:150px'>");
        for (int count = 0; count < SupportedModules::END; count++)
        {
            page += F("<option value='");
            page += String(count);
            page += F("'>");
            page += Modules[count].name;
            page += F("</option>");
        }
        page += F("</select></td></tr>");
        radioJs += F("id('module_type').value={v};");
        radioJs.replace(F("{v}"), String(config.module_type));
    }
    page += F("<tr><td>上电状态</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='0'/><i class='bui-radios'></i> 开关通电时断开</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='1'/><i class='bui-radios'></i> 开关通电时闭合</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='2'/><i class='bui-radios'></i> 开关通电时状态与断电前相反</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='3'/><i class='bui-radios'></i> 开关通电时保持断电前状态</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('power_on_state', '{v}');");
    radioJs.replace(F("{v}"), String(config.power_on_state));

    page += F("<tr><td>开关模式</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_mode' value='0'/><i class='bui-radios'></i> 自锁</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_mode' value='1'/><i class='bui-radios'></i> 互锁</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('power_mode', '{v}');");
    radioJs.replace(F("{v}"), String(config.power_mode));

    if (GPIO_PIN[GPIO_LED1] != 99)
    {
        page += F("<tr><td>面板指示灯</td><td>");
        page += F("<label class='bui-radios-label'><input type='radio' name='led_type' value='0'/><i class='bui-radios'></i> 无</label>&nbsp;&nbsp;&nbsp;&nbsp;");
        page += F("<label class='bui-radios-label'><input type='radio' name='led_type' value='1'/><i class='bui-radios'></i> 普通</label>&nbsp;&nbsp;&nbsp;&nbsp;");
        page += F("<label class='bui-radios-label'><input type='radio' name='led_type' value='2'/><i class='bui-radios'></i> 呼吸灯</label>&nbsp;&nbsp;&nbsp;&nbsp;");
        //page += F("<label class='bui-radios-label'><input type='radio' name='led_type' value='3'/><i class='bui-radios'></i> WS2812</label>");
        page += F("</td></tr>");
        radioJs += F("setRadioValue('led_type', '{v}');");
        radioJs.replace(F("{v}"), String(config.led_type));

        page += F("<tr><td>指示灯亮度</td><td><input type='range' min='1' max='100' name='led_light' value='{led_light}' onchange='ledLightRangOnChange(this)'/>&nbsp;<span>{led_light}%</span></td></tr>");
        page.replace("{led_light}", String(config.led_light));
        radioJs += F("function ledLightRangOnChange(the){the.nextSibling.nextSibling.innerHTML=the.value+'%'};");

        page += F("<tr><td>渐变时间</td><td><input type='number' name='relay_led_time' value='{v}'>毫秒</td></tr>");
        page.replace(F("{v}"), String(config.led_time));

        String tmp = "";
        for (uint8_t i = 0; i <= 23; i++)
        {
            tmp += F("<option value='{v1}'>{v}:00</option>");
            tmp += F("<option value='{v2}'>{v}:30</option>");
            tmp.replace(F("{v1}"), String(i * 100));
            tmp.replace(F("{v2}"), String(i * 100 + 30));
            tmp.replace(F("{v}"), i < 10 ? "0" + String(i) : String(i));
        }

        page += F("<tr><td>指示灯时间段</td><td>");
        page += F("<select id='led_start' name='led_start'>");
        page += tmp;
        page += F("</select>");
        page += F("&nbsp;&nbsp;到&nbsp;&nbsp;");
        page += F("<select id='led_end' name='led_end'>");
        page += tmp;
        page += F("</select>");

        radioJs += F("id('led_start').value={v1};");
        radioJs += F("id('led_end').value={v2};");
        radioJs.replace(F("{v1}"), String(config.led_start));
        radioJs.replace(F("{v2}"), String(config.led_end));
        page += F("</td></tr>");
    }
    if (GPIO_PIN[GPIO_LED_POWER] != 99 || GPIO_PIN[GPIO_LED_POWER_INV] != 99)
    {
        page += F("<tr><td>LED</td><td>");
        page += F("<label class='bui-radios-label'><input type='radio' name='led' value='0'/><i class='bui-radios'></i> 常亮</label>&nbsp;&nbsp;&nbsp;&nbsp;");
        page += F("<label class='bui-radios-label'><input type='radio' name='led' value='1'/><i class='bui-radios'></i> 常灭</label>&nbsp;&nbsp;&nbsp;&nbsp;");
        page += F("<label class='bui-radios-label'><input type='radio' name='led' value='2'/><i class='bui-radios'></i> 闪烁</label><br>未连接WIFI或者MQTT时为快闪");
        page += F("</td></tr>");
        radioJs += F("setRadioValue('led', '{v}');");
        radioJs.replace(F("{v}"), String(config.led));
    }

    page += F("<tr><td>主动上报间隔</td><td><input type='number' min='0' max='3600' name='report_interval' required value='{v}'>&nbsp;秒，0关闭</td></tr>");
    page.replace(F("{v}"), String(config.report_interval));

    page += F("<tr><td colspan='2'><button type='submit' class='btn-info'>设置</button><br>");
    page += F("<button type='button' class='btn-success' style='margin-top:10px' onclick='window.location.href=\"/ha\"'>下载HA配置文件</button></td></tr>");
    page += F("</tbody></table></form>");

#ifdef USE_TRICOLOR
    page += F("<form method='post' action='/downlight_setting' onsubmit='postform(this);return false'>");
    page += F("<table class='gridtable'><thead><tr><th colspan='2'>三色筒灯</th></tr></thead><tbody>");
    page += F("<tr><td>指定路数</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='downlight_ch' value='0'/><i class='bui-radios'></i> 无</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    for (size_t ch = 0; ch < channels; ch++)
    {
        page += F("<label class='bui-radios-label'><input type='radio' name='downlight_ch' value='{ch}'/><i class='bui-radios'></i> {ch}路</label>&nbsp;&nbsp;&nbsp;&nbsp;");
        page.replace(F("{ch}"), String(ch + 1));
    }
    page += F("</td></tr>");
    radioJs += F("setRadioValue('downlight_ch', '{v}');");
    radioJs.replace(F("{v}"), String(config.downlight_ch));

    String tmp = F("<option value='1'>白光</option><option value='2'>黄光</option><option value='3'>黄白光</option>");

    page += F("<tr><td>三色排序</td><td>");
    page += F("<select id='color1' name='color1'>");
    page += tmp;
    page += F("</select>&nbsp;&nbsp;");
    page += F("<select id='color2' name='color2'>");
    page += tmp;
    page += F("</select>&nbsp;&nbsp;");
    page += F("<select id='color3' name='color3'>");
    page += tmp;
    page += F("</select>");
    page += F("</td></tr>");
    radioJs += F("id('color1').value={v1};");
    radioJs += F("id('color2').value={v2};");
    radioJs += F("id('color3').value={v3};");
    radioJs.replace(F("{v1}"), String(config.downlight_color[0]));
    radioJs.replace(F("{v2}"), String(config.downlight_color[1]));
    radioJs.replace(F("{v3}"), String(config.downlight_color[2]));

    page += F("<tr><td>默认颜色</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='default' value='1'/><i class='bui-radios'></i> 白光</label>&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='default' value='2'/><i class='bui-radios'></i> 黄光</label>&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='default' value='3'/><i class='bui-radios'></i> 黄白光</label>&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='default' value='4'/><i class='bui-radios'></i> 记忆</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('default', '{v1}');");
    radioJs.replace(F("{v1}"), String(config.downlight_default));

    page += F("<tr><td colspan='2'><button type='submit' class='btn-info'>设置</button></td></tr>");
    page += F("</tbody></table></form>");
#endif

#ifdef USE_RCSWITCH
    if (radioReceive)
    {
        page += F("<table class='gridtable'><thead><tr><th colspan='2'>射频管理</th></tr></thead><tbody>");
        page += F("<tr><td>学习模式</td><td>");
        for (size_t ch = 0; ch < channels; ch++)
        {
            page += F(" <button type='button' style='width:60px' onclick=\"ajaxPost('/rf_do', 'do=s&c={ch}')\" class='btn-success'>{ch}路</button>");
            page.replace(F("{ch}"), String(ch + 1));
        }
        page += F("</td></tr>");

        page += F("<tr><td>删除模式</td><td>");
        for (size_t ch = 0; ch < channels; ch++)
        {
            page += F(" <button type='button' style='width:60px' onclick=\"ajaxPost('/rf_do', 'do=d&c={ch}')\" class='btn-info'>{ch}路</button>");
            page.replace(F("{ch}"), String(ch + 1));
        }
        page += F("</td></tr>");

        page += F("<tr><td>全部删除</td><td>");
        for (size_t ch = 0; ch < channels; ch++)
        {
            page += F(" <button type='button' style='width:60px' onclick=\"javascript:if(confirm('确定要清空射频遥控？')){ajaxPost('/rf_do', 'do=c&c={ch}');}\" class='btn-danger'>{ch}路</button>");
            page.replace(F("{ch}"), String(ch + 1));
        }
        page += F(" <button type='button' style='width:50px' onclick=\"javascript:if(confirm('确定要清空全部射频遥控？')){ajaxPost('/rf_do', 'do=c&c=0');}\" class='btn-danger'>全部</button>");
        page += F("</td></tr>");
        page += F("</tbody></table>");
    }
#endif

    radioJs += F("</script>");

    server->sendContent(page);
    server->sendContent(radioJs);
}

void Relay::httpDo(ESP8266WebServer *server)
{
    String c = server->arg(F("c"));
    if (c != F("1") && c != F("2") && c != F("3") && c != F("4"))
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"参数错误。\"}"));
        return;
    }
    uint8_t ch = c.toInt() - 1;
    if (ch > channels)
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"继电器数量错误。\"}"));
        return;
    }
    String str = server->arg(F("do"));
    switchRelay(ch, (str == "ON" ? true : (str == "OFF" ? false : !bitRead(lastState, ch))));

    server->send(200, F("text/html"), "{\"code\":1,\"msg\":\"操作成功\",\"data\":{" + httpGetStatus(server) + "}}");
}

#ifdef USE_RCSWITCH
void Relay::httpRadioReceive(ESP8266WebServer *server)
{
    if (!radioReceive)
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"没有射频模块。\"}"));
        return;
    }
    String d = server->arg(F("do"));
    String c = server->arg(F("c"));
    if ((d != F("s") && d != F("d") && d != F("c")) || (c != F("0") && (c.toInt() < 1 || c.toInt() > channels)))
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"参数错误。\"}"));
        return;
    }
    if (radioReceive->studyCH != 0)
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"上一个操作未完成\"}"));
        return;
    }
    if (d == F("s"))
    {
        radioReceive->study(c.toInt() - 1);
    }
    else if (d == F("d"))
    {
        radioReceive->del(c.toInt() - 1);
    }
    else if (d == F("c"))
    {
        if (c == F("0"))
        {
            radioReceive->delAll();
        }
        else
        {
            config.study_index[c.toInt() - 1] = 0;
        }
    }
    Config::saveConfig();
    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"操作成功\"}"));
}
#endif

#ifdef USE_TRICOLOR
void Relay::httpDownlightSetting(ESP8266WebServer *server)
{
    String color1 = server->arg(F("color1"));
    String color2 = server->arg(F("color2"));
    String color3 = server->arg(F("color3"));
    if (color1 == color2 || color1 == color3 || color2 == color3)
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"三色排序存在相同\"}"));
        return;
    }

    config.downlight_ch = server->arg(F("downlight_ch")).toInt();
    config.downlight_default = server->arg(F("default")).toInt();
    config.downlight_color[0] = color1.toInt();
    config.downlight_color[1] = color2.toInt();
    config.downlight_color[2] = color3.toInt();
    Config::saveConfig();
    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已经设置成功。\"}"));
}
#endif

void Relay::httpSetting(ESP8266WebServer *server)
{
    config.power_on_state = server->arg(F("power_on_state")).toInt();
    config.report_interval = server->arg(F("report_interval")).toInt();

    if (server->hasArg(F("led")))
    {
        config.led = server->arg(F("led")).toInt();
    }
    if (server->hasArg(F("power_mode")))
    {
        config.power_mode = server->arg(F("power_mode")).toInt();
    }
    if (server->hasArg(F("led_type")))
    {
        config.led_type = server->arg(F("led_type")).toInt();
    }

    if (server->hasArg(F("led_start")) && server->hasArg(F("led_end")))
    {
        config.led_start = server->arg(F("led_start")).toInt();
        config.led_end = server->arg(F("led_end")).toInt();
    }

    if (server->hasArg(F("led_light")))
    {
        config.led_light = server->arg(F("led_light")).toInt();
        ledLight = config.led_light * 10 + 23;
    }
    if (server->hasArg(F("relay_led_time")))
    {
        config.led_time = server->arg(F("relay_led_time")).toInt();
        if (config.led_type == 2 && ledTicker.active())
        {
            ledTicker.detach();
        }
    }
    checkCanLed(true);

    if (server->hasArg(F("module_type")) && !server->arg(F("module_type")).equals(String(config.module_type)))
    {
        server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已经更换模块类型 . . . 正在重启中。\"}"));
        config.module_type = server->arg(F("module_type")).toInt();
        Config::saveConfig();
        Led::blinkLED(400, 4);
        ESP.restart();
    }
    else
    {
        Config::saveConfig();
        server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已经设置成功。\"}"));
    }
}

void Relay::httpHa(ESP8266WebServer *server)
{
    char attachment[100];
    snprintf_P(attachment, sizeof(attachment), PSTR("attachment; filename=%s.yaml"), UID);

    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->sendHeader(F("Content-Disposition"), attachment);
    server->send(200, F("Content-Type: application/octet-stream"), "");

    String availability = Mqtt::getTeleTopic(F("availability"));
    char cmndTopic[100];
    strcpy(cmndTopic, Mqtt::getCmndTopic(F("POWER1")).c_str());
    server->sendContent(F("light:\r\n"));
    for (size_t ch = 0; ch < channels; ch++)
    {
        cmndTopic[strlen(cmndTopic) - 1] = ch + 49;           // 48 + 1 + ch
        powerStatTopic[strlen(powerStatTopic) - 1] = ch + 49; // 48 + 1 + ch
        server->sendContent(F("  - platform: mqtt\r\n    name: \""));
        server->sendContent(UID);
        server->sendContent(F("_l"));
        server->sendContent(String((ch + 1)));
        server->sendContent(F("\"\r\n    state_topic: \""));
        server->sendContent(powerStatTopic);
        server->sendContent(F("\"\r\n    command_topic: \""));
        server->sendContent(cmndTopic);
        server->sendContent(F("\"\r\n    payload_on: \"ON\"\r\n    payload_off: \"OFF\"\r\n    availability_topic: \""));
        server->sendContent(availability);
        server->sendContent(F("\"\r\n    payload_available: \"online\"\r\n    payload_not_available: \"offline\"\r\n\r\n"));
    }
}
#pragma endregion

#pragma region Led

void Relay::ledTickerHandle()
{
    for (uint8_t ch = 0; ch < channels; ch++)
    {
        if (!bitRead(lastState, ch) && GPIO_PIN[GPIO_LED1 + ch] != 99)
        {
            analogWrite(GPIO_PIN[GPIO_LED1 + ch], ledLevel);
        }
    }
    if (ledUp)
    {
        ledLevel++;
        if (ledLevel >= ledLight)
        {
            ledUp = false;
        }
    }
    else
    {
        ledLevel--;
        if (ledLevel <= 50)
        {
            ledUp = true;
        }
    }
}

void Relay::ledPWM(uint8_t ch, bool isOn)
{
    if (isOn)
    {
        analogWrite(GPIO_PIN[GPIO_LED1 + ch], 0);
        if (ledTicker.active())
        {
            for (uint8_t ch2 = 0; ch2 < channels; ch2++)
            {
                if (!bitRead(lastState, ch2))
                {
                    return;
                }
            }
            ledTicker.detach();
            Debug::AddInfo(PSTR("ledTicker detach"));
        }
    }
    else
    {
        if (!ledTicker.active())
        {
            ledTicker.attach_ms(config.led_time, std::bind(&Relay::ledTickerHandle, this));
            Debug::AddInfo(PSTR("ledTicker active"));
        }
    }
}

void Relay::led(uint8_t ch, bool isOn)
{
    if (config.led_type == 0 || GPIO_PIN[GPIO_LED1 + ch] == 99)
    {
        return;
    }

    if (config.led_type == 1)
    {
        //digitalWrite(GPIO_PIN[GPIO_LED1 + ch], isOn ? LOW : HIGH);
        analogWrite(GPIO_PIN[GPIO_LED1 + ch], isOn ? 0 : ledLight);
    }
    else if (config.led_type == 2)
    {
        ledPWM(ch, isOn);
    }
}

bool Relay::checkCanLed(bool re)
{
    bool result;
    if (config.led_start != config.led_end && Rtc::rtcTime.valid)
    {
        uint16_t nowTime = Rtc::rtcTime.hour * 100 + Rtc::rtcTime.minute;
        if (config.led_start > config.led_end) // 开始时间大于结束时间 跨日
        {
            result = (nowTime >= config.led_start || nowTime < config.led_end);
        }
        else
        {
            result = (nowTime >= config.led_start && nowTime < config.led_end);
        }
    }
    else
    {
        result = true; // 没有正确时间为一直亮
    }
    if (result != Relay::canLed || re)
    {
        if ((!result || config.led_type != 2) && ledTicker.active())
        {
            ledTicker.detach();
            Debug::AddInfo(PSTR("ledTicker detach2"));
        }
        Relay::canLed = result;
        Debug::AddInfo(result ? PSTR("led can light") : PSTR("led can not light"));
        for (uint8_t ch = 0; ch < channels; ch++)
        {
            if (GPIO_PIN[GPIO_LED1 + ch] != 99)
            {
                result &&config.led_type != 0 ? led(ch, bitRead(lastState, ch)) : analogWrite(GPIO_PIN[GPIO_LED1 + ch], 0);
            }
        }
    }

    return result;
}
#pragma endregion

void Relay::switchRelay(uint8_t ch, bool isOn, bool isSave)
{
    if (ch > channels)
    {
        Debug::AddInfo(PSTR("invalid channel: %d"), ch);
        return;
    }
    Debug::AddInfo(PSTR("Relay %d . . . %s"), ch + 1, isOn ? "ON" : "OFF");

    if (isOn && config.power_mode == 1)
    {
        for (size_t ch2 = 0; ch2 < channels; ch2++)
        {
            if (ch2 != ch && bitRead(lastState, ch2))
            {
                switchRelay(ch2, false, isSave);
            }
        }
    }

#ifdef USE_TRICOLOR
    colorOnOff(ch, isOn);
#endif

    bitWrite(lastState, ch, isOn);
    digitalWrite(GPIO_PIN[GPIO_REL1 + ch], isOn ? HIGH : LOW);

    powerStatTopic[strlen(powerStatTopic) - 1] = ch + 49; // 48 + 1 + ch
    Mqtt::publish(powerStatTopic, isOn ? "ON" : "OFF", globalConfig.mqtt.retain);

    if (isSave && config.power_on_state > 0)
    {
        bitWrite(config.last_state, ch, isOn);
        Config::delaySaveConfig(10);
    }
    if (Relay::canLed)
    {
        led(ch, isOn);
    }
}

void Relay::cheackButton(uint8_t ch)
{
    if (GPIO_PIN[GPIO_KEY1 + ch] == 99)
    {
        return;
    }
    bool currentState = digitalRead(GPIO_PIN[GPIO_KEY1 + ch]);
    if (currentState != ((buttonStateFlag[ch] & UNSTABLE_STATE) != 0))
    {
        buttonTimingStart[ch] = millis();
        buttonStateFlag[ch] ^= UNSTABLE_STATE;
    }
    else if (millis() - buttonTimingStart[ch] >= buttonDebounceTime)
    {
        if (currentState != ((buttonStateFlag[ch] & DEBOUNCED_STATE) != 0))
        {
            buttonTimingStart[ch] = millis();
            buttonStateFlag[ch] ^= DEBOUNCED_STATE;

            switchCount[ch] += 1;
            buttonIntervalStart[ch] = millis();

            if (millis() > lastTime[ch] + 300)
            {
                switchRelay(ch, !bitRead(lastState, ch), true);
                lastTime[ch] = millis();
            }
        }
    }

    // 如果经过的时间大于超时并且计数大于0，则填充并重置计数
    if (switchCount[ch] > 0 && (millis() - buttonIntervalStart[ch]) > specialFunctionTimeout)
    {
        Led::led(200);
        Debug::AddInfo(PSTR("switchCount %d : %d"), ch + 1, switchCount[ch]);

#ifdef USE_RCSWITCH
        if (switchCount[ch] == 10 && radioReceive)
        {
            radioReceive->study(ch);
        }
        else if (switchCount[ch] == 12 && radioReceive)
        {
            radioReceive->del(ch);
        }
        else if (switchCount[ch] == 16 && radioReceive)
        {
            radioReceive->delAll();
        }
#endif
        if (switchCount[ch] == 20)
        {
            Wifi::setupWifiManager(false);
        }
        switchCount[ch] = 0;
    }
}

void Relay::loadModule(uint8_t module)
{
    for (uint16_t i = 0; i < sizeof(GPIO_PIN); i++)
    {
        GPIO_PIN[i] = 99;
    }

    mytmplt m = Modules[module];
    uint8_t j = 0;
    for (uint8_t i = 0; i < sizeof(m.io); i++)
    {
        if (6 == i)
        {
            j = 9;
        }
        if (8 == i)
        {
            j = 12;
        }
        GPIO_PIN[m.io[i]] = j;
        j++;
    }
}

void Relay::reportPower()
{
    for (size_t ch = 0; ch < channels; ch++)
    {
        powerStatTopic[strlen(powerStatTopic) - 1] = ch + 49; // 48 + 1 + ch
        Mqtt::publish(powerStatTopic, bitRead(lastState, ch) ? "ON" : "OFF", globalConfig.mqtt.retain);
    }
}

#ifdef USE_TRICOLOR
void Relay::colorOnOff(uint8_t ch, bool isOn)
{
    if (config.downlight_ch == ch + 1 && bitRead(lastState, ch) != isOn) // 三色筒灯
    {
        if (!isOn)
        {
            colorOffTime = millis();
            return;
        }

        if (millis() > colorOffTime + (2 * 1000)) // 关灯超过5秒
        {
            if (config.downlight_default == 0)
            {
                config.downlight_index = 0;
            }
            else
            {
                uint8_t def = config.downlight_default == 4 ? config.downlight_color[config.downlight_index] : config.downlight_default;
                for (size_t i = 0; i < 3; i++)
                {
                    if (def == config.downlight_color[i])
                    {
                        config.downlight_index = i;
                        break;
                    }
                    digitalWrite(GPIO_PIN[GPIO_REL1 + ch], HIGH);
                    delay(105);
                    digitalWrite(GPIO_PIN[GPIO_REL1 + ch], LOW);
                    delay(105);
                }
            }
        }
        else
        {
            config.downlight_index = (config.downlight_index + 1) % 3;
        }
        Debug::AddInfo(PSTR("cur color: %d"), config.downlight_index);
    }
}
#endif