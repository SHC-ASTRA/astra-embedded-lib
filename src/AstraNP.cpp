/**
 * @file AstraNP.cpp
 * @author David Sharpe (ds0196@uah.edu)
 * @brief
 *
 */
#include "AstraNP.h"


AstraNeoPixel::AstraNeoPixel(int pNPPin) {
    npPin = pNPPin;

    for (int i = 0; i < 5; i++) {
        status[i] = STATUS_NA;
    }
    statusCount = 0;
    currentStatus = 0;
    limboStart = -LIMBO_TIME;
}


void AstraNeoPixel::addStatus(NPStatus pStatus, long pDuration) {
    if (statusCount == MAX_LEN) {  // we're cooked
        // Find the status closest to ending
        int closestIndex = 0;
        long closestTime = status[0].startTime + status[0].duration;

        for (int i = 1; i < MAX_LEN; i++) {
            long endTime = status[i].startTime + status[i].duration;
            if (endTime < closestTime) {
                closestTime = endTime;
                closestIndex = i;
            }
        }

        // nuke that shit
        for (int i = closestIndex; i < MAX_LEN - 1; i++) {
            status[i] = status[i + 1];
        }
        statusCount--;
    }

    // Move everything right
    for (int i = statusCount; i > 0; i--) {
        status[i] = status[i - 1];
    }
    status[0] = pStatus;
    status[0].duration = pDuration * 1000;
    status[0].addTime = millis();
    status[0].startTime = status[0].addTime;
    statusCount++;
}


void AstraNeoPixel::update() {
    long currTime = millis();

    if (currTime < limboStart + LIMBO_TIME) {
        writeColor(0x0);
        return;  // we in limbo bro
    }

    if (statusCount == 0) {
        addStatus(STATUS_IDLE, 0);  // No status, so go to idle
    }

    const long realStartTime = status[currentStatus].startTime;
    const long onTime = status[currentStatus].onTime;
    const long offTime = status[currentStatus].offTime;

    const long startOfSecCycle = realStartTime + onTime * 2 + offTime * (1 + BET_CYCLE_LEN);
    const long statusEndTime = startOfSecCycle + onTime * 2 + offTime;

    // if still on current status
    if (currTime < statusEndTime) {
        float startTime;                                             // Start time for current cycle
        if (currTime < startOfSecCycle - offTime * BET_CYCLE_LEN) {  // first cycle
            startTime = realStartTime;
        } else if (currTime < startOfSecCycle) {  // between cycles
            writeColor(0x0);
            return;
        } else {  // second cycle
            startTime = startOfSecCycle;
        }

        if (currTime < startTime + onTime) {  // first color
            writeColor(status[currentStatus].color1);
        } else if (currTime < startTime + onTime + offTime) {  // off time
            writeColor(0x0);
        } else {  // second color
            writeColor(status[currentStatus].color2);
        }

        return;  // Stay on this status until finished, don't check or move anything
    }

    writeColor(0x0);

    // Check for any expired statuses
    // Wait to do this until after the current status is finished so we don't cancel a status mid-blink
    // This really should just be a List class, but whatever
    for (int i = 0; i < statusCount; i++) {
        if (currTime > status[i].addTime + status[i].duration) {  // Found one
            for (int j = i; j < statusCount - 1; j++) {  // Move everything on the right side to the left
                status[j] = status[j + 1];
            }
            statusCount--;
            i--;
        }
    }

    // Move to next status
    currentStatus++;
    if (currentStatus > statusCount - 1)
        currentStatus = 0;

    status[currentStatus].startTime = currTime + LIMBO_TIME;  // Next status will start after limbo

    // Enter limbo
    limboStart = currTime;
}
