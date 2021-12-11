#include "headers.h"



int quantum = 0;    //quantum from RR algorithm inputted from user
pid_t schdPid;      //pid of scheduler
Algorithm algo;     //Algorithm chosen by user to run
sigset_t intmask;   //to temporarily block signal from process_generator in RR and HPF


void readInputFile(char* pFileName);    //reading processes from the provided file
void clearResources(int);               //clearing resources when exiting
void startClk();        //execv'ing the clock
void startScheduler();  //execv'ing the scheduler
void handleAlarm();     //handler for alarm signal
void getUserInput();   //get algorithm parameters from user



/*----------------------Queue----------------------*/
typedef struct Node
{
    processData *data;
    struct Node *next;
}Node;
Node *front = NULL, *rear = NULL;
int qSize = 0;  //would equal total number of processes when the program exits
void pop();     //throws front and frees memory
void enqueue(processData *val);
void _printQueue(); //internal function for testing purposes
/*-------------------------------------------------*/


int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    signal(SIGALRM, handleAlarm);

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

    //Clear clock resources
    clearResources(0);
}

void readInputFile(char* pFileName)
{
    FILE *pInputFile;

    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int id;
    processData *p;

    pInputFile = fopen(pFileName, "r"); //opening file for read
    if (pInputFile==NULL)
    {
        perror("Error reading input file");
        exit(EXIT_FAILURE);
    }
    getline(&line, &len, pInputFile); //read comment line 
    while (fscanf(pInputFile, "%ld", &id) != -1) //read line by line till end
    {
        p = (processData*)malloc(sizeof(processData));
        p->id = id;
        fscanf(pInputFile, "%d", &(p->arrivalTime));
        fscanf(pInputFile, "%d", &(p->executionTime));
        fscanf(pInputFile, "%d", &(p->priority));
        enqueue(p);
        
    }

    fclose(pInputFile); //close file upon finish
}


void clearResources(int signum)
{
    destroyClk(true);   //destroy whole group
    destroyMsgQ();      //destroy msgQ
    exit(EXIT_SUCCESS);
}

void startClk()
{
    pid_t clk_id = fork();
    if(clk_id == -1) //Error Occured
    {
        perror("Error forking for clock");
        exit(EXIT_FAILURE);
    }
    else if(clk_id == 0) //Child(clock) execv'es
    {
        char *const paramList[] = {"./clk.out", NULL};
        execv("./clk.out", paramList);
        //if it executes what is under this line, then execv has failed
        perror("Error in execv'ing to clock");
        exit(EXIT_FAILURE);
    }
    //Parent(process_generator) continues and exits the function
}

void startScheduler()
{
    schdPid = fork();
    if(schdPid == -1) //Error Occured
    {
        perror("Error forking for scheduler");
        exit(EXIT_FAILURE);
    }
    else if(schdPid == 0) //Child
    {
        char sAlgo[5]; 
        char sCnt[10];  
        char squantum[5];

        //converting ints to chars for sending as parameters
        sprintf(sAlgo, "%d", algo);
        sprintf(sCnt, "%d", qSize);
        sprintf(squantum, "%d", quantum);
        char *const paramList[] = {"./scheduler.out", sAlgo, sCnt, squantum, NULL};
        execv("./scheduler.out", paramList);

        //This will be executed in case of unsuccessful execv() only
        perror("Error in execv'ing to scheduler");
        exit(EXIT_FAILURE);
    }
    //Parent(process_generator) continues and exits the function
}

void handleAlarm()
{

}

void getUserInput()
{
    int choice;
    printf(
    "Please choose the scheduling algorithm you want:\n\
    1)Non-preemptive Highest Priority First (HPF)\n\
    2)Shortest Remaining time Next (SRTN).\n\
    3)Round Robin (RR).\n");
    scanf("%d", &choice);
    if(choice > 3 || choice < 1) //choice is out of range
    {
        printf("Error! Incorrect choice. Exiting program\n");
        exit(EXIT_FAILURE);
    }
    if(choice == 3) //quantum needed in case of RR
    {
        printf("Enter quantum for Round Robin algorithm. \n ");
        scanf("%d", &quantum);
    }
    algo =  choice - 1; //decremented since Algorithm is an enum that starts with 0
}

void pop()
{
    if(front == NULL)
        return;
    if(front == rear)
        rear = NULL;
    Node* temp = front;
    front = front->next;
    free(temp->data);   //First free data
    free(temp);
}

void enqueue(processData *val)
{
    qSize++;
    Node *newNode = malloc(sizeof(Node));
    newNode->next = NULL;
    newNode->data = val;
    
    //First node to be added
    if(front == NULL && rear == NULL)
    {
        //make both front and rear points to newNode
        front = newNode;
        rear = newNode;
    }
    else //not the first
    {
        //add newNode in rear->next
        rear->next = newNode;

        //make newNode as the rear Node
        rear = newNode;
    }
}

void _printQueue() //Internal function for testing purposes
{
    Node* index = front;
    while(index)
    {
        printf("ID: %d\n", index->data->id);
        index = index->next;
    }
}