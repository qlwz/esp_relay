// Config.h

#ifndef _CONFIG_h
#define _CONFIG_h

#include <Ticker.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb.h>
#include "Arduino.h"
#include "GlobalConfig.pb.h"
#include "../../../customize.ini"

#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



extern char UID[16];
extern char tmpData[512];
extern GlobalConfigMessage globalConfig;
extern uint32_t perSecond;
extern Ticker *tickerPerSecond;

class Config
{
private:
    static uint16_t nowCrc;
    static bool isDelay;
    static uint8_t countdown;

public:
    static uint16_t crc16(uint8_t *ptr, uint16_t len);

    static void readConfig();
    static void resetConfig();
    static bool saveConfig(bool isEverySecond = false);
    static void delaySaveConfig(uint8_t second);

    static void moduleReadConfig(uint16_t version, uint16_t size, const pb_field_t fields[], void *dest_struct);
    static bool moduleSaveConfig(uint16_t version, uint16_t size, const pb_field_t fields[], const void *src_struct);

    static void perSecondDo();
};
#endif
