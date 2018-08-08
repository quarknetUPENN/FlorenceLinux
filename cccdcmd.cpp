//
// Created by hep on 8/7/18.
//

#include <cstdio>
#include "cccdcmd.h"
#include "cccdlib.h"
#include "l1arecv.h"
#include "map"
#include <sys/mman.h>
#include <fcntl.h>

volatile unsigned int * comms;

int mmapAxiSlaves();

int munmapAxiSlaves();


int main(int argc, char *argv[]){
    printf("Memory mapping the axi slaves...");
    if (mmapAxiSlaves() != 0) {
        printf("Failed!  Exiting");
        return 1;
    }
    printf("done\n");

    std::map<std::string,registers> regmap = {{"Config", Config},
                                              {"Status", Status},
                                              {"Thresh1", Thresh1},
                                              {"Thresh2", Thresh2}};
    std::map<std::string,command> cmdmap = {{"L1A", L1A},
                                            {"Reg", Reg},
                                            {"SoftRst", SoftRst},
                                            {"BxRst", BxRst}};
    std::map<std::string,bool> rwmap = {{"RD", RD},
                                        {"WR", WR}};
    switch (argc){
        case 2:
            cccd(comms, cmdmap[argv[1]]);
            break;
        case 5:
            cccd(comms, cmdmap[argv[1]], rwmap[argv[2]], regmap[argv[3]], atoi(argv[4]));
            break;
        case 6:
            cccd(comms, cmdmap[argv[1]], rwmap[argv[2]], regmap[argv[3]], atoi(argv[4]), (unsigned) atoi(argv[5]));
            break;

        default:
            printf("Incorrect number of arguments supplied\n");
    }

    if (argc >= 3){
        if(rwmap[argv[2]] == RD){
            printb(&comms[10]);
        }
    }

    printf("Unmapping the axi slaves from memory...");
    if (munmapAxiSlaves() != 0) {
        printf("Failed to unmap memory for axi slaves!  Exiting");
        return 1;
    }
    printf("done\n");

    printf("Exiting cleanly.\n");
}

int mmapAxiSlaves() {
    int mem_fd = 0;                 // /dev/mem memory file descriptor
    mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd == -1) {
        printf("Error! mem_fd: 0x%x\n", mem_fd);
        return 1;
    }

    comms = (volatile unsigned int *) mmap(NULL, 64000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x43C00000);


    if (comms == MAP_FAILED) {
        return 1;
    }
    return 0;
}
int munmapAxiSlaves() {
    signed int munmapcheck = 0;
    munmapcheck = munmap((void *) comms, 64000);
    if (munmapcheck == -1)
        return 1;

    return 0;
}
