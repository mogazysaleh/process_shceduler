#include "headers.h"

enum ProcessState{BLOCKED, READY, RUNNING};
struct PCB;
void* getPGconnection();

int main(int argc, char * argv[])
{
    initClk();
    //TODO implement the scheduler :)
    //upon termination release the clock resources
    
    destroyClk(true);
}

struct PCB
{
    pid_t processId;
    enum ProcessState state;
    int priority;
    int arrivalTime;
    int executionTime;
    int runningTime;
    int waitingTime;
    int remainingTime;
};

void* getPGconnection()
{
    int key = shmget(1, 1024, IPC_CREAT | 0644);
    void* shmaddr = shmat(key, (void *) 0, 0);
    return shmaddr;
}