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

//copied from l1arecv 07/14
//testing program for DTMROC boards
volatile unsigned int * comms;
volatile unsigned int * tdc;
volatile unsigned int * dips;


int mmapAxiSlaves();

int munmapAxiSlaves();

int main(int argc, char *argv[]) {
    //open log file

    //get board number input
    
    
    //get voltage input
    
    //test lines: read/write, l1a data, reset line
    
    //write to log file + print to terminal
    
    //close file
    
    
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
    dips = (volatile unsigned int *) mmap(NULL, 64000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x43C20000);
    tdc = (volatile unsigned int *) mmap(NULL, 64000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x43C10000);

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
    munmapcheck = munmap((void *) dips, 64000);
    if (munmapcheck == -1)
        return 1;
    munmapcheck = munmap((void *) tdc, 64000);
    if (munmapcheck == -1)
        return 1;

    return 0;
}


