#include "ethernet.h"

#ifdef ETHERNET

#include "../hal/hal.h"
#include "../network/hostname.h"        // active_hostname() (shared across interfaces)
#include "../network/network_status.h"  // network_bring_services_up()
#include "../utils/events.h"
#include "../utils/logging.h"

static bool eth_has_ip = false;

// Static IP configuration
bool eth_static_IP_enabled = false;
IPAddress eth_static_local_IP;
IPAddress eth_static_gateway;
IPAddress eth_static_subnet;
IPAddress eth_static_dns;

// Map project-local ETH_PHY_KIND_* tags (from hal.h) to arduino-esp32's
// eth_phy_type_t. Board HAL headers return the kind tag so they don't have to
// include <ETH.h> — see hw_dfrobot_edge101.h for why that matters.
static eth_phy_type_t phy_type_from_kind(int kind) {
  switch (kind) {
    case ETH_PHY_KIND_IP101:
      return ETH_PHY_IP101;
    default:
      return ETH_PHY_MAX;  // sentinel — ETH.begin() will report failure
  }
}

static eth_clock_mode_t clk_mode_from_kind(int kind) {
  switch (kind) {
    case ETH_CLK_KIND_GPIO0_IN:
      return ETH_CLOCK_GPIO0_IN;
    default:
      return ETH_CLOCK_GPIO0_IN;  // safe default; unreachable for real boards
  }
}

// Single multiplexed handler for the ETH_* subset of arduino-esp32's WiFi event
// dispatcher. Emits our EVENT_ETHERNET_* pair and tracks whether we have an IP.
static void onEthEvent(WiFiEvent_t event, WiFiEventInfo_t /*info*/) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      DEBUG_PRINTF("Ethernet started\n");
      // Hostname must be re-applied after ETH_START; setting it before begin()
      // isn't sufficient because the netif is created here.
      ETH.setHostname(active_hostname().c_str());
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      clear_event(EVENT_ETHERNET_DISCONNECT);
      // Encode link speed + duplex into the event's data
      set_event(EVENT_ETHERNET_CONNECT, eth_encode_link(ETH.linkSpeed(), ETH.fullDuplex()));
      clear_event(EVENT_ETHERNET_CONNECT);
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      eth_has_ip = true;
      network_bring_services_up(ETH.localIP());  // log IP + syslog_start() + init_mDNS()
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      eth_has_ip = false;
      set_event(EVENT_ETHERNET_DISCONNECT, 0);
      break;

    default:
      break;
  }
}

void init_Ethernet() {
  // DEBUG_PRINTF("init_Ethernet: MDC=%d MDIO=%d POWER=%d phy_type=%d phy_addr=%d clk_mode=%d\n",
  //              (int)esp32hal->ETH_PHY_MDC_PIN(), (int)esp32hal->ETH_PHY_MDIO_PIN(), (int)esp32hal->ETH_PHY_POWER_PIN(),
  //              esp32hal->ETH_PHY_TYPE_ID(), esp32hal->ETH_PHY_ADDR_NUM(), esp32hal->ETH_CLK_MODE_ID());

  // Track pins that go through the HAL allocator. The RMII data/clock pins
  // (GPIO 0/21/22/25/26/27 on classic ESP32) are fixed by the EMAC peripheral
  // and can't be reassigned, so they don't need allocator tracking. MDC/MDIO
  // and the PHY power/reset pin are configurable and can conflict with SD/CAN.
  esp32hal->alloc_pins_ignore_unused("Ethernet", esp32hal->ETH_PHY_MDC_PIN(), esp32hal->ETH_PHY_MDIO_PIN(),
                                     esp32hal->ETH_PHY_POWER_PIN());

  // Register handler BEFORE ETH.begin(); events fire as soon as begin() runs.
  WiFi.onEvent(onEthEvent, ARDUINO_EVENT_ETH_START);
  WiFi.onEvent(onEthEvent, ARDUINO_EVENT_ETH_CONNECTED);
  WiFi.onEvent(onEthEvent, ARDUINO_EVENT_ETH_GOT_IP);
  WiFi.onEvent(onEthEvent, ARDUINO_EVENT_ETH_DISCONNECTED);

  const bool ok = ETH.begin(phy_type_from_kind(esp32hal->ETH_PHY_TYPE_ID()), esp32hal->ETH_PHY_ADDR_NUM(),
                            esp32hal->ETH_PHY_MDC_PIN(), esp32hal->ETH_PHY_MDIO_PIN(), esp32hal->ETH_PHY_POWER_PIN(),
                            clk_mode_from_kind(esp32hal->ETH_CLK_MODE_ID()));
  if (!ok) {
    logging.println("Ethernet init failed (ETH.begin returned false).");
    set_event(EVENT_ETHERNET_DISCONNECT, 0);
  } else if (eth_static_IP_enabled) {
    if (eth_static_local_IP != IPAddress() && eth_static_gateway != IPAddress() && eth_static_subnet != IPAddress()) {
      // ETH.config() stops the DHCP client and unconditionally overwrites the DNS server. Passing no DNS
      // therefore leaves the resolver at 0.0.0.0 and breaks MQTT-by-hostname/release checks. Default to
      // the gateway, which is the resolver on virtually every home network.
      IPAddress dns = (eth_static_dns != IPAddress()) ? eth_static_dns : eth_static_gateway;
      if (!ETH.config(eth_static_local_IP, eth_static_gateway, eth_static_subnet, dns)) {
        logging.println("Ethernet static IP configuration rejected, falling back to DHCP");
      }
    } else {
      logging.println("Ethernet static IP settings are invalid, falling back to DHCP");
    }
  }
}

bool ethernet_connected() {
  return eth_has_ip;
}

#endif  // ETHERNET
