/**
 * @file AstraMisc.h
 * @author David Sharpe (ds0196@uah.edu)
 * @brief Misc functions and definitions used in Astra embedded
 *
 */
#pragma once

#include <Arduino.h>

#include <vector>

// Baud rate for comms with LSS servos
#define LSS_BAUD LSS_DefaultBaud


// Baud rate for USB comms to a computer
#define SERIAL_BAUD 115200
// Baud rate for inter-microcontroller comms over UART
#define COMMS_UART_BAUD 115200

// All String messages for Astra embedded should use this delimiter
#define CMD_DELIM ','

typedef unsigned long ASTRA_TIME_T;


// Standard struct to consolidate timer variables
// Example Usage:
//
// Timer ledBlink;
// ledBlink.interval = 1000;
// ...
// if (millis() - ledBlink.lastMillis >= ledBlink.interval) {
//     ledBlink.lastMillis = millis();
//     ledBlink.state = !ledBlink.state;
//     digitalWrite(LED_BUILTIN, ledBlink.state);
// }
struct Timer {
    ASTRA_TIME_T lastMillis = 0;
    ASTRA_TIME_T interval = 0;  // e.g. (millis() - lastMillis >= interval)
    int state = 0;
};


// Clamps x between out_min and out_max using the expected input min and max
// Used for controller input
double map_d(double x, double in_min, double in_max, double out_min, double out_max);


/**
 * @brief 
 * 
 * @param args vector<String> args given by parseInput()
 * @param numArgs Number of arguments desired, not including first argument (e.g., "ctrl" or "ping")
 * @return true if the correct number of arguments have been provided
 */
inline bool checkArgs(const std::vector<String>& args, const size_t numArgs) {
    return (args.size() - 1 == numArgs);
}


/**
 * `input` into `args` separated by `delim`; equivalent to Python's `.split`;
 * Example:  "ctrl,led,on" => `{"ctrl","led","on"}`
 * @param input String to be separated
 * @param args vector<String> to hold separated Strings
 * @author David Sharpe
 */
void parseInput(const String input, std::vector<String>& args);

/**
 * @brief Converts ADC reading to voltage based on a voltage divider
 * 
 * @param reading ADC reading
 * @param r1 Resistance of R1 in the voltage divider (in kOhms)
 * @param r2 Resistance of R2 in the voltage divider (in kOhms)
 * @return float Voltage calculated from the ADC reading
 */
float convertADC(uint16_t reading, const float r1, const float r2);


// Whether or not to print on every stopwatch action
#define STOPWATCH_PRINT
// Serial(*) to use for stopwatch printouts
#define STOPWATCH_SERIAL Serial

class Stopwatch_t {
private:
    ASTRA_TIME_T start_time = 0;
    ASTRA_TIME_T stop_time = 0;
    std::vector<ASTRA_TIME_T> lap_times;

public:
    void printMicros(ASTRA_TIME_T stamp) const;

    /**
     * @brief Resets all values and starts stopwatch at current micros()
     *
     * @return ASTRA_TIME_T start_time = micros()
     */
    ASTRA_TIME_T start();

    /**
     * @brief Records current micros() as a lap time
     *
     * @return ASTRA_TIME_T lap_time = micros()
     */
    ASTRA_TIME_T lap();

    /**
     * @brief Records the current micros() as the stop time and prints summary
     *
     * @return ASTRA_TIME_T elapsed_time = micros() - start_time
     */
    ASTRA_TIME_T stop();

    /**
     * @brief Prints stopwatch summary to STOPWATCH_SERIAL
     * 
     */
    void printSummary() const;

    inline ASTRA_TIME_T getStartTime() const {
        return start_time;
    }

    inline ASTRA_TIME_T getStopTime() const {
        return stop_time;
    }

    inline ASTRA_TIME_T getElapsedTime() const {
        return stop_time - start_time;
    }
};

extern Stopwatch_t stopwatch;
