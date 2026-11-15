#include <gtest/gtest.h>

#include "../Software/src/devboard/network/local_peer.h"

/* Without password protection the web interface answers only requests whose peer address
 * ipv4_is_local_peer() accepts and whose Host header http_host_is_local() accepts. A port forward
 * keeps the peer's public source address, or (on a masquerading router) replaces it with the
 * gateway's; either way the request must be refused. */

static constexpr uint32_t ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | d;
}

static const uint32_t HOME_IP = ip(192, 168, 1, 50);
static const uint32_t HOME_MASK = ip(255, 255, 255, 0);
static const uint32_t HOME_GW = ip(192, 168, 1, 1);
static const uint32_t AP_IP = ip(192, 168, 4, 1);
static const char* const HOSTNAME = "battery-emulator-ab12";

static bool peer_local(uint32_t peer) {
  return ipv4_is_local_peer(peer, HOME_IP, HOME_MASK, HOME_GW);
}

static bool host_local(const char* host) {
  return http_host_is_local(host, HOSTNAME, HOME_IP, AP_IP);
}

TEST(LocalPeer, PrivateRangesAreLocal) {
  EXPECT_TRUE(peer_local(ip(10, 0, 0, 1)));
  EXPECT_TRUE(peer_local(ip(10, 255, 255, 255)));
  EXPECT_TRUE(peer_local(ip(172, 16, 0, 1)));
  EXPECT_TRUE(peer_local(ip(172, 31, 255, 255)));
  EXPECT_TRUE(peer_local(ip(192, 168, 1, 23)));
  EXPECT_TRUE(peer_local(ip(192, 168, 4, 2)));  // soft-AP client
  EXPECT_TRUE(peer_local(ip(169, 254, 10, 20)));
  EXPECT_TRUE(peer_local(ip(127, 0, 0, 1)));
}

TEST(LocalPeer, RangeNeighboursArePublic) {
  EXPECT_FALSE(peer_local(ip(9, 255, 255, 255)));
  EXPECT_FALSE(peer_local(ip(11, 0, 0, 1)));
  EXPECT_FALSE(peer_local(ip(172, 15, 255, 255)));
  EXPECT_FALSE(peer_local(ip(172, 32, 0, 1)));
  EXPECT_FALSE(peer_local(ip(192, 167, 255, 255)));
  EXPECT_FALSE(peer_local(ip(192, 169, 0, 1)));
  EXPECT_FALSE(peer_local(ip(8, 8, 8, 8)));
  EXPECT_FALSE(peer_local(ip(84, 2, 44, 1)));
}

TEST(LocalPeer, CarrierGradeNatIsNotLocal) {
  // Other customers of the same ISP arrive from here when the router's WAN address is CGNAT.
  EXPECT_FALSE(peer_local(ip(100, 64, 0, 1)));
  EXPECT_FALSE(peer_local(ip(100, 127, 255, 254)));
}

TEST(LocalPeer, GatewayIsNotLocal) {
  // A router that masquerades forwarded traffic makes every internet client look like itself.
  EXPECT_FALSE(peer_local(HOME_GW));
  EXPECT_TRUE(peer_local(ip(192, 168, 1, 2)));
  // No gateway known (static IP without one, station down): nothing is excluded.
  EXPECT_TRUE(ipv4_is_local_peer(HOME_GW, HOME_IP, HOME_MASK, 0));
}

TEST(LocalPeer, PublicLanSubnetIsLocal) {
  const uint32_t own = ip(203, 0, 113, 10);
  const uint32_t mask = ip(255, 255, 255, 248);  // /29: .8 - .15
  const uint32_t gw = ip(203, 0, 113, 9);
  EXPECT_TRUE(ipv4_is_local_peer(ip(203, 0, 113, 11), own, mask, gw));
  EXPECT_TRUE(ipv4_is_local_peer(ip(203, 0, 113, 14), own, mask, gw));
  EXPECT_FALSE(ipv4_is_local_peer(gw, own, mask, gw));
  EXPECT_FALSE(ipv4_is_local_peer(ip(203, 0, 113, 17), own, mask, gw));
  EXPECT_FALSE(ipv4_is_local_peer(ip(203, 0, 113, 7), own, mask, gw));
}

TEST(LocalPeer, NoStationAddressAllowsOnlyPrivateRanges) {
  // Station not connected (or AP only): localIP() and subnetMask() read 0.0.0.0, and
  // (peer & 0) == (0 & 0) would otherwise accept the whole internet.
  EXPECT_FALSE(ipv4_is_local_peer(ip(8, 8, 8, 8), 0, 0, 0));
  EXPECT_TRUE(ipv4_is_local_peer(ip(192, 168, 4, 2), 0, 0, 0));
}

TEST(LocalPeer, MaskWiderThanSlash8DoesNotCount) {
  const uint32_t own = ip(84, 2, 44, 10);
  EXPECT_FALSE(ipv4_is_local_peer(ip(84, 200, 1, 1), own, 0, 0));
  EXPECT_FALSE(ipv4_is_local_peer(ip(1, 2, 3, 4), own, ip(128, 0, 0, 0), 0));
  EXPECT_FALSE(ipv4_is_local_peer(ip(85, 0, 0, 1), own, ip(254, 0, 0, 0), 0));
  EXPECT_TRUE(ipv4_is_local_peer(ip(84, 200, 1, 1), own, ip(255, 0, 0, 0), 0));
}

TEST(LocalHost, OwnAddressesAreLocal) {
  EXPECT_TRUE(host_local("192.168.1.50"));
  EXPECT_TRUE(host_local("192.168.1.50:80"));
  EXPECT_TRUE(host_local("192.168.4.1"));
}

TEST(LocalHost, OtherAddressLiteralsAreNot) {
  EXPECT_FALSE(host_local("84.2.44.1"));  // the router's public address, as a port forward sees it
  EXPECT_FALSE(host_local("84.2.44.1:8080"));
  EXPECT_FALSE(host_local("192.168.1.1"));    // some other device, e.g. a proxy keeping its own address
  EXPECT_FALSE(host_local("192.168.1.500"));  // not an address, and not a local name either
  EXPECT_FALSE(host_local("[2001:db8::1]"));
  EXPECT_FALSE(host_local("[fe80::1]:80"));
  // Without a station or AP address no literal matches.
  EXPECT_FALSE(http_host_is_local("0.0.0.0", HOSTNAME, 0, 0));
}

TEST(LocalHost, DeviceHostnameUnderAnyDomainIsLocal) {
  EXPECT_TRUE(host_local("battery-emulator-ab12"));
  EXPECT_TRUE(host_local("battery-emulator-ab12.local"));
  EXPECT_TRUE(host_local("Battery-Emulator-AB12.local."));
  EXPECT_TRUE(host_local("battery-emulator-ab12.fritz.box"));
  EXPECT_TRUE(host_local("battery-emulator-ab12.example.com:80"));
  EXPECT_FALSE(host_local("battery-emulator-ab1.example.com"));
  EXPECT_FALSE(host_local("battery-emulator-ab123.example.com"));
  EXPECT_FALSE(http_host_is_local("x.example.com", "", HOME_IP, AP_IP));
}

TEST(LocalHost, LocalSuffixesAndSingleLabelsAreLocal) {
  EXPECT_TRUE(host_local("solar"));
  EXPECT_TRUE(host_local("solar:80"));
  EXPECT_TRUE(host_local("be.lan"));
  EXPECT_TRUE(host_local("be.home"));
  EXPECT_TRUE(host_local("be.home.arpa"));
  EXPECT_TRUE(host_local("be.garage.internal"));
  EXPECT_TRUE(host_local("be.localdomain"));
  EXPECT_TRUE(host_local("BE.LAN"));
}

TEST(LocalHost, PublicNamesAreNot) {
  EXPECT_FALSE(host_local("myhome.duckdns.org"));
  EXPECT_FALSE(host_local("solar.example.com"));
  EXPECT_FALSE(host_local("evil.lan.example.com"));
  EXPECT_FALSE(host_local("myplan.com"));
  EXPECT_FALSE(host_local("my.homelan"));
  EXPECT_FALSE(host_local("."));
  EXPECT_FALSE(host_local(":80"));
}

TEST(LocalHost, MissingHostPasses) {
  EXPECT_TRUE(host_local(""));
}
