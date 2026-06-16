#ifndef UDS_H
#define UDS_H

#include <stdint.h>

/** UDS definition file
 * @see https://github.com/rbrtjns90/uds_standard.git
 */

// ============================================================================
//    Service identifiers (SID) and common sub-functions
//    ISO 14229-1:2013 Section 9-14 (pp. 35-302), Table 1 (p. 7)
//    Positive responses use SID + 0x40 (Section 8.3, p. 33)
// ============================================================================

/**
 * @brief UDS Service Identifiers (SID)
 * 
 * ISO 14229-1:2013 Table 1 (p. 7) - Service identifier assignments:
 * - 0x00-0x0F: Reserved
 * - 0x10-0x3E: Diagnostic and communication management
 * - 0x83-0x88: Remote activation of diagnostic
 * - 0xBA-0xBE: Reserved for ISO 14229-1
 * 
 * Positive response SID = Request SID + 0x40 (Section 8.3, p. 33)
 * Negative response always starts with 0x7F (Section 8.4, p. 34)
 */
enum SID : uint8_t {
  // === Diagnostic Session Management (Section 9.2-9.3) ===
  DiagnosticSessionControl      = 0x10,  ///< Section 9.2 (p. 36) - Session control
  ECUReset                      = 0x11,  ///< Section 9.3 (p. 43) - ECU reset types
  
  // === DTC Services (Section 11.2-11.3) ===
  ClearDiagnosticInformation    = 0x14,  ///< Section 11.2 (p. 175) - Clear DTCs
  ReadDTCInformation            = 0x19,  ///< Section 11.3 (p. 178) - Read DTCs
  
  // === Data Services (Section 10.2-10.8) ===
  ReadDataByIdentifier          = 0x22,  ///< Section 10.2 (p. 106) - Read DID
  ReadMemoryByAddress           = 0x23,  ///< Section 10.3 (p. 113) - Read memory
  ReadScalingDataByIdentifier   = 0x24,  ///< Section 10.4 (p. 119) - Scaling info
  
  // === Security (Section 9.4-9.5) ===
  SecurityAccess                = 0x27,  ///< Section 9.4 (p. 47) - Seed/key auth
  CommunicationControl          = 0x28,  ///< Section 9.5 (p. 53) - Comm control
  Authentication                = 0x29,  ///< ISO 14229-1:2013 only - PKI auth
  
  // === Periodic/Dynamic Data (Section 10.5-10.6) ===
  ReadDataByPeriodicIdentifier  = 0x2A,  ///< Section 10.5 (p. 126) - Periodic DIDs
  DynamicallyDefineDataIdentifier = 0x2C, ///< Section 10.6 (p. 140) - Dynamic DIDs
  
  // === Keep-Alive ===
  TesterPresent                 = 0x3E,  ///< Section 9.6 (p. 58) - Session keep-alive

  // === Remote Activation (Section 9.7-9.11) ===
  AccessTimingParameters        = 0x83,  ///< Section 9.7 (p. 61) - Read/modify timing params
  SecuredDataTransmission       = 0x84,  ///< Section 9.8 (p. 66) - Encrypted data
  ControlDTCSetting             = 0x85,  ///< Section 9.9 (p. 71) - DTC on/off
  ResponseOnEvent               = 0x86,  ///< Section 9.10 (p. 75) - Event triggers
  LinkControl                   = 0x87,  ///< Section 9.11 (p. 99) - Baudrate control

  // === Write Services (Section 10.7-10.8) ===
  WriteDataByIdentifier         = 0x2E,  ///< Section 10.7 (p. 162) - Write DID
  InputOutputControlByIdentifier = 0x2F, ///< Section 12.2 (p. 245) - I/O control
  WriteMemoryByAddress          = 0x3D,  ///< Section 10.8 (p. 167) - Write memory

  // === Routine Control (Section 13.2) ===
  RoutineControl                = 0x31,  ///< Section 13.2 (p. 260) - Start/stop routines

  // === Upload/Download (Section 14.2-14.5) ===
  RequestDownload               = 0x34,  ///< Section 14.2 (p. 270) - Init download
  RequestUpload                 = 0x35,  ///< Section 14.3 (p. 275) - Init upload
  TransferData                  = 0x36,  ///< Section 14.4 (p. 280) - Data blocks
  RequestTransferExit           = 0x37   ///< Section 14.5 (p. 285) - End transfer
};

// ============================================================================
// Sub-function definitions for each service
// ISO 14229-1:2013 Section 9-14 - Service-specific sub-functions
// ============================================================================

/**
 * @brief DiagnosticSessionControl (0x10) sub-functions
 * 
 * ISO 14229-1:2013 Section 9.2.2.2 Table 25 (p. 39):
 * - 0x01: defaultSession - Normal operation mode
 * - 0x02: programmingSession - Flash programming mode
 * - 0x03: extendedDiagnosticSession - Extended diagnostics
 * - 0x04: safetySystemDiagnosticSession - Safety-critical access
 * - 0x05-0x3F: Reserved
 * - 0x40-0x5F: Vehicle manufacturer specific
 * - 0x60-0x7E: System supplier specific
 */
enum Session : uint8_t {
  DefaultSession      = 0x01,  ///< Table 25 - Normal operation
  ProgrammingSession  = 0x02,  ///< Table 25 - Flash programming (requires security)
  ExtendedSession     = 0x03,  ///< Table 25 - Extended diagnostic functions
  SafetySystemSession = 0x04   ///< Table 25 - Safety system diagnostics
};

/**
 * @brief ECUReset (0x11) sub-functions
 * 
 * ISO 14229-1:2013 Section 9.3.2.2 Table 33 (p. 44):
 * - 0x01: hardReset - Power-on reset equivalent
 * - 0x02: keyOffOnReset - Ignition cycle simulation
 * - 0x03: softReset - Application restart
 * - 0x04: enableRapidPowerShutDown - Fast shutdown enable
 * - 0x05: disableRapidPowerShutDown - Fast shutdown disable
 */
enum EcuResetType : uint8_t {
  HardReset             = 0x01,  ///< Table 33 - Full power cycle
  KeyOffOnReset         = 0x02,  ///< Table 33 - Ignition cycle
  SoftReset             = 0x03,  ///< Table 33 - Software restart
  EnableRapidPowerShut  = 0x04,  ///< Table 33 - Enable fast shutdown
  DisableRapidPowerShut = 0x05   ///< Table 33 - Disable fast shutdown
};

/**
 * @brief CommunicationControl (0x28) sub-functions
 * 
 * ISO 14229-1:2013 Section 9.5.2.2 Table 54 (p. 54):
 * Controls transmission and reception of messages on the network.
 * Critical for flash programming to prevent bus interference.
 * 
 * Bit 7 (suppressPosRspMsgIndicationBit) can be set to suppress positive response.
 */
enum CommunicationControlType : uint8_t {
  EnableRxAndTx                      = 0x00,  ///< Table 54 - Normal operation
  EnableRxDisableTx                  = 0x01,  ///< Table 54 - Listen only
  DisableRxEnableTx                  = 0x02,  ///< Table 54 - Transmit only
  DisableRxAndTx                     = 0x03,  ///< Table 54 - Silent mode
  EnableRxAndTxWithEnhancedAddrInfo  = 0x04,  ///< Table 54 - With subnet
  EnableRxDisableTxWithEnhancedAddrInfo = 0x05,
  DisableRxEnableTxWithEnhancedAddrInfo = 0x06,
  DisableRxAndTxWithEnhancedAddrInfo = 0x07
  // 0x08–0x3F: ISO Reserved
  // 0x40–0x5F: Vehicle manufacturer specific
  // 0x60–0x7E: System supplier specific
};

/**
 * @brief CommunicationControl (0x28) communication type parameter
 * 
 * ISO 14229-1:2013 Section 9.5.2.3 Table 55 (p. 55):
 * Bitfield specifying which message types are affected.
 */
enum CommunicationType : uint8_t {
  NormalCommunicationMessages = 0x01,  ///< Bit 0 - Application messages
  NetworkManagementMessages   = 0x02,  ///< Bit 1 - NM messages
  NetworkDownloadUpload       = 0x03   ///< Bits 0+1 - Both types
  // Bits 2-3: ISO Reserved
  // Bits 4-7: Vehicle manufacturer specific
};

/**
 * @brief RoutineControl (0x31) sub-functions
 * 
 * ISO 14229-1:2013 Section 13.2.2.2 Table 379 (p. 262):
 * - 0x01: startRoutine - Begin routine execution
 * - 0x02: stopRoutine - Abort routine execution
 * - 0x03: requestRoutineResults - Get routine output
 */
enum RoutineAction : uint8_t {
  Start = 0x01,   ///< Table 379 - Start routine
  Stop  = 0x02,   ///< Table 379 - Stop routine
  Result= 0x03    ///< Table 379 - Request results
};

/**
 * @brief ControlDTCSetting (0x85) sub-functions
 * 
 * ISO 14229-1:2013 Section 9.9.2.2 Table 87 (p. 72):
 * - 0x01: on - Enable DTC status bit updates
 * - 0x02: off - Disable DTC status bit updates
 * 
 * CRITICAL: Must disable DTC setting during flash programming to prevent
 * false DTCs from being stored due to interrupted communication.
 */
enum DTCSettingType : uint8_t {
  On  = 0x01,  ///< Table 87 - Enable DTC logging
  Off = 0x02   ///< Table 87 - Disable DTC logging (for programming)
  // 0x03–0x3F: ISO Reserved
  // 0x40–0x5F: Vehicle manufacturer specific
  // 0x60–0x7E: System supplier specific
};

/**
 * @brief AccessTimingParameters (0x83) sub-functions
 * 
 * ISO 14229-1:2013 Section 9.7.2.2 Table 74 (p. 63):
 * Allows reading and modifying P2/P2* timing parameters.
 */
enum AccessTimingParametersType : uint8_t {
  ReadExtendedTimingParameterSet = 0x01,      ///< Read extended set
  SetTimingParametersToDefaultValues = 0x02,  ///< Reset to defaults
  ReadCurrentlyActiveTimingParameters = 0x03, ///< Read current values
  SetTimingParametersToGivenValues = 0x04     ///< Set custom values
};

// ================================================================
//   Negative response handling
// ================================================================

enum NegativeResponseCode : uint8_t {
  GeneralReject                    = 0x10,
  ServiceNotSupported              = 0x11,
  SubFunctionNotSupported          = 0x12,
  IncorrectMessageLengthOrFormat   = 0x13,
  ResponseTooLong                  = 0x14,
  BusyRepeatRequest                = 0x21,
  ConditionsNotCorrect             = 0x22,
  RequestSequenceError             = 0x24,
  RequestOutOfRange                = 0x31,
  SecurityAccessDenied             = 0x33,
  InvalidKey                       = 0x35,
  ExceededNumberOfAttempts         = 0x36,
  RequiredTimeDelayNotExpired      = 0x37,
  UploadDownloadNotAccepted        = 0x70,
  TransferDataSuspended            = 0x71,
  GeneralProgrammingFailure        = 0x72,
  WrongBlockSequenceCounter        = 0x73,
  RequestCorrectlyReceived_ResponsePending = 0x78, // RCR-RP
  SubFunctionNotSupportedInActiveSession   = 0x7E,
  ServiceNotSupportedInActiveSession       = 0x7F
};

// Detect positive response SID (request SID + 0x40)
constexpr uint8_t kPositiveResponseOffset = 0x40;
#define UDS_RESPONSE_SID_OF(request_sid) ((request_sid) + kPositiveResponseOffset)

#endif /* UDS_H */