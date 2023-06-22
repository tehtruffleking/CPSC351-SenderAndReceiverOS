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

// Function to clean up shared memory and message queue
void cleanUp(int* shmid, int* msqid, void* sharedMemPtr);

// Function to handle the SIGINT signal (Ctrl+C)
void handleSIGINT() {
    printf("Terminating receiver...\n");
    // Clean up shared memory and message queue
    cleanUp(&shmid, &msqid, sharedMemPtr);
    exit(0);
}

// Function to initialize shared memory and message queue
void init() {
    // Generate a key using ftok() based on a file and a character
    key_t key = ftok("keyfile.txt", 'a');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Create or access shared memory segment
    shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach shared memory segment to process address space
    sharedMemPtr = shmat(shmid, (void*)0, 0);
    if (sharedMemPtr == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    // Create or access message queue
    msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid == -1) {
        perror("msgget");
        exit(1);
    }
}

// Function to clean up shared memory and message queue
void cleanUp(int* shmid, int* msqid, void* sharedMemPtr) {
    // Remove shared memory segment
    if (shmctl(*shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    // Remove message queue
    if (msgctl(*msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }

    // Detach shared memory segment from process address space
    if (shmdt(sharedMemPtr) == -1) {
        perror("shmdt");
        exit(1);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handleSIGINT;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // Initialize shared memory and message queue
    init();

    struct message msg;

    // Receive and process messages until "end" message is received
    while (1) {
        // Receive a message from the message queue
        if (msgrcv(msqid, &msg, sizeof(struct message) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv");
            cleanUp(&shmid, &msqid, sharedMemPtr);
            exit(1);
        }

        // Check if it is the termination message
        if (strcmp(msg.payload, "end") == 0)
            break;

        // Process the received message
        printf("Received message: %s\n", msg.payload);
    }

    // Clean up shared memory and message queue
    cleanUp(&shmid, &msqid, sharedMemPtr);

    return 0;
}
