/**
 * @file DIGIT.h
 * @author David Sharpe (ds0196@uah.edu)
 * @brief Digit PCB; controls end effector and wrist; attaches to end of arm
 *
 */
#pragma once


#if defined(ESP32)

#   if !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32_V2)

//------------------------------------------------------------------------------------------------//
//   DOIT ESP32 Devkit V1 (URC 2025, Digit V2)
//------------------------------------------------------------------------------------------------//

// Comms

#define CAN_RX 27
#define CAN_TX 14

// Linear actuator

#define LINAC_RIN 19
#define LINAC_FIN 18

// End Effector

#define MOTOR_IN1 33
#define MOTOR_IN2 25
#define MOTOR_FAULT 4

#define LASER_NMOS 23

// ADC

#define ADC_5V 35
#define ADC_12V 36
#define ADC_VBATT 39

// SparkMax (formerly FAERIE section)

#define SPARK_PWM 26

// Misc

#define PIN_NEOPIXEL 13

// LSS

#define LSS_SERIAL Serial2
// Leaving LSS IDs for main.cpp


#   elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32_V2)

//------------------------------------------------------------------------------------------------//
//   Feather ESP32 (URC 2025, Digit V1)
//------------------------------------------------------------------------------------------------//

// Comms

#define CAN_RX 14
#define CAN_TX 32

// Linear actuator

#define LINAC_RIN 12
#define LINAC_FIN 13

// End Effector

#define MOTOR_FAULT 5
#define MOTOR_IN2 19
#define MOTOR_IN1 21

#define LASER_NMOS 25

// ADC

#define ADC_5V 34
#define ADC_12V 39
#define ADC_VBATT 36

// Misc

#define MCU_DEBUG 4

// Lynxmotion Smart Servo

#define LSS_SERIAL Serial1
#define LSS_TOP_ID 0  // TODO: default is 0; change
#define LSS_BOTTOM_ID 1  // TODO: ditto

#   endif


#elif defined(CORE_TEENSY)

//------------------------------------------------------------------------------------------------//
//   Teensy 4.x (URC 2024)
//------------------------------------------------------------------------------------------------//

//------//
// Pins //
//------//

// DC on/off control for laser on end effector
#    define PIN_LASER 8
// PWM control for REV motor that opens/closes end effector
#    define PIN_EF_MOTOR 19


//-----------//
// Constants //
//-----------//

#    define COMMS_UART Serial1


#endif
