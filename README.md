# astra-embedded-lib

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL_v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

Standardizing ASTRA's embedded code.

This library holds the classes, functions, constants, and protocol implementations shared by ASTRA's microcontroller firmware, so that each submodule repo only contains what is actually unique to that submodule. A few examples of what lives here:

- `parseInput(const String, std::vector<String>&)` — splits input from USB or UART on commas into a `std::vector<String>`. The basis of every command interface on the rover.
- `CAN_sendControl(uint8_t, sparkMax_ctrlType, float)` — sends a control command to a REV SparkMax. All communication with REV motor controllers should reside in `AstraREVCAN.{h,cpp}`.
- `isCalibrated(Adafruit_BNO055&)` — checks whether the BNO has calibration data saved in EEPROM.
- `SERIAL_BAUD` — the USB serial baud rate used by all of ASTRA's MCUs.

## Table of Contents

1.  [Library contents](#library-contents)
2.  [The rover's MCUs](#the-rovers-mcus)
3.  [Usage in PlatformIO](#usage-in-platformio)
    - [Adding to an existing PlatformIO project](#adding-to-an-existing-platformio-project)
    - [Starting a new PlatformIO project](#starting-a-new-platformio-project)
    - [Build flags](#build-flags)
    - [Developing against a local checkout](#developing-against-a-local-checkout)
    - [Flashing](#flashing)
    - [Updating your libraries](#updating-your-libraries)
4.  [Using VicCAN](#using-viccan)
    - [Bringing up the bus](#bringing-up-the-bus)
    - [Reading commands](#reading-commands)
    - [Sending data](#sending-data)
    - [Serial relay](#serial-relay)
    - [Safety timeouts](#safety-timeouts)
5.  [Talking to an MCU over serial](#talking-to-an-mcu-over-serial)
    - [Common serial commands](#common-serial-commands)
    - [Testing CAN without CAN](#testing-can-without-can)
6.  [Build-time version info](#build-time-version-info)
7.  [Troubleshooting](#troubleshooting)
8.  [Adding a new header](#adding-a-new-header)
9.  [Header reference](#header-reference)
10. [Maintainers](#maintainers)

## Library contents

Headers live in `include/`, implementations in `src/`. Headers with no `.cpp` are header-only.

| Header            | Contents                                                                                                      | Requires in `lib_deps`                      |
| ----------------- | ------------------------------------------------------------------------------------------------------------- | ------------------------------------------- |
| `AstraCAN.h`      | Selects and includes the CAN library for the target MCU; `printCANframe()`.                                   | `handmade0octopus/ESP32-TWAI-CAN`           |
| `AstraMisc.h`     | Constants, `Timer`, `Stopwatch_t`, input parsing, and build-time version info. Useful to every ASTRA project. | —                                           |
| `AstraMotors.h`   | `AstraMotors` class — a single REV SparkMax, with duty cycle ramping and status frames.                       | `handmade0octopus/ESP32-TWAI-CAN`           |
| `AstraNP.h`       | `AstraNeoPixel` class — status indicator using the onboard NeoPixel.                                          | —                                           |
| `AstraREVCAN.h`   | ASTRA's implementation of the REV SparkMax CAN protocol.                                                      | `handmade0octopus/ESP32-TWAI-CAN`           |
| `AstraREVTypes.h` | Enums and status structs for the REV SparkMax (header-only).                                                  | —                                           |
| `AstraSensors.h`  | Helpers for the BNO055 IMU, BMP388 barometer, and u-blox GNSS.                                                | 4 Adafruit and SparkFun libraries           |
| `AstraVicCAN.h`   | VicCAN — ASTRA's inter-MCU communication standard (header-only).                                              | `handmade0octopus/ESP32-TWAI-CAN`, optional |

[unilib](https://github.com/SHC-ASTRA/unilib) — where the VicCAN command IDs, MCU IDs, and data types are defined — is declared as a dependency in `library.json`, so PlatformIO pulls it in on its own. You only name it in `lib_deps` yourself when developing against a local checkout.

Supporting files:

- `library.json` — PlatformIO manifest: dependencies, license, and the build hook below.
- `extra_script.py` — PlatformIO build hook; supplies the version info used by `AstraMisc.h`.
- `examples/` — starting templates for a new project.
- `.clang-format` — formatting config for ASTRA's C++ code.

## The rover's MCUs

Each submodule on the rover contains one or more PCBs (like how Arm contains Socket and Digit); each submodule gets its own repository, each PCB its own MCU, and each MCU its own PlatformIO project as a subfolder in its submodule's repository. The build flag you set names the MCU, and that name is also its address on the VicCAN bus:

| Build flag | What it does             | PlatformIO project  | Submodule repo                                                        |
| ---------- | ------------------------ | ------------------- | --------------------------------------------------------------------- |
| `CORE`     | Navigation and drive     | `core_main/`        | [core-embedded](https://github.com/SHC-ASTRA/core-embedded)           |
| `ARM`      | Main arm joints          | `socket_main/`      | [arm-embedded](https://github.com/SHC-ASTRA/arm-embedded)             |
| `DIGIT`    | Arm's end-effector       | `digit_main/`       | [arm-embedded](https://github.com/SHC-ASTRA/arm-embedded)             |
| `LANCE`    | Science drill            | `lance-embedded/`   | [biosensor-embedded](https://github.com/SHC-ASTRA/biosensor-embedded) |
| `CITADEL`  | Science chemical testing | `citadel-embedded/` | [biosensor-embedded](https://github.com/SHC-ASTRA/biosensor-embedded) |

The names and IDs are defined in [unilib](https://github.com/SHC-ASTRA/unilib)'s `can_defs.hpp`, shared with ROS2; that file is the source of truth if this table and it ever disagree.

The companion computer side lives in [rover-ros2](https://github.com/SHC-ASTRA/rover-ros2). Its `connector.py` reaches the MCUs either through [an MCU's serial relay](#serial-relay) or by joining the CAN bus directly via a USB-CAN adapter.

## Usage in PlatformIO

Everything here targets **ESP32** under the Arduino framework — currently the Adafruit Feather ESP32 V2, ESP32 DOIT Devkit V1, and the ESP32-S3 DevKitC. `AstraCAN.h` also carries a Teensy/FlexCAN_T4 path, but nothing on the rover currently uses it and it goes untested; treat it as unmaintained.

### Adding to an existing PlatformIO project

1.  Add the library to `lib_deps` in your `platformio.ini`, pinned to a release tag:

    ```ini
    lib_deps =
        https://github.com/SHC-ASTRA/astra-embedded-lib#v2.0.0
    ```

    Releases are git tags. Pin one rather than tracking the branch, so an old release of an embedded project keeps building after breaking changes land on main.

2.  Add a build flag naming which submodule this MCU is. Exactly one of `CORE`, `ARM`, `DIGIT`, `LANCE`, or `CITADEL`:

    ```ini
    build_flags =
        -D CORE
    ```

    This sets `SUBMODULE_CAN_ID`, which is the MCU's address on the VicCAN bus. Leave it out and the build still succeeds, but the MCU answers only to broadcast messages and identifies itself as a broadcast — so it looks alive while ignoring everything addressed to it.

3.  Include the headers you need. E.g., `#include "AstraVicCAN.h"`
4.  Add each header's own external dependencies to `lib_deps` — see [Library contents](#library-contents). A header missing one will stop the build with a message naming the exact line to add.

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

### Starting a new PlatformIO project

Both files you need are in [`examples/Template/`](https://github.com/SHC-ASTRA/astra-embedded-lib/tree/main/examples/Template) — grab them from GitHub, since the local copy at `.pio/libdeps/[env]/astra-Embedded-Lib/examples/Template/` doesn't exist until you've built once.

1.  Make the project directory and drop in `platformio.ini` and `Template.cpp` (rename it to `src/main.cpp`). If you'd rather start from PlatformIO's own skeleton, `pio project init --board adafruit_feather_esp32_v2` does that, then overwrite its `platformio.ini`.
2.  Edit the four marked lines in `platformio.ini`: the env name, `board`, your submodule build flag, and whichever `lib_deps` you don't need. The comments in the file say what each one does.
3.  Build once — `pio run` — to confirm the dependencies resolve before you write any code.
4.  Include whichever headers you need from `include/`.
5.  Get writing!

The Template already handles `ping`, `time`, and `led` over serial, so a fresh board is testable the moment it's flashed. See [Common serial commands](#common-serial-commands).

### Build flags

These are every flag this library reads. Anything else you see in an ASTRA `platformio.ini` — `MAINMCU`, `TESTBED`, `FLIPSKY`, `DEBUG` — belongs to that firmware project, not this library.

| Flag                                           | Effect                                                                                           |
| ---------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| `CORE` / `ARM` / `DIGIT` / `LANCE` / `CITADEL` | Sets `SUBMODULE_CAN_ID`, this MCU's VicCAN address. Effectively required; see above.             |
| `FEEDBACK_PRECISION`                           | Decimal places used when relaying numbers to `Serial`. Defaults to 7 for GNSS lat/lon precision. |
| `VICCAN_DEBUG`                                 | Prints every VicCAN frame in and out to `Serial`. Debugging only.                                |
| `STOPWATCH_PRINT`                              | Makes `Stopwatch_t` print on every `start()`, `lap()`, and `stop()`. Off by default.             |

These macros can be set in `platformio.ini` with a `-D` statement in `build_flags`. See the example from Core above.

### Developing against a local checkout

When you're changing this library and a submodule's firmware together, separately cloning this library is a lot easier than trying to edit the copy PlatformIO downloads as a dependency. Add a second environment that points at your local clones instead of GitHub:

```ini
[env:core_main_dev]
; ...everything else identical to core_main_prod...
lib_deps =
	symlink://../../astra-embedded-lib
	symlink://../../unilib
	handmade0octopus/ESP32-TWAI-CAN@^1.0.1
	; ...remaining external dependencies unchanged...
```

Both paths are relative to the project directory, so this assumes `astra-embedded-lib`, `unilib`, and the firmware repo are all checked out as siblings. Note that `unilib` has to be listed explicitly here — the local checkout doesn't resolve the dependency from `library.json` for you.

Keep the `prod` environment around so you can always flash MCUs with `main`'s code.

### Flashing

Uploading is the ordinary PlatformIO flow — the upload arrow in the VS Code toolbar, or `pio run -t upload -e core_main_prod` from a shell. You shouldn't need to press a BOOT button.

Getting PlatformIO installed in the first place is the part that varies, and it's where people actually get stuck. Pick one of these:

- **VS Code + the PlatformIO IDE extension.** Install it from the marketplace and it brings its own toolchain along. Easiest if you're already working in VS Code.
- **Nix.** Every firmware repo ships a `flake.nix` with `platformio` in it, so `nix develop` in the repo root drops you in a shell with everything on your path — no VS Code involved. `.envrc` also hooks into direnv if you have it setup. Each shell prints its own upload commands on entry, e.g. `pio run -d core_main -e core_main_prod -t upload`.

Two things to decide deliberately before you hit upload:

- **Which environment.** `prod` uses the pinned release of `astra-embedded-lib`; `dev` requires and uses a local checkout.
- **Whose code.** `main`, unless you're testing your own work or you have a feature that's finished and tested but not merged yet.

### Updating your libraries

PlatformIO provides a button which will check for updates in all of your dependencies and update them for you. Here's how to take advantage of it:

- Open the PlatformIO side-bar (the alien on the left)
- Under "Project Tasks" (the top pane), for each environment specified in your `platformio.ini`, there are a handful of folders. "General" and "Platform" should be open already.
- Open the "Dependencies" folder.
- Click "Update".

## Using VicCAN

VicCAN is how ASTRA's MCUs and the rover's companion computer talk to each other. Every MCU sits on one CAN bus, and a message consists of:

- A command ID: 0-63 (6 bits)
- An addressed MCU (e.g., Core)
- 0-4 numbers, floating point or `int16` depending on how many.

The command list is shared with ROS2 through [unilib](https://github.com/SHC-ASTRA/unilib), so both sides of the rover agree on what command 48 means without anyone copying a number by hand. (Migration in progress; this repo is fully migrated to unilib, rover-ros2 is in progress.)

- Command IDs, MCU IDs, and payload types: `unilib/can_defs.hpp`
- What each command actually does, and its arguments: [the VicCAN spreadsheet](https://docs.google.com/spreadsheets/d/1jHHier_8mMmTDISywsfqXBYcWRiQC8k8O-At4GGrdkI/edit)

`AstraVicCAN.h` gives you one global object, `vicCAN`. Which MCU it answers as comes from your submodule build flag — see [step 2 of the setup above](#adding-to-an-existing-platformio-project).

### Bringing up the bus

**This library never starts the CAN peripheral — your code does.** One line in `setup()`, after `Serial.begin()`:

```cpp
if (ESP32Can.begin(TWAI_SPEED_1000KBPS, CAN_TX, CAN_RX))
    Serial.println("CAN bus started!");
else
    Serial.println("CAN bus failed!");
```

`ESP32Can` comes from `AstraVicCAN.h` — it includes `AstraCAN.h` for you when a CAN library is available, so there's nothing extra to include.

- **`TWAI_SPEED_1000KBPS` is required.** 1 Mbit/s is the rate every ASTRA MCU and REV Sparkmax runs, and CAN gives you nothing if the two ends disagree.
- **`CAN_TX` / `CAN_RX` are yours to define**, not the library's. Use the ones for your PCB, and double-check it; swapping these or using the wrong pins entire is an easy, common issue. Transceiver (TJA1051T/3) and MCU pins do not swap like UART; TX -> TX, RX -> RX.

### Reading commands

Poll `readCan()` in `loop()`. It returns true only for frames addressed to this MCU or broadcast to everyone; frames for other submodules are skipped for you (or relayed; see [below](#serial-relay)).

```cpp
bool isREV;
CanFrame rxFrame;

if (vicCAN.readCan(&isREV, &rxFrame)) {
    const uint8_t commandID = vicCAN.getCmdId();
    std::vector<double> canData;
    vicCAN.parseData(canData);  // Payload as 0-4 doubles, whatever it was encoded as

    if (commandID == CMD_PING) {
        vicCAN.respond(1);  // "pong" — reuses the command ID we just received
    } else if (commandID == CMD_REV_SET_DUTY) {
        if (canData.size() == 2) {  // Always check the size before indexing
            leftMotor.setDuty(canData[0]);
            rightMotor.setDuty(canData[1]);
        }
    }
}
```

The arguments for `readCan()` can be optionally excluded if you don't need the functionality.

`isREV` is the escape hatch for sharing the bus with REV SparkMaxes. Those use extended 29-bit IDs, which aren't VicCAN — when one arrives, `readCan()` returns **false**, sets `isREV` true, and leaves the raw frame in `rxFrame` for you to handle yourself.

### Sending data

The overload you get is chosen by **how many arguments you pass**, not by their type:

```cpp
vicCAN.send(CMD_GNSS_LAT, latitude);                    // 1 arg  -> one double
vicCAN.send(CMD_GNSS_SAT, satCount, fixType);           // 2 args -> two floats
vicCAN.send(CMD_DATA_BMP, temp, altitude, pressure);    // 3-4 args -> four int16's, truncated
```

The third line is the one you have to be conscious of: three or four arguments always means "four `int16_t`'s", so floats get truncated. Scale first if you need the precision, keeping in mind the limits for `int16`: [-32,768, 32,767]; for example, Core sends voltages as `vBatt * 100`; rover-ros2 divides on receive.

`respond()` takes the same arguments as `send()` but reuses the command ID of the frame you just read, which is what you want for anything request/response.

### Serial relay

An MCU can bridge the CAN bus to its USB serial port; this allows the rover's companion computer to effectively join the CAN bus without a discrete USB-CAN converter, and is also very useful for debugging. The serial relay works regardless of what computer is plugged into which MCU; for example, you can plug your laptop into Digit (on the end of arm), and simultaneously send control commands to Core and read feedback from every MCU.

`vicCAN.send()` will always send a message to the CAN bus, and a message directed at one MCU will never be acted upon by another. A control message sent over Serial to a MCU it wasn't directed to will always be silently and automatically relayed to the CAN network. Relay mode changes two things. First, the MCU echoes its own outgoing feedback to Serial as well as putting it on the bus. Second, when it sees a message on the CAN bus addressed to a different MCU, it relays that message to Serial rather than dropping it — which is what lets one USB connection read the whole bus.

One note: broadcast frames on the CAN bus are never relayed to Serial; they will be acted upon by the MCU instead, and can be seen with `vicCAN.printFrame(&Serial)`.

The syntax for the Serial-side of relay mode is simple: `can_relay_<to/from>vic,<mcu>,<cmdId>[,data...]`. For example:

- `can_relay_tovic,core,19,0.4,0.4` - commands Core to drive forward at 40% duty cycle.
- `can_relay_fromvic,core,48,34.7227120` - feedback from Core with Optics's GNSS latitude.

To interact with/control relay mode, use the following functions:

- `vicCAN.relayOn()` / `vicCAN.relayOff()` — enable or disable relay mode. These are exposed to Serial as so, and can be used by a computer to identify the plugged in MCU:
  - `can_relay_mode,on` - enables relay mode. The MCU will respond with `can_relay_ready,<name>`.
  - `can_relay_mode,off` - disables relay mode. The MCU will respond with `can_relay_off,<name>`.

  That handshake is how `rover-ros2` identifies the rover's MCUs. Its `anchor` node writes `can_relay_mode,on` to each USB device and expects `can_relay_ready,<name>` back — that exact format, with `<name>` being whatever `mcuIdToString()` returns, which is your `-D` build flag in lower case. Change either side and MCU discovery breaks. A new board also won't be probed at all until its USB VID/PID is added to `anchor`'s known device list.

- `vicCAN.relayFromSerial(args)` — hand it a `can_relay_tovic,<mcu>,<cmdId>[,data...]` line that's already been through `parseInput()`. If the frame is for this MCU it gets queued for the next `readCan()`; otherwise it gets directly relayed onto the CAN bus.

Wire both up in your serial command handling, the way `core` does:

```cpp
if (command == "can_relay_tovic") {
    vicCAN.relayFromSerial(args);
} else if (command == "can_relay_mode" && args.size() == 2) {
    if (args[1] == "on")
        vicCAN.relayOn();
    else if (args[1] == "off")
        vicCAN.relayOff();
}
```

On a board with no CAN library available, all of the above still compiles and runs — everything just goes to `Serial` only.

### Safety timeouts

VicCAN solves exactly one problem: message passing. Failsafes sit deliberately outside that scope; they live in each MCU's own code, next to the hardware they protect, because that's the only code still running when things go wrong. The NUC can hard crash on a power failure and a USB or CAN cable can come out mid-command; the MCU on the far end has to know what to do on its own. Every ASTRA MCU that moves something is written that way, so a command that has been received is never in effect indefinitely.

The pattern `core` uses:

```cpp
// Stop the motors if no host control command has been received within this many ms.
#define HOST_CMD_TIMEOUT_MS 500

unsigned long lastCtrlCmd = 0;

// ...in every command handler that causes motion:
lastCtrlCmd = millis();

// ...on a timer in loop(), next to accelerate():
if (millis() - lastCtrlCmd > HOST_CMD_TIMEOUT_MS) {
    Stop();
}
```

Pick the timeout to suit what you're driving — 500 ms is core's number for a drive base, not a library constant (yet).

This is separate from the SparkMax heartbeat, which covers a different link. `CAN_sendHeartbeat(deviceId)` satisfies each REV controller's own failsafe — roughly every 25 ms, and `core` cycles IDs 1–4 from a second task every 5 ms. That one protects MCU-to-motor; the timeout above protects basestation-to-MCU.

## Talking to an MCU over serial

Alongside VicCAN, every ASTRA MCU exposes a plain-text command interface on USB serial for debugging. It's an easy way to check if a board is alive or test specific features with functionality not meant for competition code.

### Common serial commands

Command handling uses `parseInput()` to split an incoming Serial string on commas and the first token acts as the main command determiner. The Template ships the first three commands, so they work on any board no matter which submodule it is:

| Command                                    | Does                                                                                       |
| ------------------------------------------ | ------------------------------------------------------------------------------------------ |
| `ping`                                     | Replies `pong`. Fastest way to tell whether an MCU is alive and you're at the right baud.  |
| `time`                                     | Replies with `millis()` since boot. A good sanity check for your Serial interface.         |
| `led,on` / `led,off` / `led,toggle`        | Drives `LED_BUILTIN`. Performs a physical action regardless of whether your RX is working. |
| `can_relay_mode,on` / `can_relay_mode,off` | Relay mode — see [Serial relay](#serial-relay).                                            |
| `can_relay_tovic,<mcu>,<cmdId>[,data...]`  | Inject a VicCAN frame — see [Serial relay](#serial-relay).                                 |

Each submodule adds its own on top: `lance` has `drill` and `linac`, `digit` has `laser` and `zero`. These are for debugging — read that project's `main.cpp` for more information.

### Testing CAN without CAN

The relay is plain text over USB serial, so you can exercise a VicCAN handler with nothing but a serial terminal — no ROS2, no CAN bus, no second MCU.

```text
can_relay_mode,on                # replies: can_relay_ready,core
can_relay_tovic,core,1           # CMD_PING -> can_relay_fromvic,core,1,1.0000000
can_relay_tovic,core,19,0,0      # drive at 0% duty; turn the numbers up to physically drive
```

Addressed to the MCU you're plugged into, the frame gets queued and handled locally. Addressed to anything else, it goes out on the bus — so one USB cable reaches every submodule.

Whichever terminal you use, it has to send a newline at the end of each line. The MCU reads up to `\n` and will sit there waiting if your terminal only sends a carriage return.

- **VS Code "Serial Monitor" extension** — easiest if you're already in VS Code. Does not work if VS Code was installed as a Flatpak.
- **tio** — `tio /dev/ttyACM0`. Exit with `Ctrl-T` then `q`.
- **GNU Screen** — `screen /dev/ttyACM0 115200`. Exit with `Ctrl-A` then `k`, `y` to confirm.
- **PlatformIO's monitor** — `pio device monitor -e core_main_dev`. Picks up the `monitor_*` settings from your environment, which is the one thing it has going for it.

For the other side of the link, [rover-ros2](https://github.com/SHC-ASTRA/rover-ros2) has the mirror-image tooling: a `socat` pty pair that fakes a serial MCU and a virtual `vcan0` interface. See its README for more information.

## Build-time version info

`extra_script.py` runs on every build (PlatformIO picks it up from `library.json`) and injects git state as compile-time defines, so a flashed MCU can tell you exactly what it is running. Two sets get defined — `ASTRA_LIB_VERSION_*` from this library's checkout, and `PROJECT_VERSION_*` from your firmware repo:

| Define                                   | Source                                                                         |
| ---------------------------------------- | ------------------------------------------------------------------------------ |
| `*_VERSION_MAJOR` / `_MINOR` / `_PATCH`  | Latest git tag, e.g. `1`, `0`, `2` for `v1.0.2`.                               |
| `*_VERSION_ISMAIN`                       | 1 if built from the `main` branch                                              |
| `*_VERSION_ISDIRTY`                      | 1 if the working tree had uncommitted changes                                  |
| `*_VERSION_COMMIT_HASH_LOWER` / `_UPPER` | Short commit hash, split into two `int16_t` halves so it fits a VicCAN payload |

`AstraMisc.h` folds these together with `BUILD_TIMESTAMP` (seconds since 2022-01-01, computed from `__DATE__` and `__TIME__` at compile time) into a single macro:

```cpp
SEND_VERSION_INFO
```

It sends two VicCAN frames — `CMD_VERSION_COMMIT` with both commit hashes, and `CMD_VERSION_BUILD` with the build timestamp and a bitfield of the four main/dirty flags. Call it on a timer in your `loop()`. It's a macro rather than a function because `AstraVicCAN.h` is header-only, so `vicCAN` only exists in the translation unit that included it.

**The script identifies each repo by its directory name.** The libraries must end in `-embedded-lib`; the firmware projects must contain `-embedded` or start with `rover-`. Clone into a differently-named directory and the build still succeeds, but every version field silently becomes zero — see [Troubleshooting](#troubleshooting).

## Troubleshooting

**`Missing library! Please add the following line to lib_deps in platformio.ini:  <something>`**

Do what it says — copy the line into `lib_deps` and rebuild. The header you included needs an external library your project doesn't have yet. `AstraSensors.h` will report all four of its dependencies at once, so read the whole output before you start adding lines one at a time.

**`No submodule defined in platformio.ini; will only respond to broadcast messages`**

You're missing your MCU's macro (e.g., `ARM`, `DIGIT`, `LANCE`, `CITADEL`). The firmware will flash and run, and it will look healthy, but it ignores every VicCAN message addressed specifically to it.

**`Could not find a compatible CAN library. VicCAN will be limited to Serial use only.`**

`AstraVicCAN.h` couldn't find `ESP32-TWAI-CAN`, so it built the serial-only path — `vicCAN.send()` and friends still work, they just go to `Serial` instead of the CAN bus. Intentional on a board with no transceiver. If you did want CAN, add `handmade0octopus/ESP32-TWAI-CAN@^1.0.1` to `lib_deps`.

**`If you are seeing this in the code, just build the project. If you are seeing this at compile time, ask David...`**

`AstraMisc.h` needs the defines from `extra_script.py`, which only exist during a real PlatformIO build. Your editor's language server showing this is normal and harmless — don't worry about it. At actual compile time it means the build hook didn't run.

**`PROJECT repository name mismatch. crying.`** (or `ASTRA_LIB repository name mismatch. crying.`)

The directory name doesn't match what `extra_script.py` expects, so it zeroed the version defines. The build succeeds and `SEND_VERSION_INFO` reports all zeros. Rename the directory to end in `-embedded-lib`, or to contain `-embedded`, depending on which one it's crying about. Cloning a repo under a different name than upstream is the usual cause.

**`VICCAN_DEBUG is enabled. good luck soldier.`**

You turned on VicCAN's debug output. Every frame in and out gets printed to `Serial`, which is loud enough to change timing. Fine while debugging, don't ship it.

**`Raspberry Pi Pico is not supported`**

Correct, it isn't. Use an ESP32.

## Adding a new header

1.  Put the header in `include/` and its implementation in `src/`. Header-only is fine when there is nothing to compile separately.
2.  Name both files in camel case with every word capitalized, including the first, prefixed with `Astra`. Ex: `AstraMotors.h` / `AstraMotors.cpp`
3.  If the header needs an external Arduino library, guard it with `__has_include` and fail with an `#error` naming the exact `lib_deps` line to add — see `AstraSensors.h`. A missing dependency should hand you the fix, not bury you in vague include errors.
4.  If the header can do something useful without that library, `#warning` and a feature macro are better than an `#error`. `AstraVicCAN.h` does this: with no CAN library available it defines everything anyway and runs over serial only.
5.  Add it to [Library contents](#library-contents) above.

Those guards are what let all of ASTRA's shared code live in one library without every project carrying every dependency. They only apply to headers you actually include, so a project pulls in exactly the external libraries it uses.

## Header reference

The most-used symbols from each header. Everything is documented in place — open the header for full signatures and the reasoning behind anything surprising.

### `AstraMisc.h`

- `SERIAL_BAUD`, `COMMS_UART_BAUD`, `LSS_BAUD`, `CMD_DELIM` — team-wide comms constants. Use these instead of writing `115200` anywhere.
- `parseInput(input, args)` — splits a `String` on `CMD_DELIM` (commas) into a `std::vector<String>`.
- `checkArgs(args, numArgs)` — true if the right number of arguments came in. Guard your command handlers with it before indexing `args`.
- `Timer` — `lastMillis` / `interval` / `state` in one struct, for the `millis()` polling pattern.
- `Stopwatch_t` — `start()`, `lap()`, `stop()` in microseconds, as a convenient tool for timing evaluation.
- `map_d(x, in_min, in_max, out_min, out_max)` — `map()` in doubles; returns 0 rather than dividing by zero on an empty input range.
- `convertADC(reading, r1, r2)` — ADC counts to volts through a divider, resistances in kΩ.
- `SEND_VERSION_INFO`, `BUILD_TIMESTAMP` — see [Build-time version info](#build-time-version-info).

### `AstraVicCAN.h`

See [Using VicCAN](#using-viccan). The header also exposes `VicCanFrame` if you need to build or inspect frames directly, and `FEEDBACK_PRECISION` (decimal places used when relaying to `Serial`, default 7) is overridable with a build flag.

### `AstraCAN.h`

- Includes the right CAN library for the target and gives you the bus object — `ESP32Can` on ESP32.
- `printCANframe(frame)` — dump a raw frame to `Serial` for debugging.

### `AstraREVCAN.h`

- `CAN_sendControl(deviceId, ctrlType, value)` — the main one. Duty cycle, velocity, position, etc., per `sparkMax_ctrlType`.
- `CAN_sendHeartbeat(deviceId)` — SparkMaxes cut output without a regular heartbeat (~25 ms). If your motors twitch and stop, this may be why. This frequency needs to be called from a thread separate from `loop()`.
- `CAN_enumerate()` — broadcast that every SparkMax answers, staggered by its ID. Use it to find out what's actually on the bus.
- `CAN_identifySparkMax(deviceId)` — blinks one controller's LED, for working out which is which and testing connectivity.
- `CAN_setParameter()` / `CAN_reqParameter()` / `CAN_setStatusPeriod()` — configuration and status frame rates.
- `printREVFrame(frame)` / `printREVParameter(rxFrame)` — decode REV traffic to `Serial`, for debugging.

### `AstraREVTypes.h`

Enums matching REV's protocol: `sparkMax_ctrlType`, `sparkMax_IdleMode` (brake/coast), `sparkMax_faultID`, `sparkMax_ConfigParameter`, `sparkMax_PeriodicFrame`, plus the `motorStatus0/1/2` structs that status frames decode into.

### `AstraMotors.h`

One `AstraMotors` per physical motor: `AstraMotors(revId, inverted, gearBox)` — REV ID, inversion (currently a no-op), and gearbox ratio as a plain integer (64 for 64:1).

A motor starts in duty-cycle mode and switches based on which method you call — `setDuty()` and `sendDuty()` put it in duty-cycle mode, `sendSpeed()` in velocity, `sendCurrent()` in current.

- `setDuty(val)` then `accelerate()` — ramps toward the target instead of stepping to it. Call `accelerate()` on a fast timer.
- `sendDuty(val)` — sends immediately, skipping the ramp.
- `setBrake(enable)`, `identify()`, `setSlowStatusPeriods()`
- `parseStatus(apiId, frameIn)` — feed it REV status frames; results land in the public `status0` / `status1` / `status2` members.

### `AstraNP.h`

- `AstraNeoPixel(pin)` — usually `PIN_NEOPIXEL`.
- `addStatus(status, duration)` — queue a status to display; holds up to 5. **`duration` is in seconds**, not milliseconds.
- `update()` — controls the physical Neopixel; run it on a timer at least 20 Hz.
- `writeColor(color)` — drive the Neopixel directly, for showing progress during `setup()`.
- `STATUS_IDLE`, `STATUS_BMP_NOCONN`, `STATUS_BNO_NOCONN`, `STATUS_GPS_NOCONN`, `STATUS_GPS_NOLOCK`, `STATUS_CAN_NOCONN` — pre-made two-color blink patterns.

### `AstraSensors.h`

- `pullBNOData(bno, bno_data[7])`, `getBNOOrient(bno)` — IMU readings and heading.
- `displayCalStatus(bno)`, `displaySensorStatus(bno)`, `displaySensorDetails(bno)`, `displaySensorOffsets(offsets)` — diagnostics to `Serial`.
- `initializeBMP(bmp)`, `pullBMPData(bmp, bmp_data[3])` — temperature, altitude, pressure.
- `getPosition(gnss, gps_data[4])`, `getUTC(gnss)` — GNSS fix and time.

## Maintainers

| Name         | Email            | Discord   |
| ------------ | ---------------- | --------- |
| David Sharpe | <ds0196@uah.edu> | `@ddavdd` |
