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


//------------------------------------------------------------------------------------------------//
// MCU Versioning

// The below code generates a 32-bit timestamp (seconds since epoch).
// It has been sent through hell to be strictly compile-time in a pre-C++14 world.
// I hate it, but it does work.

// TODO: move these to unilib
const uint8_t CMD_VERSION_COMMIT = 7;
const uint8_t CMD_VERSION_BUILD = 8;

const int EPOCH_YEAR = 2022;
const int EPOCH_MONTH = 1;  // Month does not work correctly if changed
const int EPOCH_DATE = 1;

constexpr int isleap(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
// From https://www.geeksforgeeks.org/dsa/count-of-leap-years-in-a-given-year-range/
constexpr int leap_years(int year) { 
    return (year / 4 - (EPOCH_YEAR - 1) / 4) - (year / 100 - (EPOCH_YEAR - 1) / 100)
        + (year / 400 - (EPOCH_YEAR - 1) / 400);
}

constexpr int YEAR = (__DATE__[7] - '0') * 1000 + (__DATE__[8] - '0') * 100 + (__DATE__[9] - '0') * 10
    + (__DATE__[10] - '0');
// There is genuinely no better way to do this... https://stackoverflow.com/a/19760369
constexpr int MONTH = (
    __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 1 : 6)
    : __DATE__ [2] == 'b' ? 2
    : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4)
    : __DATE__ [2] == 'y' ? 5
    : __DATE__ [2] == 'l' ? 7
    : __DATE__ [2] == 'g' ? 8
    : __DATE__ [2] == 'p' ? 9
    : __DATE__ [2] == 't' ? 10
    : __DATE__ [2] == 'v' ? 11
    : 12);
constexpr int DATE = (__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0');
constexpr int HOUR = (__TIME__[0] - '0') * 10 + (__TIME__[1] - '0');
constexpr int MINUTE = (__TIME__[3] - '0') * 10 + (__TIME__[4] - '0');
constexpr int SECOND = (__TIME__[6] - '0') * 10 + (__TIME__[7] - '0');

// This is the actual worst... but pre-C++14 restricts constexpr functions to a single return statement.
// Inspired by https://stackoverflow.com/a/73846054
constexpr int DAYS_IN_YEAR_TILL_MONTH = (MONTH > 1 ? 31 : 0)
    + (MONTH > 2 ? 28 : 0)
    + (MONTH > 3 ? 31 : 0)
    + (MONTH > 4 ? 30 : 0)
    + (MONTH > 5 ? 31 : 0)
    + (MONTH > 6 ? 30 : 0)
    + (MONTH > 7 ? 31 : 0)
    + (MONTH > 8 ? 31 : 0)
    + (MONTH > 9 ? 30 : 0)
    + (MONTH > 10 ? 31 : 0)
    + (MONTH > 11 ? 30 : 0)
    + (isleap(YEAR) && MONTH > 2 ? 1 : 0);

constexpr int DAYS_TOTAL = (YEAR - EPOCH_YEAR) * 365 + leap_years(YEAR) + DAYS_IN_YEAR_TILL_MONTH
    + (DATE - EPOCH_DATE);

constexpr int32_t BUILD_TIMESTAMP = DAYS_TOTAL * 24 * 3600 + (HOUR - 1) * 3600 + MINUTE * 60 + SECOND;


#ifdef PROJECT_VERSION_ISMAIN
static constexpr int16_t IS_MAINDIRTY_CODE = (PROJECT_VERSION_ISMAIN) | (PROJECT_VERSION_ISDIRTY << 1)
                                            | (ASTRA_LIB_VERSION_ISMAIN << 2) | (ASTRA_LIB_VERSION_ISDIRTY << 3);
#else
#   error "If you are seeing this in the code, just build the project. If you are seeing this at compile time, ask David..."
#endif


// Seeing as how I am retarded and made VicCAN solely a header file, I can only use it in main.cpp.
// So, macro it is.
#define SEND_VERSION_INFO \
    vicCAN.send(CMD_VERSION_COMMIT, PROJECT_VERSION_COMMIT_HASH, ASTRA_LIB_VERSION_COMMIT_HASH); \
    vicCAN.send(CMD_VERSION_BUILD, BUILD_TIMESTAMP, IS_MAINDIRTY_CODE);


//------------------------------------------------------------------------------------------------//
// Common helper functions

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


//------------------------------------------------------------------------------------------------//
// Stopwatch

// Whether or not to print on every stopwatch action
// #define STOPWATCH_PRINT
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
