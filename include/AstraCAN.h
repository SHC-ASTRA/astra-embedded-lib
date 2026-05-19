/**
 * @file AstraCAN.h
 * @author Tristan McGinnis (tlm0047@uah.edu)
 * @author Charles Marmann (cmm0077@uah.edu)
 * @brief Provides methods for interfacing with Rev SparkMax motor controllers over CAN
 *
 */
#pragma once

// Must have the CAN library corresponding to the current MCU
#if defined(CORE_TEENSY) && !__has_include("FlexCAN_T4.h")  // If on Teensy, must have FlexCAN_T4
#    error Missing library! Please add the following line to lib_deps in platformio.ini:  https://github.com/tonton81/FlexCAN_T4

#elif defined(ESP32) && !__has_include("ESP32-TWAI-CAN.hpp")  // If on ESP32, must have ESP32-TWAI-CAN
#    error Missing library! Please add the following line to lib_deps in platformio.ini:  handmade0octopus/ESP32-TWAI-CAN@^1.0.1

#elif defined(ARDUINO_RASPBERRY_PI_PICO)
#    error "Raspberry Pi Pico is not supported"

#else  // We have the required library.


//--------------------------------------------------------------------------//
//   Microcontroller-specific                                               //
//--------------------------------------------------------------------------//

#    if defined(ESP32)
//---------//
//  ESP32  //
//---------//

#        include <ESP32-TWAI-CAN.hpp>  // handmade0octopus/ESP32-TWAI-CAN

void printCANframe(CanFrame& frame);


#    elif defined(CORE_TEENSY)
//----------//
//  Teensy  //
//----------//

#        include <FlexCAN_T4.h>  // https://github.com/tonton81/FlexCAN_T4

// Core and LANCE use CAN1, Arm uses CAN3
#        ifdef ARM
typedef FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> AstraCAN;
#        else
typedef FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> AstraCAN;
#        endif


#    endif  // End microcontroller check


#endif  // End library check
