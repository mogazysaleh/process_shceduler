#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <math.h>

#define true 1
#define false 0
#define SHKEY 300
#define MSGQKEY 200

typedef short bool;
typedef enum ProcessState{READY, RUNNING, FINISHED} ProcessState; //may be removed
typedef enum Algorithm{HPF, SRTN, RR} Algorithm;
typedef struct processData
{
    long id;
    int arrivalTime;
    int priority;
    int executionTime;
}processData;
typedef struct PCB
{
    int id; //id from file
    pid_t processId; //getpid()
    ProcessState state;
    int priority;//from process_generator
    int arrivalTime;//from process_generator
    int executionTime;//from process_generator
    int waitingTime;//from handler(finish time - running time)
    int remainingTime;//from process_generator (update in algorithm)
    int startTime;//in algorithm
    int finishTime;//handler
    int TA; //turnaround time
    int recentStart;
    double WTA; //weighted turnaround
} PCB;

int msgQId; //id of the msgQ to be used to share processes between scheduler and process_generator

///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================



int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}

void initMsgQ()
{
    msgQId = msgget(MSGQKEY, IPC_CREAT | 0666);
    if(msgQId == -1)
    {
        perror("Error initializing MSGQ");
        exit(EXIT_FAILURE);
    }
}

void sendPrcs(processData* prcs)
{
    printf("I will send ysta at %d\n", getClk()); //test
    if( msgsnd(msgQId, prcs, sizeof(processData) - sizeof(long), !IPC_NOWAIT) == -1)
    {
        perror("Error sending a msg");
        exit(EXIT_FAILURE);
    }
}

ssize_t rcvPrcs(processData* prcs)
{
    return msgrcv(msgQId, prcs, sizeof(processData) - sizeof(long), 0, IPC_NOWAIT);
}


void destroyMsgQ()
{
    if(msgctl(msgQId, IPC_RMID, (void *)0) == -1)
    {
        perror("Error destroying MsgQ\n");
        exit(EXIT_FAILURE);
    }
}