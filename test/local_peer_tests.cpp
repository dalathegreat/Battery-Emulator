#include <gtest/gtest.h>

#include "../Software/src/devboard/network/local_peer.h"

/* Without password protection the web interface answers only peers ipv4_is_local_peer()
 * accepts. A port forward keeps the peer's public source address, so everything outside the
 * private ranges and the station's own subnet must be refused. */

static constexpr uint32_t ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | d;
}

static const uint32_t HOME_IP = ip(192, 168, 1, 50);
static const uint32_t HOME_MASK = ip(255, 255, 255, 0);

TEST(LocalPeer, PrivateRangesAreLocal) {
  EXPECT_TRUE(ipv4_is_local_peer(ip(10, 0, 0, 1), HOME_IP, HOME_MASK));
  EXPECT_TRUE(ipv4_is_local_peer(ip(10, 255, 255, 255), HOME_IP, HOME_MASK));
  EXPECT_TRUE(ipv4_is_local_peer(ip(172, 16, 0, 1), HOME_IP, HOME_MASK));
  EXPECT_TRUE(ipv4_is_local_peer(ip(172, 31, 255, 255), HOME_IP, HOME_MASK));
  EXPECT_TRUE(ipv4_is_local_peer(ip(192, 168, 4, 2), HOME_IP, HOME_MASK));  // soft-AP client
  EXPECT_TRUE(ipv4_is_local_peer(ip(169, 254, 10, 20), HOME_IP, HOME_MASK));
  EXPECT_TRUE(ipv4_is_local_peer(ip(127, 0, 0, 1), HOME_IP, HOME_MASK));
}

TEST(LocalPeer, RangeNeighboursArePublic) {
  EXPECT_FALSE(ipv4_is_local_peer(ip(9, 255, 255, 255), HOME_IP, HOME_MASK));
  EXPECT_FALSE(ipv4_is_local_peer(ip(11, 0, 0, 1), HOME_IP, HOME_MASK));
  EXPECT_FALSE(ipv4_is_local_peer(ip(172, 15, 255, 255), HOME_IP, HOME_MASK));
  EXPECT_FALSE(ipv4_is_local_peer(ip(172, 32, 0, 1), HOME_IP, HOME_MASK));
  EXPECT_FALSE(ipv4_is_local_peer(ip(192, 167, 255, 255), HOME_IP, HOME_MASK));
  EXPECT_FALSE(ipv4_is_local_peer(ip(192, 169, 0, 1), HOME_IP, HOME_MASK));
  EXPECT_FALSE(ipv4_is_local_peer(ip(8, 8, 8, 8), HOME_IP, HOME_MASK));
  EXPECT_FALSE(ipv4_is_local_peer(ip(84, 2, 44, 1), HOME_IP, HOME_MASK));
}

TEST(LocalPeer, CarrierGradeNatIsNotLocal) {
  // Other customers of the same ISP arrive from here when the router's WAN address is CGNAT.
  EXPECT_FALSE(ipv4_is_local_peer(ip(100, 64, 0, 1), HOME_IP, HOME_MASK));
  EXPECT_FALSE(ipv4_is_local_peer(ip(100, 127, 255, 254), HOME_IP, HOME_MASK));
}

TEST(LocalPeer, PublicLanSubnetIsLocal) {
  const uint32_t own = ip(203, 0, 113, 10);
  const uint32_t mask = ip(255, 255, 255, 248);  // /29: .8 - .15
  EXPECT_TRUE(ipv4_is_local_peer(ip(203, 0, 113, 9), own, mask));
  EXPECT_TRUE(ipv4_is_local_peer(ip(203, 0, 113, 14), own, mask));
  EXPECT_FALSE(ipv4_is_local_peer(ip(203, 0, 113, 17), own, mask));
  EXPECT_FALSE(ipv4_is_local_peer(ip(203, 0, 113, 7), own, mask));
}

TEST(LocalPeer, NoStationAddressAllowsOnlyPrivateRanges) {
  // Station not connected (or AP only): localIP() and subnetMask() read 0.0.0.0, and
  // (peer & 0) == (0 & 0) would otherwise accept the whole internet.
  EXPECT_FALSE(ipv4_is_local_peer(ip(8, 8, 8, 8), 0, 0));
  EXPECT_TRUE(ipv4_is_local_peer(ip(192, 168, 4, 2), 0, 0));
}

TEST(LocalPeer, MaskWiderThanSlash8DoesNotCount) {
  const uint32_t own = ip(84, 2, 44, 10);
  EXPECT_FALSE(ipv4_is_local_peer(ip(84, 200, 1, 1), own, 0));
  EXPECT_FALSE(ipv4_is_local_peer(ip(1, 2, 3, 4), own, ip(128, 0, 0, 0)));
  EXPECT_FALSE(ipv4_is_local_peer(ip(85, 0, 0, 1), own, ip(254, 0, 0, 0)));
  EXPECT_TRUE(ipv4_is_local_peer(ip(84, 200, 1, 1), own, ip(255, 0, 0, 0)));
}
