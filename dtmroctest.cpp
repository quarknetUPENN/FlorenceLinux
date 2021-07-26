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

void testChip(int boardnum, int id, FILE *results);

int testReg(enum reg registers, int id, int value);

int compareOutput(volatile void *ptr, int regLength, unsigned int inputVal);

int testHardRst(int id);

int main(int argc, char *argv[]) {
    int boardnum, id;
    char changeId;

    FILE *results;
    
    results = fopen("testResultsDTMROC.csv", "a");
    if(results == NULL){
        printf("Error opening file! Exiting");
        return 2;
    }
    //fprintf(results, "Board #, chip ID, Config errors, Thresh1 errors, Thresh2 errors, Status, BIST result\n");
    
    printf("Memory mapping the axi slaves...");
    if (mmapAxiSlaves() != 0) {
        fprintf(stderr, "Failed!  Exiting");
            //TODO: if mmap failed, print error and do not log data
        return 1;
    }
    printf("done\n");


    // get board number input
    printf("Enter board number: ");
    scanf("%d", &boardnum);
    
    for(id = 62; id <= 63; id++){
        testChip(boardnum, id, results);
    }
    
    
    // test id switch
    printf("\n\nchange board id to 00000? (y/n)");
    //changeId = fgetc(stdin);
    scanf(" %c", &changeId);
    if(changeId == 'n'){
        return 1;
    }else {
        for(id = 0; id <= 1; id++){
            testChip(boardnum, id, results);
        }
    }


    // close file & exit
    printf("Unmapping the axi slaves from memory...");
    if (munmapAxiSlaves() != 0) {
        fprintf(stderr, "Failed to unmap memory for axi slaves!  Exiting");
        return 1;
    }
    printf("done\n");
    printf("Exiting cleanly.\n");
    
    fclose(results);
    return 0;
}

void testChip(int boardnum, int id, FILE *results){
        int configErr, thr1Err, thr2Err, rstErr, statusReg;
        bool isBistFinished, bistResult;
        
        printf("\nboard number: %0d    chip id: %X\n", boardnum, id);

        // init error indicators
        configErr = thr1Err = thr2Err = rstErr = 0;
        
        // w/r config reg
        configErr += testReg(Config, id, 0xAAAAAA);
        //testReg(Config, 63, 0x555515); //bit 6 turns on BIST, so it cannot be set to 1 to ensure normal operation
        configErr += testReg(Config, id, 0x000001);
       
        // w/r thresh 1
        thr1Err += testReg(Thresh1, id, 0xAAAA);
        thr1Err += testReg(Thresh1, id, 0x5555);
        
        // w/r thresh 2
        thr2Err += testReg(Thresh2, id, 0xAAAA);
        thr2Err += testReg(Thresh2, id, 0x5555);
        
        // read status reg (BIST)
        cccd(comms, Reg, RD, Status, id);
        printf("status before BIST:");
        printb(&comms[10]);
        cccd(comms, Reg, WR, Config, id, 65);     //start bist 

        do {
            cccd(comms, Reg, RD, Status, id);   
            //bit 8 and 10 are progress flags, is 1 when finished 
            isBistFinished = (comms[10] >> 8 & 1 ) & (comms[10] >> 10 & 1); 
            printf("    finished: %X\n", isBistFinished);
        }while(isBistFinished == false);
        bistResult = (comms[10] >> 9 & 1 ) & (comms[10] >> 11 & 1); 
        statusReg = comms[10];
        printf("BIST result: %X\n", bistResult);
        printf("status after BIST: ");
        printb(&comms[10]);

        //test rst line
        rstErr = testHardRst(id);

	
	    //write to csv
           //"Board ID, 2V5, 3V3, Config errors, Thresh1 errors, Thresh2 errors, Status, BIST result
        fprintf(results, "%02d, %02d, %03X, %02X, %02X, %03X, %08X, %X\n", 
                boardnum, id, configErr, thr1Err, thr2Err, rstErr, statusReg, bistResult);

}

int testReg(enum reg registers, int id, int value){
    int regLen, result;
    printf("testing chipid %02d reg %X\n", id, registers);

    cccd(comms, Reg, WR, registers, id, value);
    cccd(comms, Reg, RD, registers, id);
    
    //printf("value read from reg: ");
    //printb(&comms[10]);
    
    switch (registers){
        case Config: regLen = 24; break;
        case Status: regLen = 32; break;
        case Thresh1: regLen = 16; break;
        case Thresh2: regLen = 16; break;
        default: printf("shouldn't be in default :(");
    }
    
    result = compareOutput(&comms[10], regLen, value);
    printf("    compare output error: %X\n", result);
    
    return result;
}



// return the number of bytes that are different between input and read value
// should return 0 for functional registers
int compareOutput(volatile void *ptr, int regLength, unsigned int inputVal){
    unsigned int size = regLength / 8;    
    unsigned int regVal = 0; 
    auto b = (unsigned char *) ptr;
    unsigned int result = 0;
    unsigned char currByte, inputByte;
    int i;

    printf("    input:%X ", inputVal);
    regVal = comms[10];
    
    regVal = regVal >> (32 - regLength);
    printf("    read: %X\n", regVal);
    
    result = regVal ^ inputVal;     //result[i] is 1 if the two bits are different

    return result;
}

//sends a hard reset signal and then reads from registers to see
//if they are at the default value, returns the number of error bytes
int testHardRst(int id){
    int delay = 0;
    unsigned int errorCount = 0;
    
    // req_hard_rst = slv_reg0[0]    reset is comms[0] bit 0
    // reset is active high (not low as documented)
    comms[0] = 0;
    comms[0] = 1;
    comms[0] = 0;
    
    while(delay < 10000){
        delay ++;
    }
    
    //config reg should be 0001 after rst
    cccd(comms, Reg, RD, Config, id);
    //printf("\n    config reg after reset: ");
    //printb(&comms[10]);
    errorCount += compareOutput(&comms[10], 24, 0x00001);
    
    // thresh regs should be FFFF after rst
    cccd(comms, Reg, RD, Thresh1, id);      
    //printf("\nafter reset: ");
    //printb(&comms[10]);
    errorCount += compareOutput(&comms[10], 16, 0xFFFF);
    
    cccd(comms, Reg, RD, Thresh2, id);
    errorCount += compareOutput(&comms[10], 16, 0xFFFF);
    
    cccd(comms, Reg, RD, Status, id);
    printf("    status reg after reset: ");
    printb(&comms[10]);
    //errorCount += compareOutput(&comms[10], 16, 0xFFFF);
    
    printf("    rst error: %X\n", errorCount);
    return errorCount;
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


