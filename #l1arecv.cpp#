#include <iostream>
#include <fstream>
#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <string>

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include "cccdlib.h"
#include "l1arecv.h"

// the IP address and port for us to connect to on the Ubuntu computer
#define SOCKSERV "169.254.27.143"
#define PORT 8080

volatile unsigned int * comms;
volatile unsigned int * tdc;
volatile unsigned int * dips;


int mmapAxiSlaves();

int munmapAxiSlaves();

int establishSocket();

void sockSend(std::string msg);

void getL1aSet(int l1as_to_send);

volatile int sock = 0;

int main(int argc, char *argv[]) {
    printf("Memory mapping the axi slaves...");
    if (mmapAxiSlaves() != 0) {
        fprintf(stderr, "Failed!  Exiting");
        return 1;
    }
    printf("done\n");

    printf("Establishing socket connection to %s on port %u...", SOCKSERV, PORT);
    if (establishSocket() != 0){
        fprintf(stderr, "Exiting");
        return 1;
    }
    printf("done\n");


    // make sure our run is stopped
    tdc[0] = 0;

    int l1as_to_send = 11;

    // if there are no arguments, just send 11 l1as by default
    // if there is 1 arg, interpret it as the number of l1as to send
    // if there are 2 args, interpret the first as the number of l1as to send
    // and the second as the name of a file containing numbers seperated by \n chars
    // For each number in the file, set the thresholds to that value and then send l1as_to_send number of l1as
    switch(argc){
        case 1: {
            getL1aSet(l1as_to_send);
            break;
        }
        case 2: {
            l1as_to_send = atoi(argv[1]);
            getL1aSet(l1as_to_send);
            break;
        }
        case 3: {
            l1as_to_send = atoi(argv[1]);

            std::ifstream threshfile(argv[2]);
            std::string line;
            while (std::getline(threshfile, line)) {
                cccd(comms, Reg, WR, Thresh1, CHIPID_ALL, (unsigned int) stoi(line));
                cccd(comms, Reg, WR, Thresh2, CHIPID_ALL, (unsigned int) stoi(line));

                getL1aSet(l1as_to_send);
            }
            break;
        }
        default:
            getL1aSet(l1as_to_send);
            break;
    }

    sockSend("End\n");
    tdc[0] = 0;

    close(sock);
    printf("Done closing socket\n");

    printf("Unmapping the axi slaves from memory...");
    if (munmapAxiSlaves() != 0) {
        fprintf(stderr, "Failed to unmap memory for axi slaves!  Exiting");
        return 1;
    }
    printf("done\n");

    return 0;
}

void getL1aSet(int l1as_to_send) {
    // start the run by entering data taking mode, with all 0s for the trig setup indicating continuous trigger
    tdc[0] = 0b10000000000000000000000000000000;

    int l1as_sent = 0;
    bool firstdipserror = true;
    while (true) {
        if (l1as_to_send != -1) {
            if (l1as_sent >= l1as_to_send) {
                break;
            }
        }

        // make sure we aren't empty or overflowed.  If we aren't, then send all of the information in dips over
        // the socket, using "Divider" to signify the start of a new L1A
        if (dips[1] >= 660) { // this should be 662 but the read count is 2 short, possibly due to FPGA sync stages across the FIFO clock domains
            if (dips[2] == 0) {
                firstdipserror = true;
                sockSend("Divider\n");
                for (int j = 0; j < 662; j++) {
                    sockSend("[" + std::__cxx11::to_string(dips[0]) + "," + std::__cxx11::to_string(dips[1]) + "," +
                             std::__cxx11::to_string(dips[2]) + "]\n");
                }
                l1as_sent++;
            } else {
                if (!firstdipserror)
                    sockSend("dips error " + std::to_string(dips[1]) + " " + std::to_string(dips[2]) + "\n");
                firstdipserror = false;
            }
        }
    }

    // end the run
    tdc[0] = 0;
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

int establishSocket() {
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Socket creation error! ");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SOCKSERV, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address! ");
        return 2;
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Connection failed! ");
        return 3;
    }

    return 0;
}

// send a string over the socket
void sockSend(const std::string msg){
    const char * msgpt = msg.c_str();
    send(sock, msgpt, strlen(msgpt), 0);
}


