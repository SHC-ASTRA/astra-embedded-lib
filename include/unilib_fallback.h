/**
 * @file unilib_fallback.h
 * @author David Sharpe (ds0196@uah.edu)
 * @brief Replaces "unilib/can_defs.hpp" if unilib is not available.
 *
 */
#pragma once

#include <Arduino.h>

#warning "Using unilib_fallback.h; please add the following dependency instead: https://github.com/SHC-ASTRA/unilib#main"

// Microcontroller VicCAN ID's based on submodule; use these instead of the raw numbers
enum class CanMcuId : uint8_t {
    MCU_BROADCAST = 1,
    MCU_CORE,
    MCU_ARM,
    MCU_DIGIT,
    MCU_FAERIE,
    MCU_CITADEL
};

// Possible datatypes for a VicCAN frame; decides how to decode/encode data
enum class CanDataType : uint8_t {
    DT_1f64 = 0,
    DT_2f32,
    DT_4i16,
    DT_NONE
};

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