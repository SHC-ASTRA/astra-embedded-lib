# astra-embedded-lib

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL_v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

Standardizing ASTRA's embedded code.

This library holds the classes, functions, constants, and protocol implementations shared by
ASTRA's microcontroller firmware, so that each submodule repo only contains what is actually
unique to that submodule. A few examples of what lives here:

* `parseInput(const String, std::vector<String>&)` — splits input from USB or UART on commas into
  a `std::vector<String>`. The basis of every command interface on the rover.
* `CAN_sendControl(uint8_t, sparkMax_ctrlType, float)` — sends a control command to a REV
  SparkMax. All communication with REV motor controllers should reside in `AstraREVCAN.{h,cpp}`.
* `isCalibrated(Adafruit_BNO055&)` — checks whether the BNO has calibration data saved in EEPROM.
* `SERIAL_BAUD` — the USB serial baud rate used by all of ASTRA's MCUs.

## Table of Contents

 1. [Library contents](#library-contents)
 2. [Usage in PlatformIO](#usage-in-platformio)
 3. [Adding a new header](#adding-a-new-header)
 4. [Versioning](#versioning)
 5. [Responsible People](#responsible-people)

## Library contents

Headers live in `include/`, implementations in `src/`. Headers with no `.cpp` are header-only.

| Header | Contents |
| --- | --- |
| `AstraCAN.h` | Selects and includes the CAN library for the target MCU; `printCANframe()`. |
| `AstraMisc.h` | Constants, `Timer`, `Stopwatch_t`, input parsing, and build-time version info. Useful to every ASTRA project. |
| `AstraMotors.h` | `AstraMotors` class — a single REV SparkMax, with duty cycle ramping and status frames. |
| `AstraNP.h` | `AstraNeoPixel` class — status indicator using the onboard NeoPixel. |
| `AstraREVCAN.h` | ASTRA's implementation of the REV SparkMax CAN protocol. |
| `AstraREVTypes.h` | Enums and status structs for the REV SparkMax (header-only). |
| `AstraSensors.h` | Helpers for the BNO055 IMU, BMP388 barometer, and u-blox GNSS. |
| `AstraVicCAN.h` | VicCAN — ASTRA's inter-MCU communication standard (header-only). |

Supporting files:

* `library.json` — PlatformIO manifest: dependencies, license, and the build hook below.
* `extra_script.py` — PlatformIO build hook; supplies the version info used by `AstraMisc.h`.
* `examples/` — starting templates for a new project.
* `.clang-format` — formatting config for ASTRA's C++ code.

## Usage in PlatformIO

### Adding to an existing PlatformIO project

 1. Add the library to `lib_deps` in your `platformio.ini`, pinned to a release tag:

    ```ini
    lib_deps =
        https://github.com/SHC-ASTRA/astra-embedded-lib#v2.0.0
    ```

 2. Include the headers you need. E.g., `#include "AstraVicCAN.h"`
 3. Add each header's own external dependencies to `lib_deps` as well. A header that is missing
    one will stop the build with a message naming the exact line to add.

### Starting a new PlatformIO project

 1. Copy the example from `.pio/libdeps/[env]/astra-Embedded-Lib/examples/Template/`
 2. Include whichever headers you need from `include/`
 3. Get writing!

### Updating your libraries

PlatformIO provides a button which will check for updates in all of your dependencies and update
them for you. Here's how to take advantage of it:

* Open the PlatformIO side-bar (the alien on the left)
* Under "Project Tasks" (the top pane), for each environment specified in your `platformio.ini`,
  there are a handful of folders. "General" and "Platform" should be open already.
* Open the "Dependencies" folder.
* Click "Update".

## Adding a new header

 1. Put the header in `include/` and its implementation in `src/`. Header-only is fine when there
    is nothing to compile separately.
 2. Name both files in camel case with every word capitalized, including the first, prefixed with
    `Astra`. Ex: `AstraMotors.h` / `AstraMotors.cpp`
 3. If the header needs an external Arduino library, guard it with `__has_include` and fail with
    an `#error` naming the exact `lib_deps` line to add — see `AstraSensors.h`. A missing
    dependency should hand you the fix, not bury you in vague include errors.
 4. If the header can do something useful without that library, `#warning` and a feature macro are
    better than an `#error`. `AstraVicCAN.h` does this: with no CAN library available it defines
    everything anyway and runs over serial only.
 5. Add it to [Library contents](#library-contents) above.

Those guards are what let all of ASTRA's shared code live in one library without every project
carrying every dependency. They only apply to headers you actually include, so a project pulls
in exactly the external libraries it uses.

## Versioning

Releases are git tags. Pin one in `lib_deps` (see [Usage](#adding-to-an-existing-platformio-project))
rather than tracking the branch, so an old release of an embedded project continues to build
after breaking changes have been released on main.

## Responsible People

### Author

Name: David Sharpe

Email: <ds0196@uah.edu>

### Maintainer

Name: David Sharpe

Email: <ds0196@uah.edu>
