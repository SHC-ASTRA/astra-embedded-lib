# astra-embedded-lib

Standardizing ASTRA's embedded code.

## Table of Contents

 1. [Overview](#overview)
 2. [Usage in PlatformIO](#usage-in-platformio)
 3. [Naming conventions](#naming-conventions)
 4. [File List](#files)
 5. [Theory](#theory)
 6. [Updating this Repository](#updating-this-repository)
 7. [Responsible People](#responsible-people)

## Overview

This library provides classes, structs, functions, pin number macros, and
team-wide constants, just to name a few. Here are a few examples:

* `parseInput(const String, std::vector<String>&)`: takes input from USB or UART and separates it into the vector, using commas as delimiters. Very useful for dealing with commands and data input from other mcu's.
* `CAN_sendDutyCycle(uint8_t, AstraCAN&)`: Formats and sends a CAN packet to a REV motor controller with a duty cycle. All communication with REV motor controllers should reside in `AstraREVCAN.{h,cpp}`.
* `isCalibrated(Adafruit_BNO055&)`: Attempts to check whether the BNO has been calibrated in EEPROM.
* `SERIAL_BAUD`: Standard baudrate for USB Serial used by all of ASTRA's mcu's.

## Usage in PlatformIO

### Adding to an existing PlatformIO project

 1. Add the following line to `lib_deps` in your `/platformio.ini`:

**https://github.com/SHC-ASTRA/astra-embedded-lib**

 2. Add the dependencies you need from `include/`. E.g., `#include "AstraVicCAN.h"`
 3. Make sure to grab the correct dependencies for `/platformio.ini` from the library headers.

### Starting a new PlatformIO project

 1. Copy the example file from `/.pio/libdeps/[board]/astra-Embedded-Lib/examples/Template/`
 2. Grab whatever headers you need from `include/`
 3. Get writing!

### Updating your libraries

PlatformIO provides a button which will check for updates in all of your dependencies and update them for you. Here's how to take advantage of it:

* Open the PlatformIO side-bar (the alien on the left)
* Under "Project Tasks" (the top pane), for each microcontroller specified in your platformio.ini, there are a handful of folders. "General" and "Platform" should be open already.
* Open the "Dependencies" folder.
* Click "Update".

## Naming conventions

### In documentation

* **Library files** - Depending on context, either the files generally contained in the library,
or the main functional C++ files containing functions and classes.

### File names

* **Library files** - Camel case with the first letter of all words, including the first, capitalized. Ex: `AstraArm.cpp`

## Files

### Classes

* `AstraArm.h/.cpp` - Arm
* `AstraMotors.h/.cpp` - REV motor
* `AstraNP.h/.cpp` - Status indicator using NeoPixel
* `AstraVicCAN.h` - Serial/CAN-analogous communication standard

### Library

* `library.json` - PlatformIO stuff
* `README.md` - this file. Documentation and GitHub front page.

### Misc

* `AstraREVCAN.h/.cpp` - ASTRA's implementation of CAN communication with REV motors
* `AstraSensors.h/.cpp` - functions for sensors
* `AstraMisc.h/.cpp` - functions, consts, etc. useful to all ASTRA projects.

## Theory

* `main.cpp` includes all the library headers it needs
* Only if a library header is included by `main.cpp` will it throw `#include` errors. If the header isn't
included in `main.cpp`, then no include errors, but if it is, then you will get required include errors.
* `.cpp` files enable themselves when all of its required external libraries are found. External libraries
are 90% of the reason the `.cpp` files need the option to be disabled.

### Creating a new header file

 1. Place the header file in `include/` and its implementation `.cpp` in `src/`.
 2. Choose a library macro to enable its files.
 3. Place the library macro in this file for documentation.

## Responsible People

### Author

Name: David Sharpe

Email: <ds0196@uah.edu>

### Maintainer

Name: David Sharpe

Email: <ds0196@uah.edu>
