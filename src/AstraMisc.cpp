/**
 * @file AstraMisc.cpp
 * @author David Sharpe (ds0196@uah.edu)
 * @brief Implements miscellaneous functions for Astra embedded
 *
 */
#include "AstraMisc.h"

double map_d(double x, double in_min, double in_max, double out_min, double out_max) {
    const double run = in_max - in_min;
    if (run == 0)
    {
	    return 0;  // in_min == in_max, error
    }
    const double rise = out_max - out_min;
    const double delta = x - in_min;
    return (delta * rise) / run + out_min;
}

void parseInput(const String input, std::vector<String>& args) {
    // Modified from
    // https://forum.arduino.cc/t/how-to-split-a-string-with-space-and-store-the-items-in-array/888813/9

    // Index of previously found delim
    int lastIndex = -1;
    // Index of currently found delim
    int index = -1;
    // lastIndex=index, so lastIndex starts at -1, and with lastIndex+1, first search begins at 0

    // if empty input for some reason, don't do anything
    if (input.length() == 0) {
        args.push_back("ERR_NOINPUT");  // Prevent MCU crash from attempting to access args[0]
        return;
    }

    // Protection against infinite loop
    unsigned count = 0;
    while (count++, count < 200 /*arbitrary limit on number of delims because while(true) is scary*/) {
        lastIndex = index;
        // using lastIndex+1 instead of input = input.substring to reduce memory impact
        index = input.indexOf(CMD_DELIM, lastIndex + 1);
        if (index == -1) {  // No instance of delim found in input
            // If no delims are found at all, then lastIndex+1 == 0, so whole string is passed.
            // Otherwise, only the last part of input is passed because of lastIndex+1.
            args.push_back(input.substring(lastIndex + 1));
            // Exit the loop when there are no more delims
            break;
        } else {  // delim found
            // If this is the first delim, lastIndex+1 == 0, so starts from beginning
            // Otherwise, starts from last found delim with lastIndex+1
            args.push_back(input.substring(lastIndex + 1, index));
        }
    }

    // output is via vector<String>& args
}

float convertADC(uint16_t reading, const float r1, const float r2) {
    if (reading == 0)
        return 0;  // Avoid divide by zero

    // Max Vs that the voltage divider is designed to read
    const float maxSource = (3.3 * (r1 * 1000 + r2 * 1000)) / (r2 * 1000);

    // ADC range is 0-4095 (12-bit precision)
    return (static_cast<float>(reading) / 4095.0) * maxSource;  // Clamp reading [0-maxSource]
}

void Stopwatch_t::printMicros(ASTRA_TIME_T stamp) const {
    if (stamp < 2000) {  // < 2 ms
        STOPWATCH_SERIAL.print(stamp);
        STOPWATCH_SERIAL.print(" μs");
    } else if (stamp < 2 * 1000 * 1000) {  // < 2 s
        STOPWATCH_SERIAL.print(stamp / 1000.0);
        STOPWATCH_SERIAL.print(" ms");
    } else if (stamp < 2 * 1000 * 1000 * 60) {  // < 2 min
        STOPWATCH_SERIAL.print(stamp / (1000.0 * 1000.0));
        STOPWATCH_SERIAL.print(" s");
    } else {  // >= 2 min
        STOPWATCH_SERIAL.print(stamp / (1000.0 * 1000.0 * 60.0));
        STOPWATCH_SERIAL.print(" min");
    }
}

ASTRA_TIME_T Stopwatch_t::start() {
    start_time = micros();
    stop_time = 0;
    lap_times.clear();

#ifdef STOPWATCH_PRINT
    STOPWATCH_SERIAL.print("Stopwatch started at ");
    printMicros(start_time);
    STOPWATCH_SERIAL.println();
#endif

    return start_time;
}

ASTRA_TIME_T Stopwatch_t::lap() {
    ASTRA_TIME_T lap_time = micros();
    lap_times.push_back(lap_time);

#ifdef STOPWATCH_PRINT
    STOPWATCH_SERIAL.print("Stopwatch lapped (");
    STOPWATCH_SERIAL.print(lap_times.size()-1);
    STOPWATCH_SERIAL.print(") at ");
    printMicros(lap_time);
    STOPWATCH_SERIAL.println();
#endif

    return lap_time;
}

ASTRA_TIME_T Stopwatch_t::stop() {
    stop_time = micros();

#ifdef STOPWATCH_PRINT
    STOPWATCH_SERIAL.print("Stopwatch stopped.");

    printSummary();
#endif

    return stop_time - start_time;
}

void Stopwatch_t::printSummary() const {
    STOPWATCH_SERIAL.println("Stopwatch status:");
    STOPWATCH_SERIAL.print("Started at ");
    printMicros(start_time);
    STOPWATCH_SERIAL.println();

    if (lap_times.size() > 0) {
        for (const auto& i : lap_times) {
            STOPWATCH_SERIAL.print("Lap ");
            STOPWATCH_SERIAL.print(i + 1);
            STOPWATCH_SERIAL.print(" at ");
            printMicros(lap_times[i]);
            STOPWATCH_SERIAL.print(": ");
            if (i == 0)
                printMicros(lap_times[i] - start_time);
            else
                printMicros(lap_times[i] - lap_times[i - 1]);
            STOPWATCH_SERIAL.println();
        }
    }

    STOPWATCH_SERIAL.print("Stopped at ");
    printMicros(stop_time);
    if (lap_times.size() > 0) {
        STOPWATCH_SERIAL.print(": ");
        printMicros(stop_time - lap_times.back());
        STOPWATCH_SERIAL.println();
    } else {
        STOPWATCH_SERIAL.println();
    }

    STOPWATCH_SERIAL.print("Elapsed time: ");
    printMicros(stop_time - start_time);
    STOPWATCH_SERIAL.println();
}

Stopwatch_t stopwatch;
