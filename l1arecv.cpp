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
        printf("Failed!  Exiting");
        return 1;
    }
    printf("done\n");

    printf("Establishing socket connection to %s on port %u...", SOCKSERV, PORT);
    if (establishSocket() != 0){
        printf("Exiting");
        return 1;
    }
    printf("done\n");



    tdc[0] = 0;

    int l1as_to_send = 11;
    if (argc >= 2){
        l1as_to_send = atoi(argv[1]);
    }

    if (argc == 2)
        getL1aSet(l1as_to_send);
    else {
        for (int n = 2; n < argc; n++) {
            cccd(comms, Reg, WR, Thresh1, CHIPID_ALL, (unsigned int) atoi(argv[n]));
            cccd(comms, Reg, WR, Thresh2, CHIPID_ALL, (unsigned int) atoi(argv[n]));

            getL1aSet(l1as_to_send);
        }
    }

    sockSend("End\n");
    tdc[0] = 0;

    close(sock);
    printf("Done closing socket\n");

    printf("Unmapping the axi slaves from memory...");
    if (munmapAxiSlaves() != 0) {
        printf("Failed to unmap memory for axi slaves!  Exiting");
        return 1;
    }
    printf("done\n");

    return 0;
}

void getL1aSet(int l1as_to_send) {

    tdc[0] = 0b10000000000000000000000000000000;

    int l1as_sent = 0;
    while (true) {
            if (l1as_to_send != -1) {
                if (l1as_sent >= l1as_to_send) {
                    break;
                }
            }

            if (dips[2] == 0) {
                sockSend("Divider\n");
                for (int j = 0; j < 662; j++) {
                    sockSend("[" + std::__cxx11::to_string(dips[0]) + "," + std::__cxx11::to_string(dips[1]) + "," +
                             std::__cxx11::to_string(dips[2]) + "]\n");
                }
                l1as_sent++;
            }
        }
    tdc[0] = 0;
}

int mmapAxiSlaves() {
    int mem_fd = 0;                 // /dev/mem memory file descriptor
    mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd == -1) {
        printf("Error! mem_fd: 0x%x\n", mem_fd);
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
        printf("Socket creation error! ");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SOCKSERV, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address! ");
        return 2;
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed! ");
        return 3;
    }

    return 0;
}

void sockSend(const std::string msg){
    const char * msgpt = msg.c_str();
    send(sock, msgpt, strlen(msgpt), 0);
}


