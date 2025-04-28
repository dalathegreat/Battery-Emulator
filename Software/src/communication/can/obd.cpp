#include "obd.h"
#include "comm_can.h"

void transmit_can_frame(CAN_frame* tx_frame, int interface);

void show_dtc(uint8_t byte0, uint8_t byte1);

void show_dtc(uint8_t byte0, uint8_t byte1) {
  char letter;
  switch (byte0 >> 6) {
    case 0:
      letter = 'P';
      break;
    case 1:
      letter = 'C';
      break;
    case 2:
      letter = 'B';
      break;
    case 3:
      letter = 'U';
      break;
  }
  logging.printf("%c%d\n", letter, ((byte0 & 0x3F) << 8) | byte1);
}

void handle_obd_frame(CAN_frame& rx_frame) {
#ifdef DEBUG_LOG
  if (rx_frame.data.u8[1] == 0x7F) {
    const char* error_str = "?";
    switch (rx_frame.data.u8[3]) {  // See https://automotive.wiki/index.php/ISO_14229
      case 0x10:
        error_str = "generalReject";
        break;
      case 0x11:
        error_str = "serviceNotSupported";
        break;
      case 0x12:
        error_str = "subFunctionNotSupported";
        break;
      case 0x13:
        error_str = "incorrectMessageLengthOrInvalidFormat";
        break;
      case 0x14:
        error_str = "responseTooLong";
        break;
      case 0x21:
        error_str = "busyRepeatReques";
        break;
      case 0x22:
        error_str = "conditionsNotCorrect";
        break;
      case 0x24:
        error_str = "requestSequenceError";
        break;
      case 0x31:
        error_str = "requestOutOfRange";
        break;
      case 0x33:
        error_str = "securityAccessDenied";
        break;
      case 0x35:
        error_str = "invalidKey";
        break;
      case 0x36:
        error_str = "exceedNumberOfAttempts";
        break;
      case 0x37:
        error_str = "requiredTimeDelayNotExpired";
        break;
      case 0x70:
        error_str = "uploadDownloadNotAccepted";
        break;
      case 0x71:
        error_str = "transferDataSuspended";
        break;
      case 0x72:
        error_str = "generalProgrammingFailure";
        break;
      case 0x73:
        error_str = "wrongBlockSequenceCounter";
        break;
      case 0x78:
        error_str = "requestCorrectlyReceived-ResponsePending";
        break;
      case 0x7E:
        error_str = "subFunctionNotSupportedInActiveSession";
        break;
      case 0x7F:
        error_str = "serviceNotSupportedInActiveSession";
        break;
    }
    logging.printf("ODB reply Request for service 0x%02X: %s\n", rx_frame.data.u8[2], error_str);
  } else {
    switch (rx_frame.data.u8[1] & 0x3F) {
      case 3:
        logging.printf("ODB reply service 03: Show stored DTCs, %d present:\n", rx_frame.data.u8[2]);
        for (int i = 0; i < rx_frame.data.u8[2]; i++)
          show_dtc(rx_frame.data.u8[3 + 2 * i], rx_frame.data.u8[4 + 2 * i]);
        break;
      case 7:
        logging.printf("ODB reply service 07: Show pending DTCs, %d present:\n", rx_frame.data.u8[2]);
        for (int i = 0; i < rx_frame.data.u8[2]; i++)
          show_dtc(rx_frame.data.u8[3 + 2 * i], rx_frame.data.u8[4 + 2 * i]);
        break;
      default:
        logging.printf("ODBx reply frame received:\n");
    }
  }
  dump_can_frame(rx_frame, MSG_RX);
#endif
}

void transmit_obd_can_frame(unsigned int address, int interface, bool canFD) {
  static CAN_frame OBD_frame;
  OBD_frame.FD = canFD;
  OBD_frame.ID = address;
  OBD_frame.ext_ID = address > 0x7FF;
  OBD_frame.DLC = 8;
  OBD_frame.data.u8[0] = 0x01;
  OBD_frame.data.u8[1] = 0x03;
  OBD_frame.data.u8[2] = 0xAA;
  OBD_frame.data.u8[3] = 0xAA;
  OBD_frame.data.u8[4] = 0xAA;
  OBD_frame.data.u8[5] = 0xAA;
  OBD_frame.data.u8[6] = 0xAA;
  OBD_frame.data.u8[7] = 0xAA;
  static int cnt = 0;
  switch (cnt) {
    case 2:
      transmit_can_frame(&OBD_frame, interface);  // DTC TP-ISO
      break;
    case 3:
      OBD_frame.data.u8[1] = 0x07;
      transmit_can_frame(&OBD_frame, interface);  // DTC TP-ISO
      break;
    case 4:
      OBD_frame.data.u8[1] = 0x0A;
      transmit_can_frame(&OBD_frame, interface);  // DTC TP-ISO
      break;
    case 5:
      OBD_frame.data.u8[0] = 0x02;
      OBD_frame.data.u8[1] = 0x01;
      OBD_frame.data.u8[2] = 0x1C;
      transmit_can_frame(&OBD_frame, interface);  // DTC TP-ISO
      break;
  }
  cnt++;
  if (cnt == 3600)
    cnt = 0;
}
