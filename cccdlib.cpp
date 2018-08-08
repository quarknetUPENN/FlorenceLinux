//
// Created by hep on 8/7/18.
//

#include <cstdio>
#include "cccdlib.h"

#define CHECK_BIT(var, pos) (((var)>>(pos)) & 1)

int cccd(volatile unsigned int * comms, enum cmd command, bool isRead, enum reg registers, int chipId) {
    if ((command != Reg) || (isRead == WR)) {
        printf("Invalid number of fields to perform a reg read!  Ignoring");
        return 1;
    }
    return cccd(comms, command, isRead, registers, chipId, 0);
}

int cccd(volatile unsigned int * comms, enum cmd command) {
    if (command == Reg) {
        printf("Invalid number of fields to perform a reg read or write!  Ignoring");
        return 1;
    }
    return cccd(comms, command, false, Config, 0, 0);
}

int cccd(volatile unsigned int * comms, enum cmd command, bool isRead, enum reg registers, int chipId, unsigned int payload) {
    if (chipId > CHIPID_ALL) {
        printf("Unrecognized chipId %u requested! Ignoring\n", chipId);
        return 1;
    }

    // set field15.  if it's a broadcast command things are easy
    unsigned int field15 = 0;
    if ((command == L1A) | (command == SoftRst) | (command == BxRst)) {
        field15 = command << 24;
        comms[5] = 0;
    } else if (command == Reg) {
        int cmdlength = 0;
        if (isRead) {
            cmdlength = 0b00001100;
            comms[5] = 0;
        } else {
            switch (registers) {
                case Config:
                    cmdlength = 0b00100100;
                    comms[5] = payload << 8;
                    break;
                case Status:
                    cmdlength = 0b00001100;
                    comms[5] = payload;
                    break;
                case Thresh1:
                    cmdlength = 0b00011100;
                    comms[5] = payload << 16;
                    break;
                case Thresh2:
                    cmdlength = 0b00011100;
                    comms[5] = payload << 16;
                    break;
                default:
                    cmdlength = 0;
                    comms[5] = 0;
                    printf("Unrecognized register %u requested! Ignoring\n", registers);
                    return 2;
            }
        }

        field15 = (command << 24) + (cmdlength << 16) + (chipId << 10) + ((isRead ? 1 : 0) << 9) + (registers << 4);
    } else {
        printf("Unrecognized command %u requested! Ignoring\n", command);
        return 3;
    }

    // send the command by toggling the trigger bit
    comms[0] = (0 << 31) + field15;
    comms[0] = (1 << 31) + field15;

    int delay = 0;
    while (true) {
        if (isRead) {
            if ((CHECK_BIT(comms[6], 1)) && (CHECK_BIT(comms[6], 0))) {
                break;
            }

        } else {
            if (CHECK_BIT(comms[6], 1)) {
                break;
            }
        }


        if (delay > 10000) {
            printf("ERROR: moving on, timed out trying to %s command %X\n", isRead ? "receive data from" : "send",
                   field15);
            return 4;
        }
        delay++;
    }
    return 0;
}

// note, printb will perform multiple reads from the address given
void printb(volatile void *ptr) {
    size_t const size = sizeof(ptr);
    auto b = (unsigned char *) ptr;
    unsigned char byte;
    int i, j;

    for (i = size - 1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}