#include "headers.h"


int main(int agrc, char * argv[])
{
    initClk();
    int currentClk;

    //initializing remianing time of a process to its running time passed by scheduler
    int remainingTime = atoi(argv[1]);
    int schdPid = atoi(argv[2]);

    //run and decrement remainingTime each second
    while (remainingTime--)
    {
        currentClk = getClk();
        while(currentClk == getClk());
    }
    printf("PID: %d\n", schdPid);
    kill(schdPid, SIGUSR1);
    destroyClk(false);
    return 0;
}
