#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include "msg.h"    /* For the message struct */


using namespace std;

/* The size of the shared memory segment */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void *sharedMemPtr = NULL;

/**
 * The function for receiving the name of the file
 * @return - the name of the file received from the sender
 */
string recvFileName()
{
    /* The file name received from the sender */
    string fileName;
    
    /* Declare an instance of the fileNameMsg struct to be
     * used for holding the message received from the sender.
     */
    fileNameMsg msg;
    
    /* Receive the file name using msgrcv() */
    msgrcv(msqid, &msg, sizeof(fileNameMsg) - sizeof(long), FILE_NAME_TRANSFER_TYPE, 0);
    
    /* Return the received file name */
    return msg.fileName;
}

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */
void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
    /* Create a file called keyfile.txt containing string "Hello world" */
    FILE* keyFile = fopen("keyfile.txt", "w");
    fputs("Hello world", keyFile);
    fclose(keyFile);
    
    /* Generate a key */
    key_t key = ftok("keyfile.txt", 'a');
    
    /* Allocate a shared memory segment */
    shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666 | IPC_CREAT);
    
    /* Attach to the shared memory */
    sharedMemPtr = shmat(shmid, (void*)0, 0);
    
    /* Create a message queue */
    msqid = msgget(key, 0666 | IPC_CREAT);
}

/**
 * The main loop
 * @param fileName - the name of the file received from the sender.
 * @return - the number of bytes received
 */
unsigned long mainLoop(const char* fileName)
{
    /* The size of the message received from the sender */
    int msgSize = -1;
    
    /* The number of bytes received */
    int numBytesRecv = 0;
    
    /* The string representing the file name received from the sender */
    string recvFileNameStr = fileName;
    
    /* Append __recv to the end of file name */
    recvFileNameStr += "__recv";
    
    /* Open the file for writing */
    FILE* fp = fopen(recvFileNameStr.c_str(), "w");
    
    /* Error checks */
    if (!fp)
    {
        perror("fopen");
        exit(-1);
    }
    
    /* Keep receiving until the sender sets the size to 0, indicating that
     * there is no more data to send.
     */
    while (msgSize != 0)
    {
        /* Receive the message and get the value of the size field */
        message msg;
        msgrcv(msqid, &msg, sizeof(message) - sizeof(long), SENDER_DATA_TYPE, 0);
        msgSize = msg.size;
        
        /* If the sender is not telling us that we are done, then get to work */
        if (msgSize != 0)
        {
            /* Count the number of bytes received */
            numBytesRecv += msgSize;
            
            /* Save the shared memory to file */
            if (fwrite(sharedMemPtr, sizeof(char), msgSize, fp) < 0)
            {
                perror("fwrite");
            }
            
            /* Tell the sender that we are ready for the next set of bytes */
            ackMessage ack;
            ack.mtype = RECV_DONE_TYPE;
            msgsnd(msqid, &ack, sizeof(ackMessage) - sizeof(long), 0);
        }
        /* We are done */
        else
        {
            /* Close the file */
            fclose(fp);
        }
    }
    
    return numBytesRecv;
}

/**
 * Performs cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
    /* Detach from shared memory */
    shmdt(sharedMemPtr);
    
    /* Deallocate the shared memory segment */
    shmctl(shmid, IPC_RMID, NULL);
    
    /* Deallocate the message queue */
    msgctl(msqid, IPC_RMID, NULL);
}

/**
 * Handles the exit signal
 * @param signal - the signal type
 */
void ctrlCSignal(int signal)
{
    /* Free System V resources */
    cleanUp(shmid, msqid, sharedMemPtr);
}

int main(int argc, char** argv)
{
    /* Install a signal handler to delete resources on Ctrl+C */
    signal(SIGINT, ctrlCSignal);
    
    /* Initialize */
    init(shmid, msqid, sharedMemPtr);
    
    /* Receive the file name from the sender */
    string fileName = recvFileName();
    
    /* Go to the main loop */
    fprintf(stderr, "The number of bytes received is: %lu\n", mainLoop(fileName.c_str()));
    
    /* Detach from shared memory segment, and deallocate shared memory and message queue */
    cleanUp(shmid, msqid, sharedMemPtr);
    
    return 0;
}
