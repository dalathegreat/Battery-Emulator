#include <string>

extern const char* version_number;  // The current software version, shown on webserver
extern std::string http_username;
extern std::string http_password;
extern bool webserver_auth;
extern bool ota_active;
extern bool settingsUpdated;

static constexpr const char* WEB_AUTH_REALM = "Battery Emulator";

void init_webserver();
bool webserver_auth_is_ready();
void webserver_tick();
