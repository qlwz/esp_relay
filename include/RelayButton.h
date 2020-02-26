// RelayButton.h
#ifndef _RELAYBUTTON_h
#define _RELAYBUTTON_h

#include "Arduino.h"

static const uint8_t DEBOUNCED_STATE = 0b00000001;
static const uint8_t UNSTABLE_STATE = 0b00000010;
//static const uint8_t CHANGED_STATE = 0b00000100;

class RelayButton
{
protected:
    uint8_t ch = -1;
    uint8_t io = -1;
    // 按键
    int debounceTime = 50;
    unsigned long timingStart = 0;
    unsigned long intervalStart;
    int switchCount = 0;
    bool currentState;

    // 等待开关再次切换的时间（以毫秒为单位）。
    // 300对我来说效果很好，几乎没有引起注意。 如果您不想使用此功能，请设置为0。
    unsigned long specialFunctionTimeout = 300;
    int lastTime = 0;

    Relay *relay;

    uint8_t stateFlag;
    inline void setStateFlag(const uint8_t flag) { stateFlag |= flag; }
    inline void unsetStateFlag(const uint8_t flag) { stateFlag &= ~flag; }
    inline void toggleStateFlag(const uint8_t flag) { stateFlag ^= flag; }
    inline bool getStateFlag(const uint8_t flag) { return ((stateFlag & flag) != 0); }

public:
    void init(Relay *_relay, uint8_t _ch, uint8_t _io);
    void loop();
};

#endif
