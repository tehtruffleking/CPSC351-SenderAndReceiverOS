/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "msg2.h"

#define SHARED_MEMORY_CHUNK_SIZE 1000
#define MESSAGE_QUEUE_SIZE 10

struct message {
    long mtype;
    char mtext[SHARED_MEMORY_CHUNK_SIZE];
};

void init(int* shmid, int* msqid, void** sharedMemPtr) {
    key_t key = ftok("keyfile.txt", 'a');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    *shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666 | IPC_CREAT);
    if (*shmid == -1) {
        perror("shmget");
        exit(1);
    }

    *sharedMemPtr = shmat(*shmid, (void*)0, 0);
    if (*sharedMemPtr == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    *msqid = msgget(key, 0666 | IPC_CREAT);
    if (*msqid == -1) {
        perror("msgget");
        exit(1);
    }
}

void cleanUp(const int* shmid, const int* msqid, void* sharedMemPtr) {
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

void send(const int msqid, struct message* msg) {
    if (msgsnd(msqid, msg, sizeof(struct message) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    int shmid, msqid;
    void* sharedMemPtr;
    struct message msg;

    init(&shmid, &msqid, &sharedMemPtr);

    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("fopen");
        cleanUp(&shmid, &msqid, sharedMemPtr);
        exit(1);
    }

    int bytesRead;
    while ((bytesRead = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, file)) > 0) {
        msg.mtype = 1;
        memcpy(msg.mtext, sharedMemPtr, bytesRead);
        send(msqid, &msg);
        usleep(1000);
    }

    fclose(file);

    // Send termination message
    msg.mtype = 1;
    strcpy(msg.mtext, "end");
    send(msqid, &msg);

    cleanUp(&shmid, &msqid, sharedMemPtr);

    return 0;
}
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "msg2.h"

#define SHARED_MEMORY_CHUNK_SIZE 1000
#define MESSAGE_QUEUE_SIZE 10

void init(int* shmid, int* msqid, void** sharedMemPtr) {
    key_t key = ftok("keyfile.txt", 'a');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    *shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666 | IPC_CREAT);
    if (*shmid == -1) {
        perror("shmget");
        exit(1);
    }

    *sharedMemPtr = shmat(*shmid, (void*)0, 0);
    if (*sharedMemPtr == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    *msqid = msgget(key, 0666 | IPC_CREAT);
    if (*msqid == -1) {
        perror("msgget");
        exit(1);
    }
}

void cleanUp(const int* shmid, const int* msqid, void* sharedMemPtr) {
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

void send(const int msqid, struct message* msg) {
    if (msgsnd(msqid, msg, sizeof(struct message) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    int shmid, msqid;
    void* sharedMemPtr;
    struct message msg;

    init(&shmid, &msqid, &sharedMemPtr);

    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("fopen");
        cleanUp(&shmid, &msqid, sharedMemPtr);
        exit(1);
    }

    int bytesRead;
    while ((bytesRead = fread(msg.payload, sizeof(char), MAX_MSG_PAYLOAD, file)) > 0) {
        msg.mtype = 1;
        msg.size = bytesRead;
        send(msqid, &msg);
        usleep(1000);
    }

    fclose(file);

    // Send termination message
    msg.mtype = 1;
    strcpy(msg.payload, "end");
    msg.size = strlen(msg.payload) + 1;
    send(msqid, &msg);

    cleanUp(&shmid, &msqid, sharedMemPtr);

    return 0;
}

