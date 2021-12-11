#include "headers.h"
void handleAlarm(int signum);

int main(int agrc, char * argv[])
{
    initClk();
    sigset(SIGALRM,  SIG_DFL);
    signal(SIGALRM, handleAlarm);

    //initializing remianing time of a process to its running time passed by scheduler
    int remainingTime = atoi(argv[1]);
    int schdPid = atoi(argv[2]);
    int schdClk = atoi(argv[3]);

    if(remainingTime > 0)   //atoi will return - if remTime is passed to it as 0
    {
        if(schdClk != getClk()) //solution to randomness
            remainingTime--;
        
        //run and decrement remainingTime each second
        while (remainingTime--)
        {
            alarm(1);
            pause();
        }
    }


    kill(schdPid, SIGUSR2);

    destroyClk(false);
    return 0;
}

void handleAlarm(int signum)
{

}

