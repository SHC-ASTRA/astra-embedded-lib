/**
 * @file AstraVicCAN.h
 * @author David Sharpe (ds0196@uah.edu)
 * @brief Easily communicate over ASTRA Vehicle CAN
 *
 * VicCAN standard is outlined here: https://docs.google.com/spreadsheets/d/1jHHier_8mMmTDISywsfqXBYcWRiQC8k8O-At4GGrdkI/edit?usp=sharing
 * A few examples are outlined here: https://docs.google.com/document/d/1NNIyvmUF-G32Z_FQDlSaTPBeQd0kKnsuYawkYdrqaOM/edit?usp=sharing
 *
 */
#pragma once

#include <Arduino.h>

#include "AstraCAN.h"  // For CAN library and printCANframe()
#include <vector>  // For std::vector<String> args from relayFromSerial()


// How many decimal places to include in data from rover
#ifndef FEEDBACK_PRECISION
#   define FEEDBACK_PRECISION 7
#endif

#if (defined(ESP32) && __has_include("ESP32-TWAI-CAN.hpp"))
#   define CAN_AVAILABLE
#endif

// Microcontroller VicCAN ID's based on submodule; use these instead of the raw numbers
enum class McuId : uint8_t {
    MCU_BROADCAST = 1,
    MCU_CORE,
    MCU_ARM,
    MCU_DIGIT,
    MCU_FAERIE,
    MCU_CITADEL
};

// Shorthand for the VicCAN ID used by the active MCU
#if defined(CORE)
#    define SUBMODULE_CAN_ID McuId::MCU_CORE
#elif defined(ARM)
#    define SUBMODULE_CAN_ID McuId::MCU_ARM
#elif defined(DIGIT)
#    define SUBMODULE_CAN_ID McuId::MCU_DIGIT
#elif defined(FAERIE)
#    define SUBMODULE_CAN_ID McuId::MCU_FAERIE
#elif defined(CITADEL)
#    define SUBMODULE_CAN_ID McuId::MCU_CITADEL
#else  // Default - no macro in platformio.ini; will only respond to broadcast messages
#    define SUBMODULE_CAN_ID McuId::MCU_BROADCAST
#    warning "No submodule defined in platformio.ini; will only respond to broadcast messages"
#endif

// Possible datatypes for a VicCAN frame; decides how to decode/encode data
enum class CanDataType : uint8_t {
    DT_1f64 = 0,
    DT_2f32,
    DT_4i16,
    DT_8i8
};

/**
 * @brief Which datatype ID to use for no data; corresponds to dlc = 0.
 *
 * This is here to attempt to allow arbitration to work even the rest of the CAN ID is the same;
 * ideally, there will not be CAN frames with identical IDs being sent by two different sources
 * at the same time. If the relay requests telemetry at the same time a MCU provides it,
 * the DT should be different so that instead of an error, arbitration can use the different DT
 * to decide which frame should be sent first.
 */
#define DT_NA CanDataType::DT_8i8

// Command IDs for VicCAN frames
enum CanCmdId : uint8_t {
    // General misc
    CMD_PING = 1,
    CMD_TIME,
    CMD_B_LED,
    CMD_SENSOR_RECON,
    // REV Motor control
    CMD_REV_STOP = 16,
    CMD_REV_IDENTIFY,
    CMD_REV_IDLE_MODE,
    CMD_REV_SET_DUTY,
    // Misc physical control
    CMD_LSS_TURNBY_DEG = 24,
    CMD_PWMSERVO_SET_DEG,
    CMD_DCMOTOR_CTRL,
    CMD_STEPPER_CTRL,
    CMD_LASER_CTRL,
    CMD_LSS_RESET,
    // Submodule-specific
    CMD_ARM_IK_CTRL = 32,
    CMD_ARM_IK_TTG,
    CMD_DIGIT_LINAC_CTRL,
    CMD_DIGIT_WRIST_ROLL,
    CMD_DIGIT_IK_CTRL,
    CMD_FAERIE_SKAKE,
    CMD_FAERIE_UVLED,
    CMD_ARM_MANUAL,
    CMD_CITADEL_FAN_CTRL,
    // Data request
    CMD_GNSS_LAT = 48,
    CMD_GNSS_LON,
    CMD_GNSS_SAT,
    CMD_DATA_IMU_GYRO,
    CMD_DATA_IMU_ACCEL_HEADING,
    CMD_REVMOTOR_FEEDBACK,
    CMD_POWER_VOLTAGE,
    CMD_ARM_ENCODER_ANGLES,
    CMD_DATA_BMP
};

/**
 * @brief Takes a MCU name in string form (i.e., "core"), and turns it into a MCU ID enum, if valid
 * 
 * @param str String containing only the MCU name, all lower case
 * @param mcuID McuId enum output corresponding to MCU name, if valid
 * @return true on valid MCU name;
 * @return false otherwise
 */
bool mcuIdFromString(String str, McuId* mcuID) {
    if (str.length() == 0)
        return false;

    str.toLowerCase();  // Make it case-insensitive
    
    if (str == "broadcast")
        *mcuID = McuId::MCU_BROADCAST;
    else if (str == "core")
        *mcuID = McuId::MCU_CORE;
    else if (str == "arm")
        *mcuID = McuId::MCU_ARM;
    else if (str == "digit")
        *mcuID = McuId::MCU_DIGIT;
    else if (str == "faerie")
        *mcuID = McuId::MCU_FAERIE;
    else if (str == "citadel")
        *mcuID = McuId::MCU_CITADEL;
    else
        return false;
    
    return true;
}

/**
 * @brief Takes a MCU ID enum and converts it into a MCU name in String form (i.e., "core")
 * 
 * @param mcuID MCU ID enum
 * @return String containing only the name of the MCU
 */
String mcuIdToString(const McuId mcuID) {
    if (mcuID == McuId::MCU_BROADCAST)
        return "broadcast";
    else if (mcuID == McuId::MCU_CORE)
        return "core";
    else if (mcuID == McuId::MCU_ARM)
        return "arm";
    else if (mcuID == McuId::MCU_DIGIT)
        return "digit";
    else if (mcuID == McuId::MCU_FAERIE)
        return "faerie";
    else if (mcuID == McuId::MCU_CITADEL)
        return "citadel";
    else  // Should never run unless an MCU is added and not included in this function
        return "error_mcu";
}


//------------------------------------------------------------------------------------------------//
//  VicCanFrame  -  Contains specific data used to create a CanFrame for inter-MCU comms
//------------------------------------------------------------------------------------------------//

/**
 * @brief Abstracts away raw CAN frame using data points important for VicCAN
 *
 */
class VicCanFrame {
   public:
    // Within CAN ID
    McuId mcuId;           // 3 bits
    CanDataType dataType;  // 2 bits
    uint8_t cmdId;         // 6 bits
    // Built-in to CAN frame
    bool rtr;         // 1 bit -- currently not used
    uint8_t dlc;      // 4 bits
    uint8_t data[8];  // 0..8 bytes


    // Constructor
    VicCanFrame() {
        clear();
    }

    /**
     * @brief Resets all data values; mainly for use with static keyword
     * 
     */
    void clear() {
        mcuId = McuId::MCU_BROADCAST;
        dataType = DT_NA;
        cmdId = 0;
        rtr = false;
        dlc = 0;
        for (int i = 0; i < 8; i++) {
            data[i] = 0;
        }
    }

    /**
     * @brief Automatically parse frame's data into a std::vector<double>.
     *
     * @param outData std::vector<double>; is automatically cleared.
     */
    void parseData(std::vector<double>& outData) const {
        outData.clear();
        outData.reserve(4);

        // The idea behind this method of encoding data into a can frame is that there is no
        // type noercion--the data is stored bit-by-bit in data[8] as if it was one cohesive
        // 64-bit memory address. re-interpret_cast is used to accomplish this as C++ is
        // not going to mess with an unsigned int.

        // Since each element of data[] is 1 byte, we can shift each one to where it needs to
        // fit in its relevent number memory-wise, and OR them together to make the number.
        // Once each number has been constructed byte-by-byte, we can re-interpret_cast it
        // to the desired type. (then static_cast everything to a double for consistency)

        if (dlc == 0) {
            // No data to parse
        }
        else if (dataType == CanDataType::DT_1f64) {  // 1 double
            uint64_t udata =
               (static_cast<uint64_t>(data[0]) << 56) |
               (static_cast<uint64_t>(data[1]) << 48) |
               (static_cast<uint64_t>(data[2]) << 40) |
               (static_cast<uint64_t>(data[3]) << 32) |
               (static_cast<uint64_t>(data[4]) << 24) |
               (static_cast<uint64_t>(data[5]) << 16) |
               (static_cast<uint64_t>(data[6]) << 8) |
               (static_cast<uint64_t>(data[7]) << 0);
            outData.push_back(static_cast<double>(*reinterpret_cast<double*>(&udata)));  // shut up, ik
        }
        else if (dataType == CanDataType::DT_2f32) {  // 2 floats
            uint32_t udata =
               (static_cast<uint32_t>(data[0]) << 24) |
               (static_cast<uint32_t>(data[1]) << 16) |
               (static_cast<uint32_t>(data[2]) << 8) |
               (static_cast<uint32_t>(data[3]) << 0);
            outData.push_back(static_cast<double>(*reinterpret_cast<float*>(&udata)));
            udata =
               (static_cast<uint32_t>(data[4]) << 24) |
               (static_cast<uint32_t>(data[5]) << 16) |
               (static_cast<uint32_t>(data[6]) << 8) |
               (static_cast<uint32_t>(data[7]) << 0);
            outData.push_back(static_cast<double>(*reinterpret_cast<float*>(&udata)));
        }
        else if (dataType == CanDataType::DT_4i16) {  // 4 ints
            uint16_t udata =
               (static_cast<uint16_t>(data[0]) << 8) |
               (static_cast<uint16_t>(data[1]) << 0);
            outData.push_back(static_cast<double>(*reinterpret_cast<int16_t*>(&udata)));
            udata =
               (static_cast<uint16_t>(data[2]) << 8) |
               (static_cast<uint16_t>(data[3]) << 0);
            outData.push_back(static_cast<double>(*reinterpret_cast<int16_t*>(&udata)));
            udata =
               (static_cast<uint16_t>(data[4]) << 8) |
               (static_cast<uint16_t>(data[5]) << 0);
            outData.push_back(static_cast<double>(*reinterpret_cast<int16_t*>(&udata)));
            udata =
               (static_cast<uint16_t>(data[6]) << 8) |
               (static_cast<uint16_t>(data[7]) << 0);
            outData.push_back(static_cast<double>(*reinterpret_cast<int16_t*>(&udata)));
        }
        // TODO: this does not do what it is supposed to do, unless the 8 ints should be unsigned
        else if (dataType == CanDataType::DT_8i8) {  // 8 ints
            for (int i = 0; i < 8; i++) {
                outData.push_back(static_cast<double>(data[i]));
            }
        }
    }

    /**
     * @brief Represents the VicCanFrame as a string; for use with Serial.println()
     * 
     * @return String 
     */
    String toStr() {
        String res;
        res.reserve(30);
        res = mcuIdToString(mcuId);
        res += " [";
        res += cmdId;
        res += "]";
        static std::vector<double> canData;
        parseData(canData);
        if (canData.size() > 0) {
            for (const double& data : canData) {
                res += ", ";
                res += data;
            }
        }
        return res;
    }


    /**
     * @brief Whether this mcu should care about this CAN frame
     * 
     * @return true if the frame's MCU ID matches ours or is a broadcast;
     * @return false if the frame is intended for a different mcu only.
     */
    inline bool isForMe() {
        return mcuId == SUBMODULE_CAN_ID || mcuId == McuId::MCU_BROADCAST;
    }


    /**
     * @brief Take a CAN ID and parse it into its components
     * 
     * @param id The 11-bit CAN ID to parse
     */
    void parseCanId(uint32_t id) {
        mcuId = static_cast<McuId>((id >> 8) & 0x7);
        dataType = static_cast<CanDataType>((id >> 6) & 0x3);
        cmdId = id & 0x3F;
    }

#ifdef CAN_AVAILABLE
    /**
     * @brief Take a CanFrame (typically from ESP32Can.read()) and parse it into its components
     * 
     * @param frame CanFrame to parse
     */
    void parseCanFrame(CanFrame& frame) {
        clear();
        parseCanId(frame.identifier);
        rtr = static_cast<bool>(frame.rtr);
        dlc = frame.data_length_code;
        for (int i = 0; i < dlc; i++) {
            data[i] = frame.data[i];
        }
    }
#endif

#ifdef CAN_AVAILABLE
    /**
     * @brief Read the CAN network for a frame; automatically parses into the VicCanFrame object.
     * 
     * @return true if a frame is successfully read;
     * @return false if no frame is received.
     */
    bool readCan() {
        static CanFrame inFrame;
        if (!ESP32Can.readFrame(inFrame, 0))
            return false;  // No CAN frame received

        parseCanFrame(inFrame);
        return true;
    }
#endif


    /**
     * @brief Opposite of parseCanId; takes components and creates an 11-bit CAN ID
     *
     * @param pDataType VicCAN datatype
     * @return int 11-bit CAN ID
     */
    int createCanId(CanDataType pDataType) {
        return (static_cast<uint8_t>(mcuId) << 8) | (static_cast<uint8_t>(pDataType) << 6) | cmdId;
    }

    inline int createCanId() {
        return createCanId(dataType);
    }

#ifdef CAN_AVAILABLE
    void createCanFrame(CanFrame& frame) {
        frame.identifier = createCanId();
        frame.rtr = rtr;
        frame.data_length_code = dlc;
        for (int i = 0; i < 8; i++) {
            frame.data[i] = data[i];
        }
    }
#endif

#ifdef CAN_AVAILABLE
    void sendCan() {
        static CanFrame outFrame;
        createCanFrame(outFrame);

        ESP32Can.writeFrame(outFrame);
    }
#endif
};


//------------------------------------------------------------------------------------------------//
//  VicCanController  -  Handles reading and responding to VicCanFrames
//------------------------------------------------------------------------------------------------//

class VicCanController {
   private:
    VicCanFrame inVicCanFrame;  // Translated from ESP32Can.readFrame()
    bool relayMode;             // Whether to relay stray CAN frames to Serial
    bool relayFrameWaiting;     // Whether to use a queued frame from relayFromSerial()


   public:

    /**
     * @brief Enable relay mode (relay stray CAN frames to Serial, and send our own frames straight to Serial)
     *
     */
    inline void relayOn() {
        relayMode = true;
        Serial.print("can_relay_ready,");
        Serial.println(mcuIdToString(SUBMODULE_CAN_ID));
    }

    /**
     * @brief Disable relay mode (ignore stray CAN frames; still relay stray frames from Serial to CAN)
     *
     */
    inline void relayOff() {
        relayMode = false;
        Serial.print("can_relay_off,");
        Serial.println(mcuIdToString(SUBMODULE_CAN_ID));
    }

    /**
     * @brief Get the command ID from the current VicCAN frame
     *
     * @return uint8_t command ID
     */
    inline uint8_t getCmdId() {
        return inVicCanFrame.cmdId;
    }

    /**
     * @brief Extends readCanFrame() to check destination of CAN Frame, relays stray CAN frames
     * to Serial if relayMode is on, and checks for a queued frame from relayFromSerial().
     *
     * @return true upon reading a CAN frame for this MCU;
     * @return false upon not finding a CAN frame or only reading one for a different MCU
     */
    bool readCan() {
        // Check for queued frame from relayFromSerial()
        if (relayFrameWaiting) {
            relayFrameWaiting = false;
#ifdef VICCAN_DEBUG
            Serial.println("Using frame waiting from serial relay");
#endif
            return true;  // Use inVicCanFrame already set by relayFromSerial()
        }

#ifdef CAN_AVAILABLE
        // Check CAN network for a frame
        if (!inVicCanFrame.readCan())
            return false;  // No CAN frame received

#   ifdef VICCAN_DEBUG
        Serial.println("Received CAN frame: ");
        Serial.println(inVicCanFrame.toStr());
#   endif

        // Relay stray CAN frames to Serial if relayMode is on
        if (!inVicCanFrame.isForMe()) {
            if (relayMode) {
#   ifdef VICCAN_DEBUG
                Serial.println("Relaying from CAN to Serial:");
                Serial.println(inVicCanFrame.toStr());
#   endif
                relayToSerial(inVicCanFrame);
            }
            return false;  // Not for this MCU
        }

        // We have a CAN command that we should act on.
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Automatically parse data from incoming CanFrame into a std::vector<double>.
     *
     * @param outData std::vector<double>; is automatically cleared.
     */
    inline void parseData(std::vector<double>& outData) {
        inVicCanFrame.parseData(outData);
    }

    inline void printFrame(Print *interface) {
        interface->println(inVicCanFrame.toStr());
    }


    //-------//
    // Relay //
    //-------//

    /**
     * @brief Relay CAN frame to Serial interface, either from CAN network or respond()/send()
     *
     * Serial layout: "can_relay_fromvic, [mcu], [cmdId], data[0-8]..."
     *
     * @param vicFrame The VicCanFrame to be relayed
     */
    void relayToSerial(const VicCanFrame& vicFrame) {
        Serial.print("can_relay_fromvic,");
        Serial.print(mcuIdToString(vicFrame.mcuId));
        Serial.print(",");
        Serial.print(vicFrame.cmdId);

        static std::vector<double> canData;
        vicFrame.parseData(canData);  // Load data from vicFrame into canData
        if (!canData.empty()) {  // Only print out data if there is any
            for (const double& data : canData) {
                Serial.print(",");
                Serial.print(data, FEEDBACK_PRECISION);
            }
        }

        Serial.println();
    }

    /**
     * @brief Relay a CAN frame from Serial interface to CAN network, or queue to act on it.
     *
     * Serial layout: "can_relay_tovic, [mcu], [cmdId], data[0-8]..."
     *
     * @param args std::vector<String> containing the Serial input
     */
    void relayFromSerial(const std::vector<String>& args) {
        if (args.size() < 3 || args.size() > 11) {
            Serial.println("Error: Invalid command");
            return;
        }

        if (args[2].toInt() < 0 || args[2].toInt() > 63) {
            Serial.println("Error: Invalid command ID");
            return;
        }

        VicCanFrame outVicFrame;

        outVicFrame.cmdId = args[2].toInt();

        // Interpret MCU ID string
        if (!mcuIdFromString(args[1], &outVicFrame.mcuId)) {  // Returns false if invalid MCU name
            Serial.println("Error: Invalid MCU name");
            return;
        }

        if (args.size() > 3) {  // Add data to VicCanFrame if we have it; if not, default is no data
            outVicFrame.dlc = 8;

            /**/ if (args.size() - 3 == 1) {
                outVicFrame.dataType = CanDataType::DT_1f64;
                encodeData(outVicFrame.data, args[3].toDouble());
            }
            else if (args.size() - 3 == 2) {
                outVicFrame.dataType = CanDataType::DT_2f32;
                encodeData(outVicFrame.data, args[3].toFloat(), args[4].toFloat());
            }
            else if (args.size() - 3 == 3) {
                outVicFrame.dataType = CanDataType::DT_4i16;
                encodeData(outVicFrame.data, args[3].toInt(), args[4].toInt(), args[5].toInt(), 0);
            }
            else if (args.size() - 3 == 4) {
                outVicFrame.dataType = CanDataType::DT_4i16;
                encodeData(outVicFrame.data, args[3].toInt(), args[4].toInt(), args[5].toInt(), args[6].toInt());
            }
            else if (args.size() - 3 == 8) {
                outVicFrame.dataType = CanDataType::DT_8i8;
                encodeData(outVicFrame.data, args[3].toInt(), args[4].toInt(), args[5].toInt(), args[6].toInt(),
                           args[7].toInt(), args[8].toInt(), args[9].toInt(), args[10].toInt());
            }
            else {
                Serial.println("Invalid data length");
                return;
            }
        }

        // If this CAN frame is for this MCU, queue it to act on it
        if (outVicFrame.mcuId == SUBMODULE_CAN_ID || outVicFrame.mcuId == McuId::MCU_BROADCAST) {
            relayFrameWaiting = true;
            inVicCanFrame = outVicFrame;
#ifdef VICCAN_DEBUG
            Serial.println("Queuing frame from Serial.");
#endif
        }  // purposefully no else

#ifdef CAN_AVAILABLE
        // If this CAN frame is not specifically for this MCU, relay it to the CAN network (includes broadcast messages)
        if (outVicFrame.mcuId != SUBMODULE_CAN_ID) {
#   ifdef VICCAN_DEBUG
            Serial.println("Relaying from Serial to CAN:");
            Serial.println(outVicFrame.toStr());
#   endif
            outVicFrame.sendCan();
        }
#endif
    }


    //-------------------------//
    // CAN Frame data encoding //
    //-------------------------//

    // DT_1f64 -- 1 double
    void encodeData(uint8_t canData[8], double data1) {
        uint64_t udata = *reinterpret_cast<uint64_t*>(&data1);
        canData[0] = (udata >> 56) & 0xFF;
        canData[1] = (udata >> 48) & 0xFF;
        canData[2] = (udata >> 40) & 0xFF;
        canData[3] = (udata >> 32) & 0xFF;
        canData[4] = (udata >> 24) & 0xFF;
        canData[5] = (udata >> 16) & 0xFF;
        canData[6] = (udata >> 8) & 0xFF;
        canData[7] = udata & 0xFF;
    }

    // DT_2f32 -- 2 floats
    void encodeData(uint8_t canData[8], float data1, float data2) {
        uint32_t udata = *reinterpret_cast<uint32_t*>(&data1);
        canData[0] = (udata >> 24) & 0xFF;
        canData[1] = (udata >> 16) & 0xFF;
        canData[2] = (udata >> 8) & 0xFF;
        canData[3] = udata & 0xFF;
        udata = *reinterpret_cast<uint32_t*>(&data2);
        canData[4] = (udata >> 24) & 0xFF;
        canData[5] = (udata >> 16) & 0xFF;
        canData[6] = (udata >> 8) & 0xFF;
        canData[7] = udata & 0xFF;
    }

    // DT_4i16 -- 4 ints
    void encodeData(uint8_t canData[8], int16_t data1, int16_t data2, int16_t data3, int16_t data4) {
        uint16_t udata = *reinterpret_cast<uint16_t*>(&data1);
        canData[0] = (udata >> 8) & 0xFF;
        canData[1] = udata & 0xFF;
        udata = *reinterpret_cast<uint16_t*>(&data2);
        canData[2] = (udata >> 8) & 0xFF;
        canData[3] = udata & 0xFF;
        udata = *reinterpret_cast<uint16_t*>(&data3);
        canData[4] = (udata >> 8) & 0xFF;
        canData[5] = udata & 0xFF;
        udata = *reinterpret_cast<uint16_t*>(&data4);
        canData[6] = (udata >> 8) & 0xFF;
        canData[7] = udata & 0xFF;
    }

    // DT_8i8 -- 8 ints
    void encodeData(uint8_t canData[8], int8_t data1, int8_t data2, int8_t data3, int8_t data4, int8_t data5,
                    int8_t data6, int8_t data7, int8_t data8) {
        canData[0] = *reinterpret_cast<uint8_t*>(&data1);
        canData[1] = *reinterpret_cast<uint8_t*>(&data2);
        canData[2] = *reinterpret_cast<uint8_t*>(&data3);
        canData[3] = *reinterpret_cast<uint8_t*>(&data4);
        canData[4] = *reinterpret_cast<uint8_t*>(&data5);
        canData[5] = *reinterpret_cast<uint8_t*>(&data6);
        canData[6] = *reinterpret_cast<uint8_t*>(&data7);
        canData[7] = *reinterpret_cast<uint8_t*>(&data8);
    }


    //------------------------//
    // Sending to Basestation //
    //------------------------//

    // Move this code out of send(...) because it's all the exact same
    void sendFrame(VicCanFrame& outVicFrame, uint8_t cmdId) {
        outVicFrame.mcuId = SUBMODULE_CAN_ID;
        outVicFrame.cmdId = cmdId;
        outVicFrame.dlc = 8;

#ifdef CAN_AVAILABLE
        if (relayMode) {
#endif
#ifdef VICCAN_DEBUG
            Serial.println("Relaying from CAN to Serial:");
            Serial.println(outVicFrame.toStr());
#endif
            relayToSerial(outVicFrame);
#ifdef CAN_AVAILABLE
        }
        else {
#   ifdef VICCAN_DEBUG
            Serial.println("Sending CAN frame:");
            Serial.println(outVicFrame.toStr());
#   endif
            outVicFrame.sendCan();
        }
#endif
    }


    void send(uint8_t cmdId, double data) {
        VicCanFrame outVicFrame;
        outVicFrame.dataType = CanDataType::DT_1f64;
        encodeData(outVicFrame.data, data);
        sendFrame(outVicFrame, cmdId);
    }

    void send(uint8_t cmdId, float data1, float data2) {
        VicCanFrame outVicFrame;
        outVicFrame.dataType = CanDataType::DT_2f32;
        encodeData(outVicFrame.data, data1, data2);
        sendFrame(outVicFrame, cmdId);
    }

    void send(uint8_t cmdId, int16_t data1, int16_t data2, int16_t data3, int16_t data4 = 0) {
        VicCanFrame outVicFrame;
        outVicFrame.dataType = CanDataType::DT_4i16;
        encodeData(outVicFrame.data, data1, data2, data3, data4);
        sendFrame(outVicFrame, cmdId);
    }

    void send(uint8_t cmdId, int8_t data1, int8_t data2, int8_t data3, int8_t data4, int8_t data5, int8_t data6 = 0,
              int8_t data7 = 0, int8_t data8 = 0) {
        VicCanFrame outVicFrame;
        outVicFrame.dataType = CanDataType::DT_8i8;
        encodeData(outVicFrame.data, data1, data2, data3, data4, data5, data6, data7, data8);
        sendFrame(outVicFrame, cmdId);
    }


    inline void respond(double data) {
        send(inVicCanFrame.cmdId, data);
    }

    inline void respond(float data1, float data2) {
        send(inVicCanFrame.cmdId, data1, data2);
    }

    inline void respond(int16_t data1, int16_t data2, int16_t data3, int16_t data4 = 0) {
        send(inVicCanFrame.cmdId, data1, data2, data3, data4);
    }

    inline void respond(int8_t data1, int8_t data2, int8_t data3, int8_t data4, int8_t data5,
                        int8_t data6 = 0, int8_t data7 = 0, int8_t data8 = 0) {
        send(inVicCanFrame.cmdId, data1, data2, data3, data4, data5, data6, data7, data8);
    }
} vicCAN;
