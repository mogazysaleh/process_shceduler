#include "headers.h"

void clearResources(int);
void startClk();
void startScheduler(int);
void* schedulerShm();
enum Algorithm getAlgorithm();


struct processData;
struct Node;

struct Node *front = NULL, *rear = NULL;

void enqueue(struct processData val);
bool dequeue(struct processData **p);
bool isEmpty();

void readInputFile(char* pFileName);

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    readInputFile("processes.txt");
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int algo = getAlgorithm();
    //printf("algoo is %d", algo);

    // 3. Initiate and create the scheduler and clock processes.
    startClk(); 
    startScheduler(algo);

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

void startScheduler(int algo)
{
    pid_t scheduler_id = fork();
    if(scheduler_id == -1)
    {
        perror("Error forking for scheduler");
        exit(EXIT_FAILURE);
    }
    else
    {
        char s[3];
        sprintf(s,"%d", algo);
        //printf("algo is %s", s);
        char *const paramList[] = {"./scheduler", s, NULL};
        execv("./scheduler", paramList);

        perror("Error in execv'ing to scheduler");
        exit(EXIT_FAILURE);
    }
}

void* schedulerShm()
{
    int key = shmget(1, 1024, IPC_EXCL | 0644);
    void* shmaddr = shmat(key, (void *) 0, 0);
    return shmaddr;
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

struct processData
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
};

struct Node
{
    struct processData data;
    struct Node *next;
};



void enqueue(struct processData val)
{
    struct Node *newNode = malloc(sizeof(struct Node));

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

bool dequeue(struct processData **p)
{
    //used to free the first Node after dequeue
    struct Node *temp;

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

        struct processData *data = malloc(sizeof(struct processData));
        data->id = temp->data.id;
        data->arrivaltime = temp->data.arrivaltime;
        data->runningtime = temp->data.runningtime;
        data->priority = temp->data.priority;

        *p=data;

        //free the first node
        free(temp);

        return true;
    }
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
    struct processData p;

    pInputFile = fopen(pFileName, "r");
    if (pInputFile==NULL)
    {
        perror("Error reading input file!");
        exit(EXIT_FAILURE);
    }
    getline(&line, &len, pInputFile); //read comment line 
    while (fscanf(pInputFile, "%d", &p.id)!= -1) 
    { 
        fscanf(pInputFile, "%d", &p.arrivaltime);
        fscanf(pInputFile, "%d", &p.runningtime);
        fscanf(pInputFile, "%d", &p.priority);
        enqueue(p);
    }

    fclose(pInputFile);
}
