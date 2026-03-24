/**
 * @file AstraMotors.h
 * @author Tristan McGinnis (tlm0047@uah.edu)
 * @brief Outlines class for controlling Rev Sparkmax motors
 *
 */
#pragma once

#include <cmath>

#include "AstraREVCAN.h"


class AstraMotors {
    int motorID;  // REV motor ID
    sparkMax_ctrlType controlMode;
    bool inverted;  // Inverts the speed of the motor, this should be true for right wheels

    int currentMotorSpeed;  // Current speed of the motor
    int maxSpeed;

    float currentDutyCycle;
    float targetDutyCycle;
    float dutyCycleAccel;
    float maxDuty;

    int gearBox;

    bool rotatingToPos;
    float targetPos;


   public:

    // Keep public for now for testing
    motorStatus0 status0;
    motorStatus1 status1;
    motorStatus2 status2;
    
    /**
     * @brief Default constructor for a REV motor controller
     *
     * @param setMotorID REV motor ID for this motor
     * @param SetInverted Whether or not to invert the motor's direction (used for right wheels)
     * @param SetGearBox Gearbox ratio attached to motor; e.g. for 64:1, use 64
     */
    AstraMotors(int setMotorID = 0, bool SetInverted = false, int setGearBox = 1);


    //---------------------------------------------//
    //  Getters
    //---------------------------------------------//
    
    inline sparkMax_ctrlType getControlMode() const {
        return controlMode;
    }
    // Get the current speed
    inline int getSpeed() const {
        return currentMotorSpeed;
    }
    inline float getDuty() const {
        return currentDutyCycle;
    }
    inline float getSetDuty() const {
        return targetDutyCycle;
    }
    // Get the motor's set CAN ID
    inline int getID() const {
        return motorID;
    }

    inline int getGearBox() const {
        return gearBox;
    }

    // Returns the direction in which the motor is spinning; either 0 (not moving), 1 (cw), or -1 (ccw)
    inline int direction() {
        if (controlMode == sparkMax_ctrlType::kDutyCycle) {
            if (currentDutyCycle == 0)
                return 0;
            else
                return (currentDutyCycle > 0) ? 1 : -1;
        } else {
            if (currentMotorSpeed == 0)
                return 0;
            else
                return (currentMotorSpeed > 0) ? 1 : -1;
        }
    }

    // Whether or not the motor is currently turning to a position using the internal encoder feedback
    // (from turnByDeg() or turnToDeg())
    inline bool isRotToPos() {
        return rotatingToPos;
    }


    //---------------------------------------------//
    //  Setters
    //---------------------------------------------//

    void setDuty(float val);  // Set the targetDutyCycle variable; will be enacted via accelerate()

    void parseStatus(uint32_t apiId, uint8_t frameIn[]);  // Parse a status frame from 8-byte CAN data and REV API ID
    void parseStatus0(uint8_t frameIn[]);
    void parseStatus1(uint8_t frameIn[]);
    void parseStatus2(uint8_t frameIn[]);


    //---------------------------------------------//
    //  Controlling physical motor
    //---------------------------------------------//
    
    // Send the identify command to the SparkMax; makes it flash its LED purple and white
    inline void identify() {
        CAN_identifySparkMax(motorID);
    }

    // Set idle mode for the motor; either brake (true) or coast (false)
    inline void setBrake(bool enable) {
        CAN_setParameter(motorID, sparkMax_ConfigParameter::kIdleMode, sparkMax_ParameterType::kUint32, static_cast<uint32_t>(enable));
    }

    // Send the currently tracked duty cycle (currentDutyCycle) to the motor
    inline void sendDuty() {
        CAN_sendControl(motorID, sparkMax_ctrlType::kDutyCycle, currentDutyCycle);
    }
    // Send the currently tracked speed (velocity; currentMotorSpeed) to the motor
    inline void sendSpeed() {
        CAN_sendControl(motorID, sparkMax_ctrlType::kVelocity, currentMotorSpeed);
    }
    
    void sendDuty(float val);   // Send this duty cycle to the motor (bypasses acceleration)
    void sendSpeed(float val);  // Send this speed to the motor (bypasses acceleration)

    // No acceleration for current control
    inline void sendCurrent(float val) {
        controlMode = sparkMax_ctrlType::kCurrent;
        CAN_sendControl(motorID, sparkMax_ctrlType::kCurrent, val);
    }

    void accelerate();          // Run UpdateForAcceleration() and sendDuty()

    void turnByDeg(float deg);  // Turn the motor by deg degrees
    void turnToDeg(float deg);  // Turn the motor to deg degrees
    
    // Stop motor; does not activate brake mode
    inline void stop() {
        rotatingToPos = false;
        sendDuty(0);
    }
};
