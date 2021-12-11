#include "headers.h"
#include "scheduler.h"


int main(int argc, char * argv[])
{
    //setting user-defined signal handlers
    signal(SIGALRM, handleAlarm);
    signal(SIGUSR1, handleUser1);
    signal(SIGUSR2, handleSigChild);

    // sigignore(SIGCHLD); //ignoring childsignal so it doesn't interrupt running

    algo = atoi(argv[1]);           //algorithm chosen by user
    processesCnt = atoi(argv[2]);   //number of total processes in system
    quantum = atoi(argv[3]);        //quantum in case of RR algorithm

    //initializing the clock
    initClk();

    //initializing Message queue with process_generator
    initMsgQ();

    //clearing file and printing comment line
    createSchedulerLog();

    //initializing PQ and deadQ
    if(algo != RR)
        prQueue = (PCB**) calloc(processesCnt, sizeof(PCB*));
    deadQ = (PCB**) calloc(processesCnt, sizeof(PCB*));

    while(true)
    {
        pause();
    }
}