#include "WiFi.h"
#include "ESPmDNS.h"

#if defined(_WIN32)
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")  // Link with Ws2_32.lib
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

WiFiClass WiFi;
MDNSResponder MDNS;

char hostname[256];

const char* WiFiClass::getHostname() {

  if (gethostname(hostname, sizeof(hostname)) != 0) {
    return "UnknownHost";
  }

  return hostname;
}

std::string get_local_ip() {
#if defined(_WIN32)
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
    return "127.0.0.1";
  }

  struct hostent* host = gethostbyname(hostname);
  if (!host || host->h_addr_list[0] == nullptr) {
    return "127.0.0.1";
  }

  return inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
#else
  struct ifaddrs* ifaddr;
  if (getifaddrs(&ifaddr) == -1) {
    return "127.0.0.1";
  }

  std::string ip = "127.0.0.1";
  for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
      struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
      char addr[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(sa->sin_addr), addr, INET_ADDRSTRLEN);
      if (std::string(ifa->ifa_name) != "lo") {
        ip = addr;
        break;
      }
    }
  }

  freeifaddrs(ifaddr);
  return ip;
#endif
}

IPAddress WiFiClass::localIP() {
  return IPAddress(get_local_ip());
}
