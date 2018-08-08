//
// Created by hep on 8/7/18.
//

#ifndef FLORENCELINUX_CCCD_H
#define FLORENCELINUX_CCCD_H

#define WR 0
#define RD 1
#define CHIPID_ALL 0b111111

typedef enum cmd {
    L1A = 0b1100000,
    SoftRst = 0b1010100,
    BxRst = 0b1010010,
    Reg = 0b1010111
} command;

typedef enum reg {
    Config = 0b00000, Status = 0b00011, Thresh1 = 0b01010, Thresh2 = 0b01100
} registers;

int cccd(volatile unsigned int * comms, enum cmd command);

int cccd(volatile unsigned int * comms, enum cmd command, bool isRead, enum reg registers, int chipId);

int cccd(volatile unsigned int * comms, enum cmd command, bool isRead, enum reg registers, int chipId, unsigned int payload);

void printb(volatile void *ptr);

#endif //FLORENCELINUX_CCCD_H
