/**
 * @file AstraMotors.cpp
 * @author Tristan McGinnis (tlm0047@uah.edu)
 * @brief Implements class for controlling Rev Sparkmax motors
 *
 */

#if __has_include("ESP32-TWAI-CAN.hpp") || __has_include("FlexCAN_T4.h")

#    include "AstraMotors.h"


AstraMotors::AstraMotors(int setMotorID, bool SetInverted, int setGearBox) {
    motorID = setMotorID;
    controlMode = sparkMax_ctrlType::kDutyCycle;
    inverted = SetInverted;

    currentMotorSpeed = 0;
    maxSpeed = 10000;

    currentDutyCycle = 0;
    targetDutyCycle = 0;
    dutyCycleAccel = 0.05;
    maxDuty = 1;

    gearBox = setGearBox;
    rotatingToPos = false;
}

AstraMotors::AstraMotors(int setMotorID, sparkMax_ctrlType setCtrlMode, bool SetInverted, int setGearBox) {
    AstraMotors(setMotorID, SetInverted, setGearBox);
}


void AstraMotors::setDuty(float val) {  // controller input value
    controlMode = sparkMax_ctrlType::kDutyCycle;
    if (abs(val) <= 0.02) {
        targetDutyCycle = 0;
    } else {
        targetDutyCycle = val;
    }
}


void AstraMotors::sendDuty(float val) {
    setDuty(val);
    currentDutyCycle = targetDutyCycle;
    sendDuty();
}

void AstraMotors::sendSpeed(float val) {
    controlMode = sparkMax_ctrlType::kVelocity;
    if (abs(val) <= 0.02)
        val = 0;
    else if (val > maxSpeed)
        val = maxSpeed;
    else if (val < -maxSpeed)
        val = -maxSpeed;

    currentMotorSpeed = val;
    sendSpeed();  // Send currentMotorSpeed to motors
}

void AstraMotors::accelerate() {
    // Acceleration only applies to duty cycle
    if (controlMode != sparkMax_ctrlType::kDutyCycle)
        return;

    if (targetDutyCycle == 0)
        currentDutyCycle = 0;

    // rotToPos control
    if (rotatingToPos) {
        if (millis() - status2.timestamp > 100
            || (direction() == 1 && status2.sensorPosition > targetPos - 1)
            || (direction() == -1 && status2.sensorPosition < targetPos + 1))
        {
            stop();
#   ifdef MOTOR_DEBUG
            Serial.println("Stopping rotation.");
#   endif
        }
        return;
    }
    // Regular (linear) acceleration
    else if (targetDutyCycle != currentDutyCycle) {
        const float threshold = 0.1;
        const float current = currentDutyCycle;
        const float target = targetDutyCycle;

        if (abs(target - current) <= threshold) {  // if within threshold, just set it, don't gradually accelerate
            currentDutyCycle = targetDutyCycle;
        } else if (current < target - threshold) {  // increment if below set
            currentDutyCycle += dutyCycleAccel;
        } else if (current > target + threshold) {  // decrement if above set
            currentDutyCycle -= dutyCycleAccel;
        } else {
            if ((current > 0 && target < 0) ||
                (current < 0 && target > 0))  // if sticks in opposite direction, quick stop
            {
                currentDutyCycle = 0;
                targetDutyCycle = 0;
            }
            currentDutyCycle = 0;
        }
    }

    sendDuty();
}


void AstraMotors::parseStatus(uint32_t apiId, uint8_t frameIn[]) {
    if (apiId == 0x60) {
        parseStatus0(frameIn);
    } else if (apiId == 0x61) {
        parseStatus1(frameIn);
    } else if (apiId == 0x62) {
        parseStatus2(frameIn);
    }
}

void AstraMotors::parseStatus0(uint8_t frameIn[]) {
    // (A) Applied output is 16-bit and comes from [0] and [1]
    uint16_t outputMSB = (frameIn[1] << 8);  // Full byte from [1]
    uint16_t outputLSB = frameIn[0];  // Full byte from [0]
    status0.appliedOutput = static_cast<float>(outputMSB | outputLSB) / 32767.0;  // No re-interpret

    // (bits) Faults are 16-bit and come from [2] and [3]
    status0.faults = (frameIn[3] << 8) | frameIn[2];

    // (bits) Sticky faults are 16-bit and come from [4] and [5]
    status0.stickyFaults = (frameIn[5] << 8) | frameIn[4];

    // (bits) Lock is a dedicated bit in the data frame (7)
    status0.lock = (frameIn[6] >> 2) & 0x3;

    // (bit) Inverted is a dedicated bit in the data frame (7)
    status0.isInverted = (frameIn[6] >> 1) & 0x1;

    // (ms) Timestamp
    status0.timestamp = millis();
}

void AstraMotors::parseStatus1(uint8_t frameIn[]) {
    // (RPM) Motor velocity is the first 32-bits, little endian IEEE 754 float
    uint32_t motorVel = (frameIn[3] << 24) | (frameIn[2] << 16) | (frameIn[1] << 8) | frameIn[0];
    status1.sensorVelocity = *reinterpret_cast<float*>(&motorVel);

    // (*C) Motor temperature is a dedicated byte in the data frame (4).
    status1.motorTemperature = frameIn[4];

    // (V) Motor voltage is 12-bit and comes from [5] and [6]
    uint16_t voltageMSB = ((static_cast<uint16_t>(frameIn[6]) & 0xF) << 8);  // Lower 4 bits from [6]
    uint16_t voltageLSB = frameIn[5];  // Full byte from [5]
    status1.busVoltage = static_cast<float>(voltageMSB | voltageLSB) / 128.0;  // No re-interpret

    // (A) Motor current is 12-bit and comes from [6] and [7]
    uint16_t currentMSB = (static_cast<uint16_t>(frameIn[7]) << 4);  // Full byte from [7]
    uint16_t currentLSB = ((static_cast<uint16_t>(frameIn[6]) & 0xF0) >> 4);  // Upper 4 bits from [6]
    status1.outputCurrent = static_cast<float>(currentMSB | currentLSB) / 32.0;  // No re-interpret

    // (ms) Timestamp
    status1.timestamp = millis();
}

void AstraMotors::parseStatus2(uint8_t frameIn[]) {
    // (rotations) Motor position is the first 32-bits, little endian IEEE 754 float
    uint32_t motorPos = (frameIn[3] << 24) | (frameIn[2] << 16) | (frameIn[1] << 8) | frameIn[0];
    status2.sensorPosition = *reinterpret_cast<float*>(&motorPos);

    // Ignore second half of frame

    // (ms) Timestamp
    status2.timestamp = millis();
}

// I made this guy to help drive the rover a specific distance, but with ros2_control and Nav2,
// it will be better anyways to use inertial and visual odometry to control distance driven
// with discrete velocity control rather than using the motor's built-in encoder when
// driving for hundreds of meters...
[[deprecated("Use closed-loop control with external sensors instead.")]]
void AstraMotors::turnByDeg(float deg) {
    rotatingToPos = true;
    targetPos = status2.sensorPosition + ((deg / 360.0) * gearBox);

#ifdef MOTOR_DEBUG
    Serial.print("Turning to pos: ");
    Serial.println(targetPos);
#endif

    if (controlMode == sparkMax_ctrlType::kDutyCycle) {
        const float dutyCycle = 0.5;  // Arbitrary for now
        if (deg < 0)
            sendDuty(dutyCycle);
        else
            sendDuty(-1 * dutyCycle);
    } else if (controlMode == sparkMax_ctrlType::kVelocity) {
        const float speed = 100;  // Arbitrary for now
        if (deg < 0)
            sendSpeed(speed);
        else
            sendSpeed(-1 * speed);
    }
}

#endif  // __has_include("FlexCAN_T4.h")
