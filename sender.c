/*

#####                                     
#     # ###### #    # #####  ###### #####  
#       #      ##   # #    # #      #    # 
 #####  #####  # #  # #    # #####  #    # 
      # #      #  # # #    # #      #####  
#     # #      #   ## #    # #      #   #  
 #####  ###### #    # #####  ###### #    # 



   This program is a sender that demonstrates inter-process communication 
   using shared memory and message queues.
   It reads the contents of a file and transfers them to a receiver program.
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

// Function to handle the SIGINT signal (Ctrl+C)
void handleSIGINT() {
    printf("Terminating sender...\n");
    // Clean up shared memory and message queue
    cleanUp();
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

// Function to send a message through the message queue
void send(struct message* msg) {
    // Send the message to the message queue
    if (msgsnd(msqid, msg, sizeof(struct message) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    // Check the command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = handleSIGINT;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    struct message msg;

    // Initialize shared memory and message queue
    init();

    // Open the file
    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("fopen");
        cleanUp();
        exit(1);
    }

    int bytesRead;
    // Read from the file and send messages until the end is reached
    while ((bytesRead = fread(msg.payload, sizeof(char), MAX_MSG_PAYLOAD, file)) > 0) {
        // Set the message type and size
        msg.mtype = 1;
        msg.size = bytesRead;
        // Send the message
        send(&msg);
        // Sleep for a short time
        usleep(1000);
    }

    // Close the file
    fclose(file);

    // Send termination message
    msg.mtype = 1;
    strcpy(msg.payload, "end");
    msg.size = strlen(msg.payload) + 1;
    send(&msg);

    // Clean up shared memory and message queue
    cleanUp();

    return 0;
}
