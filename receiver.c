/*

 ######                                              
#     # ######  ####  ###### # #    # ###### #####  
#     # #      #    # #      # #    # #      #    # 
######  #####  #      #####  # #    # #####  #    # 
#   #   #      #      #      # #    # #      #####  
#    #  #      #    # #      #  #  #  #      #   #  
#     # ######  ####  ###### #   ##   ###### #    # 


   This program is a receiver that demonstrates inter-process communication 
   using shared memory and message queues.
   It receives messages from a sender program and writes them to a file.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include "msg2.h"

#define SHARED_MEMORY_CHUNK_SIZE 1000
#define MESSAGE_QUEUE_SIZE 10

int shmid, msqid;
void* sharedMemPtr;

void cleanUp(int* shmid, int* msqid, void* sharedMemPtr);

void handleSIGINT() {
    printf("Terminating receiver...\n");
    cleanUp(&shmid, &msqid, sharedMemPtr);
    exit(0);
}

void init() {
    key_t key = ftok("keyfile.txt", 'a');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    sharedMemPtr = shmat(shmid, (void*)0, 0);
    if (sharedMemPtr == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid == -1) {
        perror("msgget");
        exit(1);
    }
}

void cleanUp(int* shmid, int* msqid, void* sharedMemPtr) {
    if (shmctl(*shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    if (msgctl(*msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }

    if (shmdt(sharedMemPtr) == -1) {
        perror("shmdt");
        exit(1);
    }
}

void receive(struct message* msg) {
    if (msgrcv(msqid, msg, sizeof(struct message) - sizeof(long), 1, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handleSIGINT;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    struct message msg;

    init();

    while (1) {
        receive(&msg);
        if (strcmp(msg.payload, "end") == 0) {
            printf("Termination message received: %s\n", msg.payload);
            break;
        } else {
            printf("Received message: %s\n", msg.payload);
        }
    }

    cleanUp(&shmid, &msqid, sharedMemPtr);

    return 0;
}
