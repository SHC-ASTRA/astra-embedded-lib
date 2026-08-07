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
 3. [Build-time version info](#build-time-version-info)
 4. [Adding a new header](#adding-a-new-header)
 5. [Troubleshooting](#troubleshooting)
 6. [Versioning](#versioning)
 7. [Responsible People](#responsible-people)

## Library contents

Headers live in `include/`, implementations in `src/`. Headers with no `.cpp` are header-only.

| Header | Contents | Requires in `lib_deps` |
| --- | --- | --- |
| `AstraCAN.h` | Selects and includes the CAN library for the target MCU; `printCANframe()`. | `handmade0octopus/ESP32-TWAI-CAN` |
| `AstraMisc.h` | Constants, `Timer`, `Stopwatch_t`, input parsing, and build-time version info. Useful to every ASTRA project. | — |
| `AstraMotors.h` | `AstraMotors` class — a single REV SparkMax, with duty cycle ramping and status frames. | `handmade0octopus/ESP32-TWAI-CAN` |
| `AstraNP.h` | `AstraNeoPixel` class — status indicator using the onboard NeoPixel. | — |
| `AstraREVCAN.h` | ASTRA's implementation of the REV SparkMax CAN protocol. | `handmade0octopus/ESP32-TWAI-CAN` |
| `AstraREVTypes.h` | Enums and status structs for the REV SparkMax (header-only). | — |
| `AstraSensors.h` | Helpers for the BNO055 IMU, BMP388 barometer, and u-blox GNSS. | `adafruit/Adafruit BNO055`, `adafruit/Adafruit Unified Sensor`, `adafruit/Adafruit BMP3XX Library`, `sparkfun/SparkFun u-blox GNSS Arduino Library` |
| `AstraVicCAN.h` | VicCAN — ASTRA's inter-MCU communication standard (header-only). | `handmade0octopus/ESP32-TWAI-CAN`, optional |

[unilib](https://github.com/SHC-ASTRA/unilib) — where the VicCAN command IDs, MCU IDs, and data types
are defined — is declared as a dependency in `library.json`, so PlatformIO pulls it in on its own. You
only name it in `lib_deps` yourself when developing against a local checkout.

Supporting files:

* `library.json` — PlatformIO manifest: dependencies, license, and the build hook below.
* `extra_script.py` — PlatformIO build hook; supplies the version info used by `AstraMisc.h`.
* `examples/` — starting templates for a new project.
* `.clang-format` — formatting config for ASTRA's C++ code.

## Usage in PlatformIO

### Target hardware

Everything here targets **ESP32** under the Arduino framework — currently the Adafruit Feather
ESP32 V2, ESP32 DOIT Devkit V1, and the ESP32-S3 DevKitC. `AstraCAN.h` also carries a
Teensy/FlexCAN_T4 path, but nothing on the rover currently uses it and it goes untested;
treat it as unmaintained.

### Adding to an existing PlatformIO project

 1. Add the library to `lib_deps` in your `platformio.ini`, pinned to a release tag:

    ```ini
    lib_deps =
        https://github.com/SHC-ASTRA/astra-embedded-lib#v2.0.0
    ```

 2. Add a build flag naming which submodule this MCU is. Exactly one of `CORE`, `ARM`, `DIGIT`,
    `LANCE`, or `CITADEL`:

    ```ini
    build_flags =
        -D CORE
    ```

    This sets `SUBMODULE_CAN_ID`, which is the MCU's address on the VicCAN bus. Leave it out and
    the build still succeeds, but the MCU answers only to broadcast messages and identifies itself
    as a broadcast — so it looks alive while ignoring everything addressed to it.

 3. Include the headers you need. E.g., `#include "AstraVicCAN.h"`
 4. Add each header's own external dependencies to `lib_deps` — see
    [Library contents](#library-contents). A header missing one will stop the build with a message
    naming the exact line to add.

A complete environment, from `core-embedded`:

```ini
[env:core_main_prod]
platform = espressif32
board = adafruit_feather_esp32_v2
framework = arduino
monitor_speed = 115200
monitor_filters = send_on_enter
monitor_echo = true
build_flags =
	-D CORE
	-D MAINMCU
lib_deps =
	https://github.com/SHC-ASTRA/astra-embedded-lib#v2.0.0
	handmade0octopus/ESP32-TWAI-CAN@^1.0.1
	adafruit/Adafruit BMP3XX Library@^2.1.2
	adafruit/Adafruit BNO055@^1.6.3
	adafruit/Adafruit Unified Sensor@^1.1.14
	sparkfun/SparkFun u-blox GNSS Arduino Library@^2.2.25
	fastled/FastLED@^3.6.0
```

### Developing against a local checkout

When you're changing this library and a submodule's firmware together, separately cloning this
library is a lot easier than trying to edit the copy PlatformIO downloads as a dependency.
Add a second environment that points at your local clones instead of GitHub:

```ini
[env:core_main_dev]
; ...everything else identical to core_main_prod...
lib_deps =
	symlink://../../astra-embedded-lib
	symlink://../../unilib
	handmade0octopus/ESP32-TWAI-CAN@^1.0.1
	; ...remaining external dependencies unchanged...
```

Both paths are relative to the project directory, so this assumes `astra-embedded-lib`, `unilib`,
and the firmware repo are all checked out as siblings. Note that `unilib` has to be listed
explicitly here — the local checkout doesn't resolve the dependency from `library.json` for you.

Keep the `prod` environment around so you can always flash MCUs with `main`'s code.

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

## Build-time version info

`extra_script.py` runs on every build (PlatformIO picks it up from `library.json`) and injects git
state as compile-time defines, so a flashed MCU can tell you exactly what it is running. Two sets
get defined — `ASTRA_LIB_VERSION_*` from this library's checkout, and `PROJECT_VERSION_*` from your
firmware repo:

| Define | Source |
| --- | --- |
| `*_VERSION_MAJOR` / `_MINOR` / `_PATCH` | Latest git tag, e.g. `1`, `0`, `2` for `v1.0.2`. |
| `*_VERSION_ISMAIN` | 1 if built from the `main` branch |
| `*_VERSION_ISDIRTY` | 1 if the working tree had uncommitted changes |
| `*_VERSION_COMMIT_HASH_LOWER` / `_UPPER` | Short commit hash, split into two `int16_t` halves so it fits a VicCAN payload |

`AstraMisc.h` folds these together with `BUILD_TIMESTAMP` (seconds since 2022-01-01, computed from
`__DATE__` and `__TIME__` at compile time) into a single macro:

```cpp
SEND_VERSION_INFO
```

It sends two VicCAN frames — `CMD_VERSION_COMMIT` with both commit hashes, and `CMD_VERSION_BUILD`
with the build timestamp and a bitfield of the four main/dirty flags. Call it on a timer in your
`loop()`. It's a macro rather than a function because `AstraVicCAN.h` is header-only, so
`vicCAN` only exists in the translation unit that included it.

**The script identifies each repo by its directory name.** The libraries must end in
`-embedded-lib`; the firmware projects must contain `-embedded` or start with `rover-`. Clone into
a differently-named directory and the build still succeeds, but every version field silently
becomes zero — see [Troubleshooting](#troubleshooting).

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

## Troubleshooting

**`Missing library! Please add the following line to lib_deps in platformio.ini:  <something>`**

Do what it says — copy the line into `lib_deps` and rebuild. The header you included needs an
external library your project doesn't have yet. `AstraSensors.h` will report all four of its
dependencies at once, so read the whole output before you start adding lines one at a time.

**`No submodule defined in platformio.ini; will only respond to broadcast messages`**

You're missing your MCU's macro (e.g., `ARM`, `DIGIT`, `LANCE`, `CITADEL`). The firmware will
flash and run, and it will look healthy, but it ignores every VicCAN message addressed specifically
to it.

**`Could not find a compatible CAN library. VicCAN will be limited to Serial use only.`**

`AstraVicCAN.h` couldn't find `ESP32-TWAI-CAN`, so it built the serial-only path — `vicCAN.send()`
and friends still work, they just go to `Serial` instead of the CAN bus. Intentional on a board with
no transceiver. If you did want CAN, add `handmade0octopus/ESP32-TWAI-CAN@^1.0.1` to `lib_deps`.

**`If you are seeing this in the code, just build the project. If you are seeing this at compile time, ask David...`**

`AstraMisc.h` needs the defines from `extra_script.py`, which only exist during a real PlatformIO
build. Your editor's language server showing this is normal and harmless — don't worry about it. At
actual compile time it means the build hook didn't run.

**`PROJECT repository name mismatch. crying.`** (or `ASTRA_LIB repository name mismatch. crying.`)

The directory name doesn't match what `extra_script.py` expects, so it zeroed the version defines.
The build succeeds and `SEND_VERSION_INFO` reports all zeros. Rename the directory to end in
`-embedded-lib`, or to contain `-embedded`, depending on which one it's crying about. Cloning a repo
under a different name than upstream is the usual cause.

**`VICCAN_DEBUG is enabled. good luck soldier.`**

You turned on VicCAN's debug output. Every frame in and out gets printed to `Serial`, which is loud
enough to change timing. Fine while debugging, don't ship it.

**`Raspberry Pi Pico is not supported`**

Correct, it isn't. Use an ESP32.

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
