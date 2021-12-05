#include "headers.h"

typedef struct PCB
{
    int id; //id from file
    pid_t processId; //getpid()
    enum ProcessState state;
    int priority;
    int arrivalTime;
    int executionTime;
    int runningTime;
    int waitingTime;
    int remainingTime;
} PCB;

processData rcvd;
PCB **readyQ;
int qSize = 0;


void cpyPrcs(PCB* prcs, processData* rcvd)
{
    prcs->id = rcvd->id;
    prcs->priority = rcvd->priority;
    prcs->runningTime = rcvd->runningtime;
    prcs->arrivalTime = rcvd->arrivaltime;
}

void _printQ()
{
    for(int i = 0; i < qSize; i++)
        printf("%d  %d  %d  %d\n", readyQ[i]->id, readyQ[i]->priority, readyQ[i]->runningTime, readyQ[i]->arrivalTime);
}

int main(int argc, char * argv[])
{
    initClk();
    initMsgQ();
    Algorithm algo = atoi(argv[1]);
    int processesCnt = atoi(argv[2]);
    readyQ = (PCB**) malloc(sizeof(PCB*) * processesCnt);
    //TODO implement the scheduler :)
    //upon termination release the clock resources
    while(true)
    {
        if(rcvPrcs(&rcvd) != -1) //process arrived
        {
            printf("rcvd ysta\n");
            PCB* prcs = (PCB*) malloc(sizeof(PCB));
            cpyPrcs(prcs, &rcvd);
            readyQ[qSize++] = prcs;
        }
        if(getClk() == 10)
            _printQ();
    }
    destroyClk(true);
}

