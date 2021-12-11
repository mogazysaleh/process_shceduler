#include "headers.h"
#include "process_generator.h"





int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    signal(SIGALRM, handleAlarm);
    signal(SIGCHLD, handleAlarm);

    //Read the input file.
    readInputFile("processes.txt");

    //Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    getUserInput();

    //Initiate and create the scheduler and clock processes.
    initMsgQ();
    startScheduler();
    startClk();
    

    //Use this function after creating the clock process to initialize clock
    initClk();

    while(front)
    {
        //Send the information to the scheduler at the appropriate time.
        if(front->data->arrivalTime == getClk())
        {
            sendPrcs(front->data);
            kill(schdPid, SIGUSR1);
            pop();
        }
        else
        {
            alarm(1);
            pause();
        }
    }
    pause(); //sleep till scheduler finishes

    clearResources(0);  //clear resources
}