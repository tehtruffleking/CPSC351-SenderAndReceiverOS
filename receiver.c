/*
   This program is a receiver that demonstrates inter-process communication 
   using shared memory and message queues.
   It receives messages from a sender program and writes them to a file.
*/

#include "msg2.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>

#define SHARED_MEMORY_CHUNK_SIZE 1000
#define MESSAGE_QUEUE_SIZE 10

int shmid, msqid;
void* sharedMemPtr;

// Function to handle the SIGINT signal (Ctrl+C)
void handleSIGINT(int sig) {
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
void cleanUp() {
    // Remove shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    // Remove message queue
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }

    // Detach shared memory segment from process address space
    if (shmdt(sharedMemPtr) == -1) {
        perror("shmdt");
        exit(1);
    }
}

// Function to receive the file name through the message queue
void recvFileName(char* fileName) {
    struct fileNameMsg msg;
    msgrcv(msqid, &msg, sizeof(struct fileNameMsg) - sizeof(long), FILE_NAME_TRANSFER_TYPE, 0);
    strcpy(fileName, msg.fileName);
}

// Main loop to receive and write messages to a file
void mainLoop(const char* fileName) {
    FILE* file = fopen(fileName, "w");
    if (file == NULL) {
        perror("fopen");
        exit(1);
    }

    struct message msg;
    int msgSize;
    while (1) {
        // Receive a message from the message queue
        msgrcv(msqid, &msg, sizeof(struct message) - sizeof(long), SENDER_DATA_TYPE, 0);
        msgSize = msg.size;

        // Check if the message size is 0, indicating the end of the file
        if (msgSize == 0) {
            break;
        }

        // Write the message payload to the file
        fwrite(msg.payload, sizeof(char), msgSize, file);

        // Send an acknowledgement message to the sender
        struct ackMessage ack;
        ack.mtype = RECV_DONE_TYPE;
        msgsnd(msqid, &ack, sizeof(struct ackMessage) - sizeof(long), 0);
    }

    fclose(file);
}

int main(int argc, char* argv[]) {
    // Check the command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    void (*prevHandler)(int);

    // Set up signal handler for SIGINT (Ctrl+C)
    prevHandler = signal(SIGINT, handleSIGINT);
    if (prevHandler == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    // Initialize shared memory and message queue
    init();

    char fileName[MAX_FILE_NAME_SIZE];
    // Receive the file name through the message queue
    recvFileName(fileName);
    printf("Received file name: %s\n", fileName);

    // Enter the main loop to receive and write messages to the file
    mainLoop(argv[1]);

    // Clean up shared memory and message queue
    cleanUp();

    // Restore the previous signal handler for SIGINT
    if (signal(SIGINT, prevHandler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    return 0;
}
