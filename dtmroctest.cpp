#include <iostream>
#include <fstream>
#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <string>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include "cccdlib.h"
#include "l1arecv.h"
#include "dtmroctest.h"

//copied from l1arecv 07/14/21
//testing program for DTMROC boards

volatile unsigned int * comms;
volatile unsigned int * tdc;
volatile unsigned int * dips;

int mmapAxiSlaves();

int munmapAxiSlaves();

int testReg(enum reg registers, int id, int value);

int testHardRst();

int main(int argc, char *argv[]) {
    int id;
    bool isBistFinished;
    
    printf("Memory mapping the axi slaves...");
    if (mmapAxiSlaves() != 0) {
        fprintf(stderr, "Failed!  Exiting");
            //TODO: if mmap failed, print error and do not log data
        return 1;
    }
    printf("done\n");

    // open log file

    // get board number input
    
    
    // get voltage input
    
    for(id = 62; id <= 63; id++){
        //id may get changed later, need manual changing though so idk how
        
        // w/r config reg
        testReg(Config, id, 0xAAAAAA);
        //testReg(Config, 63, 0x555515); //bit 6 turns on BIST, so it cannot be set to 1 to ensure normal operation
        testReg(Config, id, 0x000001);
       
        // w/r thresh 1
        testReg(Thresh1, id, 0xAAAA);
        testReg(Thresh1, id, 0x5555);
        
        // w/r thresh 2
        testReg(Thresh2, id, 0xAAAA);
        testReg(Thresh2, id, 0x5555);
        
        // read status reg (BIST)
        cccd(comms, Reg, WR, Config, id, 0x40);     //start bist
        do {
            cccd(comms, Reg, RD, Status, id);   
            //bit 8 and 10 are progress flags, is 1 when finished 
            isBistFinished = (comms[10] & 0b100000000) & (comms[10] & 0b10000000000);       
            printf("finished: %X\n", isBistFinished);
        }while(isBistFinished == false);
        
        
    }


    
    // test reset line
    // read config, thresh 1, 2, status again
    
    // write to log file + print to terminal
    
    // close file
    
    printf("Unmapping the axi slaves from memory...");
    if (munmapAxiSlaves() != 0) {
        fprintf(stderr, "Failed to unmap memory for axi slaves!  Exiting");
        return 1;
    }
    printf("done\n");

    printf("Exiting cleanly.\n");
    
    return 0;
}

int testReg(enum reg registers, int id, int value){
    printf("testing board %d reg %3X\n", id, registers);

    cccd(comms, Reg, WR, registers, id, value);
    cccd(comms, Reg, RD, registers, id);
    
    //TODO: compare value and ret, log file
    printb(&comms[10]);


    return 0;
}

int testHardRst(){
    return 0;
}


int mmapAxiSlaves() {
    int mem_fd = 0;
    mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd == -1) {
        fprintf(stderr, "Error! mem_fd: 0x%x\n", mem_fd);
        return 1;
    }

    comms = (volatile unsigned int *) mmap(NULL, 64000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x43C00000);
    //dips = (volatile unsigned int *) mmap(NULL, 64000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x43C20000);
    //tdc = (volatile unsigned int *) mmap(NULL, 64000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x43C10000);

    if ((comms == MAP_FAILED) || (dips == MAP_FAILED) || (tdc == MAP_FAILED)) {
        return 1;
    }
    return 0;
}
int munmapAxiSlaves() {
    signed int munmapcheck = 0;
    munmapcheck = munmap((void *) comms, 64000);
    if (munmapcheck == -1)
        return 1;
    //munmapcheck = munmap((void *) dips, 64000);
    //if (munmapcheck == -1)
    //    return 1;
    //munmapcheck = munmap((void *) tdc, 64000);
    //if (munmapcheck == -1)
    //    return 1;

    return 0;
}


