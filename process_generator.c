#include "headers.h"

void clearResources(int);
void startClk();
enum Algorithm getAlgorithm();


int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    getAlgorithm();
    // 3. Initiate and create the scheduler and clock processes.
    startClk(); 

    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // To get time use this
    printf("current time is %d\n", getClk());
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    exit(EXIT_SUCCESS);
    //TODO Clears all resources in case of interruption
}

void startClk()
{
    pid_t clk_id = fork();
    if(clk_id == -1)
    {
        perror("Error forking for clock");
        exit(EXIT_FAILURE);
    }
    else if(clk_id == 0)
    {
        char *const paramList[] = {"./clk", NULL};
        execv("./clk", paramList);
        //if it executes what is under this line, then execv has failed
        perror("Error in execv'ing to clock");
        exit(EXIT_FAILURE);
    }
}

enum Algorithm getAlgorithm()
{
    int choice;
    printf(
    "Please choose the scheduling algorithm you want:\n\
    1)Non-preemptive Highest Priority First (HPF)\n\
    2)Shortest Remaining time Next (SRTN).\n\
    3)Round Robin (RR).\n");
    scanf("%d", &choice);
    if(choice > 3 || choice < 1)
    {
        printf("Error! Incorrect choice. Exiting program\n");
        exit(EXIT_FAILURE);
    }
    return choice - 1;
}
