#include <Arduino.h>
#include "Framework.h"
#include "Relay.h"

#ifdef USE_HOMEKIT
#include "homekit.h"
#endif

#ifdef USE_WEILE
#include "WeiLe.hpp"
#endif
#ifdef USE_ZIGBEE
#include "ZigBee.hpp"
#endif

void setup()
{
    Framework::one(115200);

    module = new Relay();

#ifdef USE_WEILE
    addModule(callModule_WeiLe);
#endif
#ifdef USE_ZIGBEE
    addModule(callModule_ZigBee);
#endif

    Framework::setup();

#ifdef USE_HOMEKIT
    homekit_init();
#endif
}

void loop()
{
#ifdef USE_HOMEKIT
    homekit_loop();
#else
    Framework::loop();
#endif
}