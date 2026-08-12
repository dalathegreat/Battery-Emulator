#pragma once

#include "../../devboard/utils/types.h"
#include "../../lib/mcp2515_lite/mcp2515_lite.h"

#include <cstring>  // for memcpy

// Check the struct layout matches closely enough to be able to memcpy (which
// then gets optimized out)
static_assert(sizeof(MCP2515_Lite_Frame) <= sizeof(CAN_frame), "MCP2515_Lite_Frame must fit within CAN_frame");
static_assert(offsetof(MCP2515_Lite_Frame, fd) == offsetof(CAN_frame, FD), "fd/FD field offset mismatch");
static_assert(offsetof(MCP2515_Lite_Frame, ext) == offsetof(CAN_frame, ext_ID), "ext/ext_ID field offset mismatch");
static_assert(offsetof(MCP2515_Lite_Frame, dlc) == offsetof(CAN_frame, DLC), "dlc/DLC field offset mismatch");
static_assert(offsetof(MCP2515_Lite_Frame, id) == offsetof(CAN_frame, ID), "id/ID field offset mismatch");
static_assert(offsetof(MCP2515_Lite_Frame, data) == offsetof(CAN_frame, data), "data field offset mismatch");

static inline void copy_can_frame_to_mcp2515_lite_frame(const CAN_frame& source, MCP2515_Lite_Frame& target) {
  memcpy(&target, &source, sizeof(MCP2515_Lite_Frame));
  target.fd = false;  // MCP2515 does not support CAN-FD
}

static inline void copy_mcp2515_lite_frame_to_can_frame(const MCP2515_Lite_Frame& source, CAN_frame& target) {
  memcpy(&target, &source, sizeof(MCP2515_Lite_Frame));
  target.FD = false;  // MCP2515 does not support CAN-FD
  // The remaining bytes in CAN_frame.data will be left as-is
}
