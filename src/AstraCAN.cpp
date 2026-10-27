/**
 * @file AstraCAN.cpp
 * @author Tristan McGinnis (tlm0047@uah.edu)
 * @author Charles Marmann (cmm0077@uah.edu)
 * @author David Sharpe (ds0196@uah.edu)
 * @brief
 *
 */

#if __has_include("ESP32-TWAI-CAN.hpp")

#    include "AstraCAN.h"

void printCANframe(CanFrame& frame) {
    Serial.print("CAN: ");
    Serial.print(frame.identifier, HEX);
    Serial.print(" [");
    Serial.print(frame.data_length_code);
    Serial.print("] ");
    for (int i = 0; i < frame.data_length_code; i++) {
        Serial.print(frame.data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

#endif  // __has_include()
