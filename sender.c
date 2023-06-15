#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "msg.h"    /* For the message struct */



/* The size of the shared memory segment */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the allocated message queue
 */
void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
    /* Generate a key */
    key_t key = ftok("keyfile.txt", 'a');
    
    /* Get the id of the shared memory segment */
    shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666 | IPC_CREAT);
    
    /* Attach to the shared memory */
    sharedMemPtr = shmat(shmid, (void*)0, 0);
    
    /* Get the id of the message queue */
    msqid = msgget(key, 0666 | IPC_CREAT);
}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
    /* Detach from shared memory */
    shmdt(sharedMemPtr);
    
    /* Destroy the shared memory segment */
    shmctl(shmid, IPC_RMID, NULL);
    
    /* Destroy the message queue */
    msgctl(msqid, IPC_RMID, NULL);
}

/**
 * The main send function
 * @param fileName - the name of the file
 * @return - the number of bytes sent
 */
unsigned long sendFile(const char* fileName)
{
    /* A buffer to store message we will send to the receiver. */
    message sndMsg; 
    
    /* A buffer to store message received from the receiver. */
    ackMessage rcvMsg;
        
    /* The number of bytes sent */
    unsigned long numBytesSent = 0;
    
    /* Open the file */
    FILE* fp =  fopen(fileName, "r");

    /* Was the file open? */
    if(!fp)
    {
        perror("fopen");
        exit(-1);
    }
    
    /* Read the whole file */
    while(!feof(fp))
    {
        /* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and
         * store them in shared memory.  fread() will return how many bytes it has
         * actually read. This is important; the last chunk read may be less than
         * SHARED_MEMORY_CHUNK_SIZE.
         */
        if((sndMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
        {
            perror("fread");
            exit(-1);
        }
        
        /* Count the number of bytes sent. */
        numBytesSent += sndMsg.size;
        
        /* Send a message to the receiver telling him that the data is ready
         * to be read (message of type SENDER_DATA_TYPE).
         */
        sndMsg.mtype = SENDER_DATA_TYPE;
        msgsnd(msqid, &sndMsg, sizeof(message) - sizeof(long), 0);
        
        /* Wait until the receiver sends us a message of type RECV_DONE_TYPE telling us 
         * that he finished saving a chunk of memory. 
         */
        msgrcv(msqid, &rcvMsg, sizeof(ackMessage) - sizeof(long), RECV_DONE_TYPE, 0);
    }
    

    /** Once we are out of the above loop, we have finished sending the file.
     * Let's tell the receiver that we have nothing more to send. We will do this by
     * sending a message of type SENDER_DATA_TYPE with size field set to 0.   
     */
    sndMsg.mtype = SENDER_DATA_TYPE;
    sndMsg.size = 0;
    msgsnd(msqid, &sndMsg, sizeof(message) - sizeof(long), 0);
    
    /* Close the file */
    fclose(fp);
    
    return numBytesSent;
}

/**
 * Used to send the name of the file to the receiver
 * @param fileName - the name of the file to send
 */
void sendFileName(const char* fileName)
{
    /* Get the length of the file name */
    int fileNameSize = strlen(fileName);

    /* Make sure the file name does not exceed the maximum buffer size in the fileNameMsg struct */
    if (fileNameSize >= MAX_FILE_NAME_SIZE) {
        fprintf(stderr, "File name exceeds the maximum buffer size\n");
        exit(-1);
    }

    /* Create an instance of the struct representing the message containing the name of the file */
    fileNameMsg msg;
    
    /* Set the message type FILE_NAME_TRANSFER_TYPE */
    msg.mtype = FILE_NAME_TRANSFER_TYPE;
    
    /* Set the file name in the message */
    strncpy(msg.fileName, fileName, MAX_FILE_NAME_SIZE);
    
    /* Send the message using msgsnd */
    msgsnd(msqid, &msg, sizeof(fileNameMsg) - sizeof(long), 0);
}


int main(int argc, char** argv)
{
    /* Check the command line arguments */
    if(argc < 2)
    {
        fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
        exit(-1);
    }
        
    /* Connect to shared memory and the message queue */
    init(shmid, msqid, sharedMemPtr);
    
    /* Send the name of the file */
    sendFileName(argv[1]);
        
    /* Send the file */
    fprintf(stderr, "The number of bytes sent is %lu\n", sendFile(argv[1]));
    
    /* Cleanup */
    cleanUp(shmid, msqid, sharedMemPtr);
        
    return 0;
}

