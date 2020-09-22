#include <Arduino.h>
#include "Framework.h"
#include "Relay.h"

#ifdef USE_HOMEKIT
#include "homekit.h"
#endif

void setup()
{
    Framework::one(115200);

    module = new Relay();

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