/**
 * @file AstraREVCAN.h
 * @author David Sharpe (ds0196@uah.edu)
 * @brief ASTRA's utilities for interfacing with the REV Sparkmax over CAN
 *
 */
#pragma once

#include <Arduino.h>  // For fixed-size integer type definitions

#include "AstraCAN.h"
#include "AstraREVTypes.h"


//--------------------------------------------------------------------------//
//   Specific REV Commands                                                  //
//--------------------------------------------------------------------------//

/**
 * @brief Sends a broadcast CAN message which all SparkMaxes will respond to after [ID] ms
 *
 */
void CAN_enumerate();


/**
 * @brief Sends a control command to the specified SparkMax device
 *
 * @param deviceId The REV ID to send the control command to
 * @param ctrlType The type of control command to send (i.e., duty cycle, velocity, etc.)
 * @param value The float value to set the control mode to
 */
void CAN_sendControl(uint8_t deviceId, sparkMax_ctrlType ctrlType, float value);

/**
 * @brief Sends a heartbeat frame to the specified REV ID; required for each SparkMax atleast every 25 ms (i think)
 *
 * @param deviceId REV ID
 */
void CAN_sendHeartbeat(uint8_t deviceId);

/**
 * @brief Makes the light on the specified SparkMax flash like this: https://docs.revrobotics.com/brushless/spark-max/status-led
 *
 * @param deviceId REV ID
 */
void CAN_identifySparkMax(uint8_t deviceId);


/**
 * @brief Sets a parameter on the specified SparkMax device. If incorrect type, then SparkMax will return an error
 *
 * @param deviceId REV ID
 * @param parameterID The parameter ID to set
 * @param type The type of the parameter (ie., int32, float, etc.)
 * @param value The value to set for the parameter
 */
void CAN_setParameter(uint8_t deviceId, sparkMax_ConfigParameter parameterID, sparkMax_ParameterType type,
                      uint32_t value);

/**
 * @brief Request the value (and type) of a parameter from the specified SparkMax device
 *
 * @param deviceId REV ID
 * @param parameterID The ID of the parameter to request
 */
void CAN_reqParameter(uint8_t deviceId, sparkMax_ConfigParameter parameterID);

/**
 * @brief Set the period of a periodic status frame on the specified SparkMax device
 *
 * These are the default periods, in ms:
 * Status0 - 10
 * Status1 - 20
 * Status2 - 20
 * Status3 - 50
 * Status4 - 20
 * Status5 - 200
 * Status6 - 200
 * Status7 - 250
 *
 * @param deviceId REV ID
 * @param frameId The ID of the periodic status frame to set the period of
 * @param periodMs The period to set for the specified periodic status frame, in milliseconds
 */
void CAN_setStatusPeriod(uint8_t deviceId, sparkMax_PeriodicFrame frameId, int periodMs);


//-------------------------------------//
//  Backwards Compatibility Functions  //
//-------------------------------------//

[[deprecated("Use CAN_sendHeartbeat(uint8_t deviceId) instead")]]
inline void sendHeartbeat(TwaiCAN& Can0, int can_id) {
    CAN_sendHeartbeat(can_id);
}


//--------------------------//
//  Basic Helper Functions  //
//--------------------------//

/**
 * @brief Convert float to little endian decimal representation
 * 
 * @param[in] x Float in big endian to be converted
 * @param buffer_data 64-bit buffer corresponding to the data frame of a CAN packet
 */
void Float2LEDec(float x, uint8_t (&buffer_data)[8]);


// Using target device REV ID and REV API ID
void CAN_sendPacket(uint8_t deviceId, int32_t apiId, uint8_t data[], uint8_t dataLen);

/**
 * @brief Compose and send a CAN Frame using direct values; allows platform-independent code
 * 
 * @param messageID CAN Frame ID (extended, 29-bits)
 * @param data Size 8 array of bytes
 * @param dataLen CAN Frame DLC (Data length code, 0..8)
 */
void CAN_sendPacket(uint32_t messageID, uint8_t data[], uint8_t dataLen);


/**
 * @brief Parses and prints a REV CAN Frame to Serial for debugging
 *
 * @param frame The CAN frame to be printed
 */
void printREVFrame(CanFrame frame);

/**
 * @brief Parses and prints a REV CAN Frame including parameter information
 *
 * @param rxFrame The CAN frame to be printed
 */
void printREVParameter(CanFrame rxFrame);
