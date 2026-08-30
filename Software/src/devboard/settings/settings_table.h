#ifndef BE_DEVBOARD_SETTINGS_SETTINGS_TABLE_H
#define BE_DEVBOARD_SETTINGS_SETTINGS_TABLE_H

#include <stddef.h>
#include <stdint.h>

// One row per persisted setting: the single place a setting's NVS key, storage type, default
// and range are stated. Today nothing reads this table - it is data, added ahead of the
// consumers that replace the hand-written copies in comm_nvm.cpp, settings_html.cpp and
// webserver.cpp.

// SettingKind encodes the ON-FLASH NVS TYPE, not the in-memory type. NVS entries carry a
// type tag (u32=0x04, i32=0x14, u8=0x01) and Preferences enforces it: getInt() on a key
// stored via putUInt() FAILS and returns the default. A uniform "Int" kind calling one
// NVS function for everything would silently reset every existing device's settings to
// defaults after the refactor. Kind picks the matching put/get call; the pipeline value
// is int32_t regardless (safe: the largest value any row can hold is 400000; the table
// test asserts max <= INT32_MAX on U32 rows). Enums are U32 rows (stored via putUInt
// today) - the cast back to the enum type belongs to the setter that lands with the
// consumer, not here.
enum class SettingKind : uint8_t {
  BoolU8,
  U32,
  I32,
  Str,
};

enum SettingFlags : uint8_t {
  SF_NONE = 0,
  SF_SKIP_IF_ZERO = 1,     // stored 0 means "never stored"; the loader leaves the destination alone
  SF_REBOOT_REQUIRED = 2,  // written only by /saveSettings, applied on the next boot
  SF_SECRET = 4,           // never rendered back into HTML
};

struct SettingDesc {
  const char* nvs_key;      // <= 15 chars (NVS limit), unique across the table - both asserted below
  const char* placeholder;  // %VAR% in the HTML templates; nullptr when the key is not rendered by name
  SettingKind kind;
  SettingFlags flags;
  union {
    int32_t i;
    const char* s;
  } def;        // selected by kind: .s for Str rows, .i for every other kind
  int32_t min;  // Str rows: unused, 0
  int32_t max;  // Str rows: unused, 0
};

// The rows. ONE list generates both the Sid enum and the table, so an added setting cannot
// appear in one and not the other (the events.h precedent).
//
// def is the BOOT default - what comm_nvm.cpp passes as the fallback today. Where that
// fallback is a runtime value rather than a literal (the Tesla GTW globals, the charge and
// discharge current limits) the literal here is that variable's own initializer, and
// settings_table_tests.cpp asserts the two still agree.
//
// min/max describe the STORED value, which is not always the value the UI shows: the SOC
// window, the current and voltage limits and the BMS reset duration are all scaled between
// the two (see the scaling notes in the row comments). Rows carrying SF_SKIP_IF_ZERO include
// 0 in their range - for those keys 0 is the "never stored" sentinel, not a magnitude.
//
// placeholder is filled only where the template's %VAR% carries the key's own name, which is
// the case for 127 of the rows and is checked mechanically against settings_html.cpp. The
// remaining rows render through a differently-named placeholder (the SOC window, the current
// and voltage limits and the BMS reset duration all render the live datalayer value under
// names like %SOC_MAX_PERCENTAGE%), or are not on the settings page at all. Those mappings
// arrive with the render consumer, which is where a completeness test can prove them.
//
// clang-format off
#define SETTINGS_TABLE_ROWS(S_INT, S_STR)                                                                              \
  /* --- Network / WiFi --- */                                                                                         \
  S_STR(SSID,           "SSID",           "SSID",           SF_REBOOT_REQUIRED,             "")                        \
  S_STR(PASSWORD,       "PASSWORD",       "PASSWORD",       SF_REBOOT_REQUIRED | SF_SECRET, "")                        \
  S_STR(HTTPUSER,       "HTTPUSER",       "HTTPUSER",       SF_REBOOT_REQUIRED,             "admin")                   \
  S_STR(HTTPPASS,       "HTTPPASS",       "HTTPPASS",       SF_REBOOT_REQUIRED | SF_SECRET, "")                        \
  S_INT(WEBAUTH,        "WEBAUTH",        "WEBAUTH",        BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_STR(HOSTNAME,       "HOSTNAME",       "HOSTNAME",       SF_REBOOT_REQUIRED,             "")                        \
  /* The one bool the codebase already boots true - and it renders correctly only because the  */                      \
  /* settings page reads the live variable instead of the key (settings_html.cpp:627).         */                      \
  S_INT(WIFIAPENABLED,  "WIFIAPENABLED",  "WIFIAPENABLED",  BoolU8, SF_REBOOT_REQUIRED,             1, 0, 1)           \
  S_INT(WIFICHANNEL,    "WIFICHANNEL",    "WIFICHANNEL",    U32,    SF_REBOOT_REQUIRED,             0, 0, 14)          \
  /* Default is wifi.cpp:22's DEFAULT_AP_PASSWORD, which is a runtime const char* and so       */                      \
  /* cannot be named in a constant expression; the literal is duplicated here knowingly.       */                      \
  S_STR(APPASSWORD,     "APPASSWORD",     "APPASSWORD",     SF_REBOOT_REQUIRED | SF_SECRET, "123456789")               \
  S_INT(ESPNOWENABLED,  "ESPNOWENABLED",  "ESPNOWENABLED",  BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_STR(ESPNOWMACS,     "ESPNOWMACS",     "ESPNOWMACS",     SF_REBOOT_REQUIRED,             "")                        \
  S_INT(STATICIP,       "STATICIP",       "STATICIP",       BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_STR(LOCALIP,        "LOCALIP",        "LOCALIP",        SF_REBOOT_REQUIRED,             "")                        \
  S_STR(GATEWAY,        "GATEWAY",        "GATEWAY",        SF_REBOOT_REQUIRED,             "")                        \
  S_STR(SUBNET,         "SUBNET",         "SUBNET",         SF_REBOOT_REQUIRED,             "")                        \
  S_STR(DNS,            "DNS",            "DNS",            SF_REBOOT_REQUIRED,             "")                        \
                                                                                                                       \
  /* --- MQTT --- */                                                                                                   \
  S_INT(MQTTENABLED,    "MQTTENABLED",    "MQTTENABLED",    BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_STR(MQTTSERVER,     "MQTTSERVER",     "MQTTSERVER",     SF_REBOOT_REQUIRED,             "")                        \
  S_INT(MQTTPORT,       "MQTTPORT",       "MQTTPORT",       U32,    SF_REBOOT_REQUIRED,          1883, 1, 65535)       \
  S_STR(MQTTUSER,       "MQTTUSER",       "MQTTUSER",       SF_REBOOT_REQUIRED,             "")                        \
  S_STR(MQTTPASSWORD,   "MQTTPASSWORD",   "MQTTPASSWORD",   SF_REBOOT_REQUIRED | SF_SECRET, "")                        \
  S_INT(MQTTTIMEOUT,    "MQTTTIMEOUT",    "MQTTTIMEOUT",    U32,    SF_REBOOT_REQUIRED,          2000, 1, 60000)       \
  /* Stored in ms, entered in seconds (webserver.cpp multiplies by 1000 on save).             */                       \
  S_INT(MQTTPUBLISHMS,  "MQTTPUBLISHMS",  "MQTTPUBLISHMS",  U32,    SF_REBOOT_REQUIRED,          5000, 1000, 300000)   \
  S_INT(MQTTCELLV,      "MQTTCELLV",      "MQTTCELLV",      BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(MQTTHEAP,       "MQTTHEAP",       "MQTTHEAP",       BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(HADISC,         "HADISC",         "HADISC",         BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_STR(HADISCTOPIC,    "HADISCTOPIC",    "HADISCTOPIC",    SF_REBOOT_REQUIRED,             "homeassistant")           \
  S_INT(HADISCFWU,      "HADISCFWU",      "HADISCFWU",      BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  /* Not user-editable: mqtt.cpp stores the firmware signature it last published for.         */                       \
  S_INT(HADISCFW,       "HADISCFW",       nullptr,          U32,    SF_NONE,                        0, 0, INT32_MAX)   \
  S_INT(REMBMSRESET,    "REMBMSRESET",    "REMBMSRESET",    BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
                                                                                                                       \
  /* --- Battery selection --- */                                                                                      \
  /* Enum rows: the upper bound is the enum's own Highest, which lives in a driver header this */                      \
  /* table deliberately does not include. The cast and its bound belong to the setter that     */                      \
  /* arrives with the consumer PR, so the range stays open here.                               */                      \
  S_INT(BATTTYPE,       "BATTTYPE",       "BATTTYPE",       U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(BATTCHEM,       "BATTCHEM",       "BATTCHEM",       U32,    SF_REBOOT_REQUIRED,             1, 0, INT32_MAX)   \
  S_INT(BATTCOMM,       "BATTCOMM",       "BATTCOMM",       U32,    SF_REBOOT_REQUIRED,             3, 0, INT32_MAX)   \
  S_INT(BATT2COMM,      "BATT2COMM",      "BATT2COMM",      U32,    SF_REBOOT_REQUIRED,             3, 0, INT32_MAX)   \
  S_INT(BATT3COMM,      "BATT3COMM",      "BATT3COMM",      U32,    SF_REBOOT_REQUIRED,             3, 0, INT32_MAX)   \
  S_INT(SHUNTTYPE,      "SHUNTTYPE",      "SHUNTTYPE",      U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(SHUNTCOMM,      "SHUNTCOMM",      "SHUNTCOMM",      U32,    SF_REBOOT_REQUIRED,             3, 0, INT32_MAX)   \
  S_INT(DBLBTR,         "DBLBTR",         "DBLBTR",         BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(TRIBTR,         "TRIBTR",         "TRIBTR",         BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
                                                                                                                       \
  /* --- Battery limits (live-writable; stored units differ from the UI's) --- */                                      \
  S_INT(BATTERY_WH_MAX, "BATTERY_WH_MAX", "BATTERY_WH_MAX", U32,    SF_SKIP_IF_ZERO,                0, 0, 400000)      \
  /* Stored as whole percent, held as pptt: loader multiplies by 10, save divides by 10.       */                      \
  S_INT(MAXPERCENTAGE,  "MAXPERCENTAGE",  nullptr,          U32,    SF_SKIP_IF_ZERO,                0, 0, 100)         \
  /* Same x10, and signed - the loader's guard is the stored range below, wider than the UI's. */                      \
  S_INT(MINPERCENTAGE,  "MINPERCENTAGE",  nullptr,          I32,    SF_NONE,                        0, -100, 500)      \
  /* Stored in deciamps; the UI's 0..1000 is amps.                                             */                      \
  S_INT(MAXCHARGEAMP,   "MAXCHARGEAMP",   nullptr,          U32,    SF_NONE,                      300, 0, 10000)       \
  S_INT(MAXDISCHARGEAMP, "MAXDISCHARGEAMP", nullptr,        U32,    SF_NONE,                      300, 0, 10000)       \
  S_INT(USE_SCALED_SOC, "USE_SCALED_SOC", nullptr,          BoolU8, SF_NONE,                        0, 0, 1)           \
  S_INT(USEVOLTLIMITS,  "USEVOLTLIMITS",  nullptr,          BoolU8, SF_NONE,                        0, 0, 1)           \
  /* Stored in decivolts; the UI's 0..1000 is volts.                                           */                      \
  S_INT(TARGETCHVOLT,   "TARGETCHVOLT",   nullptr,          U32,    SF_SKIP_IF_ZERO,                0, 0, 10000)       \
  S_INT(TARGETDISCHVOLT, "TARGETDISCHVOLT", nullptr,        U32,    SF_SKIP_IF_ZERO,                0, 0, 10000)       \
  /* Stored in ms; the UI's 1..59 is seconds.                                                  */                      \
  S_INT(BMSRESETDUR,    "BMSRESETDUR",    nullptr,          U32,    SF_SKIP_IF_ZERO,                0, 0, 59000)       \
  /* Stored x10 (decivolts) from a float the page submits.                                     */                      \
  S_INT(BATTPVMAX,      "BATTPVMAX",      "BATTPVMAX",      U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(BATTPVMIN,      "BATTPVMIN",      "BATTPVMIN",      U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(BATTCVMAX,      "BATTCVMAX",      "BATTCVMAX",      U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(BATTCVMIN,      "BATTCVMIN",      "BATTCVMIN",      U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
                                                                                                                       \
  /* --- Inverter --- */                                                                                               \
  S_INT(INVTYPE,        "INVTYPE",        "INVTYPE",        U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(INVCOMM,        "INVCOMM",        "INVCOMM",        U32,    SF_REBOOT_REQUIRED,             3, 0, INT32_MAX)   \
  S_INT(LOWPASSFILTER,  "LOWPASSFILTER",  "LOWPASSFILTER",  BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(SLOWCANINV,     "SLOWCANINV",     "SLOWCANINV",     BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(INVOFFGRID,     "INVOFFGRID",     "INVOFFGRID",     BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(CHGTAPERSOC,    "CHGTAPERSOC",    "CHGTAPERSOC",    BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  /* Stored as the start SOC in whole percent; the loader turns it into a band: 10000 - v*100. */                      \
  S_INT(CHGTAPERSTART,  "CHGTAPERSTART",  "CHGTAPERSTART",  U32,    SF_REBOOT_REQUIRED,            95, 50, 100)        \
  S_INT(CHGTAPERFLOOR,  "CHGTAPERFLOOR",  "CHGTAPERFLOOR",  U32,    SF_REBOOT_REQUIRED,           400, 0, 2000)        \
  /* Loader guards with < 16 rather than a range check.                                        */                      \
  S_INT(SOFAR_ID,       "SOFAR_ID",       "SOFAR_ID",       U32,    SF_REBOOT_REQUIRED,             0, 0, 15)          \
  S_INT(PYLONSEND,      "PYLONSEND",      "PYLONSEND",      U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(PYLONOFFSET,    "PYLONOFFSET",    "PYLONOFFSET",    BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(PYLONORDER,     "PYLONORDER",     "PYLONORDER",     BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(PYLONBAUD,      "PYLONBAUD",      "PYLONBAUD",      U32,    SF_REBOOT_REQUIRED,           500, 0, INT32_MAX)   \
  S_INT(PYLONBRAND,     "PYLONBRAND",     nullptr,          U32,    SF_REBOOT_REQUIRED,             0, 0, 2)           \
  S_INT(INVCELLS,       "INVCELLS",       "INVCELLS",       U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(INVMODULES,     "INVMODULES",     "INVMODULES",     U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(INVCELLSPER,    "INVCELLSPER",    "INVCELLSPER",    U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(INVVLEVEL,      "INVVLEVEL",      "INVVLEVEL",      U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(INVCAPACITY,    "INVCAPACITY",    "INVCAPACITY",    U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(INVBTYPE,       "INVBTYPE",       "INVBTYPE",       U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(INVSUNTYPE,     "INVSUNTYPE",     nullptr,          U32,    SF_REBOOT_REQUIRED,             0, 0, 6)           \
  S_INT(FOXESSTYPE,     "FOXESSTYPE",     "FOXESSTYPE",     U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(FOXESSSUBTYPE,  "FOXESSSUBTYPE",  "FOXESSSUBTYPE",  U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(FOXESSMODULES,  "FOXESSMODULES",  "FOXESSMODULES",  U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(INVICNT,        "INVICNT",        "INVICNT",        U32,    SF_REBOOT_REQUIRED,             0, 0, 2)           \
  S_INT(DEYEBYD,        "DEYEBYD",        "DEYEBYD",        BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(PRIMOGEN24,     "PRIMOGEN24",     "PRIMOGEN24",     BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
                                                                                                                       \
  /* --- Charger --- */                                                                                                \
  S_INT(CHGTYPE,        "CHGTYPE",        "CHGTYPE",        U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(CHGCOMM,        "CHGCOMM",        "CHGCOMM",        U32,    SF_REBOOT_REQUIRED,             3, 0, INT32_MAX)   \
  /* Absorbed from upstream; every value is upstream's own. CHGSTARQ: loader default 0, the     */     \
  /* POST handler clamps anything above 2 back to 0, and it is applied without a reboot.        */     \
  /* INVACCREB: read with getBool, default false. INVWDTMO is not user-editable at all - the    */     \
  /* inverter declares it over Modbus register 402 - so it has NO %VAR% placeholder, and its    */     \
  /* default and range are BYD-MODBUS.h's own MODBUS_INV_WATCHDOG_DEFAULT_S and MIN/MAX_S.      */     \
  S_INT(CHGSTARQ,       "CHGSTARQ",       "CHGSTARQ",       U32,    SF_NONE,                        0, 0, 2)           \
  S_INT(INVACCREB,      "INVACCREB",      "INVACCREB",      BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(INVWDTMO,       "INVWDTMO",       nullptr,          U32,    SF_NONE,                       60, 5, 3600)        \
  S_INT(CHGPOWER,       "CHGPOWER",       "CHGPOWER",       U32,    SF_REBOOT_REQUIRED,          1000, 0, 65000)       \
  S_INT(DCHGPOWER,      "DCHGPOWER",      "DCHGPOWER",      U32,    SF_REBOOT_REQUIRED,          1000, 0, 65000)       \
                                                                                                                       \
  /* --- Per-driver specials --- */                                                                                    \
  S_INT(INTERLOCKREQ,   "INTERLOCKREQ",   "INTERLOCKREQ",   BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(DALYPWRPCT,     "DALYPWRPCT",     "DALYPWRPCT",     U32,    SF_REBOOT_REQUIRED,            50, 1, 10000)       \
  S_INT(DALYPWRDV,      "DALYPWRDV",      "DALYPWRDV",      U32,    SF_REBOOT_REQUIRED,            50, 1, 10000)       \
  S_INT(DALYDVSTART,    "DALYDVSTART",    "DALYDVSTART",    U32,    SF_REBOOT_REQUIRED,            20, 1, 200)         \
  S_INT(DALYPWRDEG,     "DALYPWRDEG",     "DALYPWRDEG",     U32,    SF_REBOOT_REQUIRED,            60, 1, 10000)       \
  S_INT(DALYPWR0C,      "DALYPWR0C",      "DALYPWR0C",      U32,    SF_REBOOT_REQUIRED,           800, 0, 100000)      \
  S_INT(SOCESTIMATED,   "SOCESTIMATED",   "SOCESTIMATED",   BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(CHGESTIMATED,   "CHGESTIMATED",   "CHGESTIMATED",   BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(DIGITALHVIL,    "DIGITALHVIL",    "DIGITALHVIL",    BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  /* The five Tesla GTW rows default to the driver's own globals (BATTERIES.cpp:513-517), which */                     \
  /* the loader reads as its fallback. The settings page still renders them from the key with a */                     \
  /* 0 default, so a device that has never saved shows 0 where the firmware uses these.         */                     \
  S_INT(GTWCOUNTRY,     "GTWCOUNTRY",     "GTWCOUNTRY",     U32,    SF_REBOOT_REQUIRED,         17477, 0, INT32_MAX)   \
  S_INT(GTWRHD,         "GTWRHD",         "GTWRHD",         BoolU8, SF_REBOOT_REQUIRED,             1, 0, 1)           \
  S_INT(GTWMAPREG,      "GTWMAPREG",      "GTWMAPREG",      U32,    SF_REBOOT_REQUIRED,             2, 0, INT32_MAX)   \
  S_INT(GTWCHASSIS,     "GTWCHASSIS",     "GTWCHASSIS",     U32,    SF_REBOOT_REQUIRED,             2, 0, 3)           \
  S_INT(GTWPACK,        "GTWPACK",        "GTWPACK",        U32,    SF_REBOOT_REQUIRED,             1, 0, 3)           \
                                                                                                                       \
  /* --- BYD Atto 3 auto-calibration (live routes of their own) --- */                                                 \
  S_INT(BYDAUTOCALDRIFT, "BYDAUTOCALDRIFT", nullptr,        U32,    SF_NONE,                        5, 1, 20)          \
  S_INT(BYDAUTOCALEN,   "BYDAUTOCALEN",   nullptr,          BoolU8, SF_NONE,                        1, 0, 1)           \
  S_INT(BYDAUTOCALDRFT2, "BYDAUTOCALDRFT2", nullptr,        U32,    SF_NONE,                        5, 1, 20)          \
  S_INT(BYDAUTOCALEN2,  "BYDAUTOCALEN2",  nullptr,          BoolU8, SF_NONE,                        1, 0, 1)           \
  S_INT(BYDKEEPISOOFF,  "BYDKEEPISOOFF",  nullptr,          BoolU8, SF_NONE,                        1, 0, 1)           \
                                                                                                                       \
  /* --- Contactor / precharge / hardware --- */                                                                       \
  S_INT(EQSTOP,         "EQSTOP",         "EQSTOP",         U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  /* Not user-editable on the settings page: written by the equipment stop route.               */                     \
  S_INT(EQUIPMENT_STOP, "EQUIPMENT_STOP", nullptr,          BoolU8, SF_NONE,                        0, 0, 1)           \
  S_INT(CNTCTRL,        "CNTCTRL",        "CNTCTRL",        BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(NCCONTACTOR,    "NCCONTACTOR",    "NCCONTACTOR",    BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(PRECHGMS,       "PRECHGMS",       "PRECHGMS",       U32,    SF_REBOOT_REQUIRED,           100, 1, 65000)       \
  S_INT(CNTCTRLDBL,     "CNTCTRLDBL",     "CNTCTRLDBL",     BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(CNTCTRLTRI,     "CNTCTRLTRI",     "CNTCTRLTRI",     BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(PWMCNTCTRL,     "PWMCNTCTRL",     "PWMCNTCTRL",     BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  /* The page's min/max ride on a text input, so the browser does not enforce them.             */                     \
  S_INT(PWMFREQ,        "PWMFREQ",        "PWMFREQ",        U32,    SF_REBOOT_REQUIRED,         20000, 1, 65000)       \
  S_INT(PWMHOLD,        "PWMHOLD",        "PWMHOLD",        U32,    SF_REBOOT_REQUIRED,           250, 1, 1023)        \
  S_INT(PERBMSRESET,    "PERBMSRESET",    "PERBMSRESET",    BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  /* Loader coerces anything that is not 24 or 48 back to 24.                                   */                     \
  S_INT(PERBMSRESETH,   "PERBMSRESETH",   "PERBMSRESETH",   U32,    SF_REBOOT_REQUIRED,            24, 24, 48)         \
  S_INT(PERBMSDEFSOC,   "PERBMSDEFSOC",   "PERBMSDEFSOC",   BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(PERBMSSKIPBAL,  "PERBMSSKIPBAL",  "PERBMSSKIPBAL",  BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(EXTPRECHARGE,   "EXTPRECHARGE",   "EXTPRECHARGE",   BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(NOINVDISC,      "NOINVDISC",      "NOINVDISC",      BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(MAXPRETIME,     "MAXPRETIME",     "MAXPRETIME",     U32,    SF_REBOOT_REQUIRED,         15000, 0, INT32_MAX)   \
  S_INT(MAXPREFREQ,     "MAXPREFREQ",     "MAXPREFREQ",     U32,    SF_REBOOT_REQUIRED,         34000, 0, INT32_MAX)   \
  S_INT(MEASURECPUTEMP, "MEASURECPUTEMP", "MEASURECPUTEMP", BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(CPUTEMPOFFSET,  "CPUTEMPOFFSET",  "CPUTEMPOFFSET",  I32,    SF_REBOOT_REQUIRED,             0, INT32_MIN, INT32_MAX) \
  /* Boots via getUInt(key, false) today - a bool literal as the uint default. It evaluates to  */                     \
  /* 0 and every page passes 0, so the honest default below changes nothing.                    */                     \
  S_INT(LEDMODE,        "LEDMODE",        "LEDMODE",        U32,    SF_REBOOT_REQUIRED,             0, 0, 5)           \
                                                                                                                       \
  /* --- GPIO option pins (each gated to one board by #ifdef in the loader; rows are not) --- */                       \
  S_INT(GPIOOPT1,       "GPIOOPT1",       "GPIOOPT1",       U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(GPIOOPT2,       "GPIOOPT2",       "GPIOOPT2",       U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(GPIOOPT3,       "GPIOOPT3",       "GPIOOPT3",       U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(GPIOOPT4,       "GPIOOPT4",       "GPIOOPT4",       U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(GPIOOPT5,       "GPIOOPT5",       "GPIOOPT5",       U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
  S_INT(GPIOOPT6,       "GPIOOPT6",       "GPIOOPT6",       U32,    SF_REBOOT_REQUIRED,             0, 0, INT32_MAX)   \
                                                                                                                       \
  /* --- Logging / syslog --- */                                                                                       \
  S_INT(PERFPROFILE,    "PERFPROFILE",    "PERFPROFILE",    BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(CANLOGUSB,      "CANLOGUSB",      "CANLOGUSB",      BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(USBENABLED,     "USBENABLED",     "USBENABLED",     BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(WEBENABLED,     "WEBENABLED",     "WEBENABLED",     BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(CANLOGSD,       "CANLOGSD",       "CANLOGSD",       BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(SDLOGENABLED,   "SDLOGENABLED",   "SDLOGENABLED",   BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_INT(SYSLOGEN,       "SYSLOGEN",       "SYSLOGEN",       BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)           \
  S_STR(SYSLOGIP,       "SYSLOGIP",       "SYSLOGIP",       SF_REBOOT_REQUIRED,             "")                        \
  S_INT(SYSLOGPORT,     "SYSLOGPORT",     "SYSLOGPORT",     U32,    SF_REBOOT_REQUIRED,           514, 1, 65535)       \
  S_INT(SYSLOGFAC,      "SYSLOGFAC",      "SYSLOGFAC",      U32,    SF_REBOOT_REQUIRED,             1, 0, 23)          \
                                                                                                                       \
  /* --- CT clamp (shunt) --- */                                                                                       \
  /* Stored as a string so it can hold a negative: -1 means "auto".                             */                     \
  S_STR(CTOFFSET,       "CTOFFSET",       "CTOFFSET",       SF_REBOOT_REQUIRED,             "-1.0")                    \
  S_INT(CTVNOM,         "CTVNOM",         "CTVNOM",         U32,    SF_REBOOT_REQUIRED,            40, 0, 500)         \
  S_INT(CTANOM,         "CTANOM",         "CTANOM",         U32,    SF_REBOOT_REQUIRED,           100, 0, 200)         \
  S_INT(CTATTEN,        "CTATTEN",        "CTATTEN",        U32,    SF_REBOOT_REQUIRED,             3, 0, 3)           \
  S_INT(CTINVERT,       "CTINVERT",       "CTINVERT",       BoolU8, SF_REBOOT_REQUIRED,             0, 0, 1)
// clang-format on

// Sid indexes the table directly - both come from the one list above, so a row cannot exist
// without an id or an id without a row.
enum class Sid : size_t {
#define SETTINGS_SID_ROW(sid, ...) sid,
  SETTINGS_TABLE_ROWS(SETTINGS_SID_ROW, SETTINGS_SID_ROW)
#undef SETTINGS_SID_ROW
      Count,
};

inline constexpr size_t SID_COUNT = static_cast<size_t>(Sid::Count);

#define SETTINGS_ROW_INT(sid, key, ph, kind, flags, def, mn, mx) \
  SettingDesc{key, ph, SettingKind::kind, static_cast<SettingFlags>(flags), {.i = (def)}, mn, mx},
#define SETTINGS_ROW_STR(sid, key, ph, flags, def) \
  SettingDesc{key, ph, SettingKind::Str, static_cast<SettingFlags>(flags), {.s = (def)}, 0, 0},

inline constexpr SettingDesc SETTINGS_TABLE[SID_COUNT] = {SETTINGS_TABLE_ROWS(SETTINGS_ROW_INT, SETTINGS_ROW_STR)};

#undef SETTINGS_ROW_INT
#undef SETTINGS_ROW_STR

constexpr size_t setting_key_len(const char* s) {
  size_t n = 0;
  while (s[n] != '\0') {
    ++n;
  }
  return n;
}

constexpr bool setting_key_equal(const char* a, const char* b) {
  size_t i = 0;
  while (a[i] != '\0' && a[i] == b[i]) {
    ++i;
  }
  return a[i] == b[i];
}

// Whole-table validation, evaluated by the compiler. An inverted range, an out-of-range
// default, a key over the NVS ceiling and a duplicate key or placeholder are build failures
// rather than things a device discovers.
//
// Reading the union is what keeps this honest: touching def.s on a non-Str row is not a
// constant expression, so a row whose kind and default disagree fails to compile here.
constexpr bool table_valid() {
  for (size_t a = 0; a < SID_COUNT; ++a) {
    if (SETTINGS_TABLE[a].nvs_key == nullptr) {
      return false;
    }
    if (setting_key_len(SETTINGS_TABLE[a].nvs_key) > 15) {  // NVS physical key limit
      return false;
    }
    if (SETTINGS_TABLE[a].min > SETTINGS_TABLE[a].max) {
      return false;
    }
    if (SETTINGS_TABLE[a].kind == SettingKind::Str) {
      if (SETTINGS_TABLE[a].def.s == nullptr) {
        return false;
      }
    } else {
      if (SETTINGS_TABLE[a].def.i < SETTINGS_TABLE[a].min || SETTINGS_TABLE[a].def.i > SETTINGS_TABLE[a].max) {
        return false;
      }
    }
    for (size_t b = a + 1; b < SID_COUNT; ++b) {
      if (setting_key_equal(SETTINGS_TABLE[a].nvs_key, SETTINGS_TABLE[b].nvs_key)) {
        return false;
      }
      if (SETTINGS_TABLE[a].placeholder != nullptr && SETTINGS_TABLE[b].placeholder != nullptr &&
          setting_key_equal(SETTINGS_TABLE[a].placeholder, SETTINGS_TABLE[b].placeholder)) {
        return false;
      }
    }
  }
  return true;
}

static_assert(table_valid(), "settings table inconsistent - see table_valid() for the rules it checks");

// Row lookup by id. The table itself is constexpr and reachable directly; this exists so a
// consumer can take a row without spelling the cast, and so the table has one owning TU.
const SettingDesc& setting_desc(Sid sid);

#endif  // BE_DEVBOARD_SETTINGS_SETTINGS_TABLE_H
