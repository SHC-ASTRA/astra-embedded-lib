# Project headers

Each PCB with a MCU on it gets its own project header.

## Files

* `CORE` - MCUs on Core PCB. Controls the REV motors corresponding to
the wheels of the rover and collects sensor data like IMU and GPS.
* `TESTBED` - MCUs on Testbed PCB. Mini version of Core. 14/04/25 Note: Testbed now uses Core PCB.
* `ARM` - MCUs on Socketboard. Controls the REV motors corresponding to
the arms' axes and Lynxmotion servos on Wrist (when attached).
Also relays power and CAN signals to Digit/LANCE.
* `DIGIT` - MCU on Digit PCB. Controls end effector.
* `LANCE` - MCU on LANCE PCB. Controls drill, vacuum, and SHT30 sensor. Replaces FAERIE.
* `CITADEL` - MCU on CITADEL PCB. Controls fans and pumps to facilitate
chemical sample testing as well as bio arm.
