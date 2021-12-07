#include "headers.h"

typedef struct Node
{
    processData data;
    struct Node *next;
}Node;


Node *front = NULL, *rear = NULL;
int processesCnt = 0;


void initMsgQ()
{
    qid = msgget(QKEY, IPC_CREAT | 0666);
    if(qid == -1)
    {
        perror("Error initializing MSGQ");
        exit(EXIT_FAILURE);
    }
}

void clearResources(int);
void startClk();
void startScheduler(int);
Algorithm getAlgorithm();

void enqueue(processData val);
bool dequeue(processData *p);
bool isEmpty();
void pop();

void readInputFile(char* pFileName);

//test functions
void _printQueue(Node* front);
void _childErr(int signum);

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // signal(SIGINT, _childErr);

    processData *prcs;
    
    // 1. Read the input files.
    readInputFile("processes.txt");


    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int algo = getAlgorithm();
    initMsgQ();

    // 3. Initiate and create the scheduler and clock processes.
    startClk(); 
    startScheduler(algo);

    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // To get time use this
    
    // TODO 
    // 5. Create a data structure for processes and provide it with its parameters.
    
    // 7. Clear clock resources

    //Generation Main Loop
    Node* indexNode = front;
    while(true)
    {   
        // 6. Send the information to the scheduler at the appropriate time.
        if(indexNode != NULL && indexNode->data.arrivaltime == getClk()) 
        {
            sendPrcs(&(indexNode->data));
            indexNode = indexNode->next;
        }
    }
    clearResources(0);
}






void clearResources(int signum)
{
    destroyClk(true);
    destroyMsgQ();
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
    else if(clk_id == 0) //Child
    {
        char *const paramList[] = {"./clk.out", NULL};
        execv("./clk.out", paramList);
        //if it executes what is under this line, then execv has failed
        perror("Error in execv'ing to clock");
        exit(EXIT_FAILURE);
    }
    //Parent continues and exits the function
}

void startScheduler(int algo)
{
    pid_t scheduler_id = fork();
    if(scheduler_id == -1) //Error Occured
    {
        perror("Error forking for scheduler");
        exit(EXIT_FAILURE);
    }
    else if(scheduler_id == 0) //Child
    {
        char sAlgo[5]; //converting algo to char
        char sCnt[5];
        sprintf(sAlgo, "%d", algo);
        sprintf(sCnt, "%d", processesCnt);
        char *const paramList[] = {"./scheduler.out", sAlgo, sCnt, NULL};
        execv("./scheduler.out", paramList);

        perror("Error in execv'ing to scheduler");
        exit(EXIT_FAILURE);
    }
}

Algorithm getAlgorithm()
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

void enqueue(processData val)
{
    Node *newNode = malloc(sizeof(Node));

    newNode->data.id = val.id;
    newNode->data.arrivaltime=val.arrivaltime;
    newNode->data.runningtime = val.runningtime;
    newNode->data.priority=val.priority;

    newNode->next = NULL;
    
    //if it is the first Node
    if(front == NULL && rear == NULL)
        //make both front and rear points to the new Node
    {
        front = newNode;
        rear = newNode;
    }
    else
    {
        //add newnode in rear->next
        rear->next = newNode;

        //make the new Node as the rear Node
        rear = newNode;
    }
}

bool dequeue(processData *p)
{
    //used to free the first Node after dequeue
    Node *temp;

    if(front == NULL)
         return false;
    else
    {
        //take backup
        temp = front;

        //make the front Node points to the next Node
        //logically removing the front element
        front = front->next;

        if(front == NULL)
            rear = NULL;

        processData *data = malloc(sizeof(processData));
        data->id = temp->data.id;
        data->arrivaltime = temp->data.arrivaltime;
        data->runningtime = temp->data.runningtime;
        data->priority = temp->data.priority;

        p=data;

        //free the first node
        free(temp);

        return true;
    }
}

void pop()
{
    Node* temp = front;
    front = front->next;
    free(temp);
}

bool isEmpty()
{
    if(rear==NULL & front==NULL)
        return true;
    else 
        return false;
}

void readInputFile(char* pFileName)
{
    FILE *pInputFile;

    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    processData p;

    pInputFile = fopen(pFileName, "r");
    if (pInputFile==NULL)
    {
        perror("Error reading input file!");
        exit(EXIT_FAILURE);
    }
    getline(&line, &len, pInputFile); //read comment line 
    while (fscanf(pInputFile, "%d", &p.id)!= -1) 
    {
        processesCnt++;
        fscanf(pInputFile, "%d", &p.arrivaltime);
        fscanf(pInputFile, "%d", &p.runningtime);
        fscanf(pInputFile, "%d", &p.priority);
        enqueue(p);
    }

    fclose(pInputFile);
}

void _printQueue(Node* front) //Internal function to test the correctness of inputting
{
    while(front)
    {
        printf("%d\n", front->data);
        front = front->next;
    }
}


void _childErr(int signum)
{

}