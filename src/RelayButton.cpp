#include "Relay.h"
#include "RadioReceive.h"
#include "RelayButton.h"

void RelayButton::init(Relay *_relay, uint8_t _ch, uint8_t _io)
{
    ch = _ch;
    io = _io;
    relay = _relay;
    pinMode(io, INPUT_PULLUP);
    if (digitalRead(io))
    {
        setStateFlag(DEBOUNCED_STATE | UNSTABLE_STATE);
    }
}

void RelayButton::loop()
{
    currentState = digitalRead(io);
    if (currentState != getStateFlag(UNSTABLE_STATE))
    {
        timingStart = millis();
        toggleStateFlag(UNSTABLE_STATE);
    }
    else if (millis() - timingStart >= debounceTime)
    {
        if (currentState != getStateFlag(DEBOUNCED_STATE))
        {
            timingStart = millis();
            toggleStateFlag(DEBOUNCED_STATE);

            switchCount += 1;
            intervalStart = millis();

            if (millis() > lastTime + 300)
            {
                relay->switchRelay(ch, !bitRead(relay->lastState, ch), true);
                lastTime = millis();
            }
        }
    }

    // 如果经过的时间大于超时并且计数大于0，则填充并重置计数
    if (switchCount > 0 && (millis() - intervalStart) > specialFunctionTimeout)
    {
        Led::led(200);
        Debug::AddInfo(PSTR("switchCount %d : %d"), ch + 1, switchCount);

#ifdef USE_RCSWITCH
        if (switchCount == 10 && relay->radioReceive)
        {
            relay->radioReceive->study(ch);
        }
        else if (switchCount == 12 && relay->radioReceive)
        {
            relay->radioReceive->del(ch);
        }
        else if (switchCount == 16 && relay->radioReceive)
        {
            relay->radioReceive->delAll();
        }
#endif
        if (switchCount == 20)
        {
            Wifi::setupWifiManager(false);
        }
        switchCount = 0;
    }
}
