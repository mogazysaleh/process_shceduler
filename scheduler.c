#include "headers.h"

Algorithm algo;
int processesCnt;
int quantum;

int deadSize = 0;
PCB** deadQ;
PCB* runningP = NULL;
FILE *pLog;

typedef struct Node
{
    PCB *data;
    struct Node *next;
}Node;

Node *front = NULL, *rear = NULL;

void initMsgQ()
{
    qid = msgget(QKEY, 0);
    if(qid == -1)
    {
        perror("Error initializing MSGQ");
        exit(EXIT_FAILURE);
    }
}

void freeMem();

PCB * dequeue();
void enqueue(PCB *val);
int forkPrcs(int runningTime);
void schdSRTN();
void schdRR();
void handler(int signum);

void _printQueue(Node* front);

void initializePrcs(PCB* prcs1, const processData* rcvd);

void createSchedulerLog();
void writeSchedulerLog();

/////////////////////////PQ/////////////////////////
PCB** priorityQ;
int qSize = 0;
bool insert(PCB* prcs);
PCB* pop();
PCB* peak();

/////////////////////////Internal PQ functions/////////////////////////
void _heapifySRTN(int index);
void _heapifyHPF(int index);
void _printQ();
void _swap(int ind1, int ind2);


int main(int argc, char * argv[])
{
    processData rcvd;
    int currentClk;
    algo = atoi(argv[1]);
    processesCnt = atoi(argv[2]);
    quantum = atoi(argv[3]);

    if(algo != RR)
        priorityQ = calloc(processesCnt, sizeof(PCB*));
    
    deadQ = calloc(processesCnt, sizeof(PCB*));
        
    signal(SIGUSR1, handler);
    
    initClk();
    initMsgQ();

    createSchedulerLog();
    
    while(true)
    {
        currentClk = getClk();
        if(msgrcv(qid, &rcvd, sizeof(processData) - sizeof(long), 0, IPC_NOWAIT) != -1)
        {
            printf("RCVD\n");
            PCB* prcs = (PCB*) malloc(sizeof(PCB));
            initializePrcs(prcs, &rcvd);
            if(algo == RR)
                enqueue(prcs);
            else
                insert(prcs);
            // _printQ(priorityQ, qSize);
            // _printQueue(front);
            
        }
        if(deadSize == processesCnt)
        {
            //output file
               //close file descriptor
            freeMem();          //freeing allocated dynamic memory
            destroyClk(false);  //destroying clock
            exit(EXIT_SUCCESS); //exitting program
        }
        schdRR();
        //call your algo
        if(getClk() == 30)
            exit(1);
        while(currentClk == getClk());
    }
}

bool insert(PCB* prcs)
{
    if(qSize == processesCnt)
        return false;
    priorityQ[++qSize] = prcs;

    if(algo == RR || qSize == 1)
        return true;
    else if(algo == SRTN)
        _heapifySRTN(qSize/2);
    else if(algo == HPF)
        _heapifyHPF(qSize/2);

    return true; //do not heapify in case of algo == RR
}

void initializePrcs(PCB* prcs1, const processData* rcvd)
{
    prcs1->id = rcvd->id;
    prcs1->arrivalTime =  rcvd->arrivaltime;
    prcs1->priority = rcvd->priority;
    prcs1->runningTime = rcvd->runningtime;
    prcs1->remainingTime = rcvd->runningtime;
    prcs1->startTime = -1; //meaning that process has never run before
}
void _printQ()
{
    for(int i = 1; i <= qSize; i++)
        printf("%d  %d  %d  %d\n", priorityQ[i]->id, priorityQ[i]->priority, priorityQ[i]->runningTime, priorityQ[i]->arrivalTime);
}
void _heapifySRTN(int index)
{
    int least = index, left = 2 * index, right = left + 1;

    if((left <= qSize) && (priorityQ[left]->remainingTime < priorityQ[least]->remainingTime))
		least = left;
	
	if((right <= qSize) && (priorityQ[right]->remainingTime < priorityQ[least]->remainingTime))
		least = right;

	if(least == index)
		return;
    
	_swap(least, index);
	_heapifySRTN(least);
}
void _heapifyHPF(int index)
{
    int least = index, left = 2 * index, right = left + 1;

    if((left <= qSize) && (priorityQ[left]->priority < priorityQ[least]->priority))
		least = left;
	
	if((right <= qSize) && (priorityQ[right]->priority < priorityQ[least]->priority))
		least = right;

	if(least == index)
		return;
    
	_swap(least, index);
	_heapifyHPF(least);
}
void _swap(int ind1, int ind2)
{
    PCB* tmp = priorityQ[ind1];
    priorityQ[ind1] = priorityQ[ind2];
    priorityQ[ind2] = tmp;
}

PCB* pop()
{
    if(qSize == 0)
        return NULL;
    PCB* prcs = priorityQ[1];
    priorityQ[1] = priorityQ[qSize--];
    if(qSize > 1)
        if(algo == SRTN)
            _heapifySRTN(1);
        else if(algo == HPF)
            _heapifyHPF(1);

    return prcs;
}

PCB* peak()
{
    if(qSize == 0)
        return NULL;
    
    return priorityQ[1];
}

void freeMem()
{
    for(int i = 0; i < processesCnt; i++)
        free(priorityQ[i]);

    for(int i = 0; i < processesCnt; i++)
        free(deadQ[i]);

    free(priorityQ);
    free(deadQ);
}

void schdSRTN()
{
    if (peak() == NULL)
        return;

    if (runningP) //There is a process already running
    {
        if (runningP->remainingTime > peak()->remainingTime) //if there is a process better than the currently running, switch
        {
            //stop the running process
            kill(runningP->processId, SIGSTOP);
            runningP->state = READY;

            //switch the two processes
            insert(runningP);
            runningP = pop();

            //change state of prcs that has just been popped
            runningP->state = RUNNING;

            if (runningP->startTime == -1) //process did not run before
            {
                runningP->startTime = getClk();
                runningP->processId = forkPrcs(runningP->runningTime);
            }
            else //process did run before
                kill(runningP->processId, SIGCONT);
        }
    }
    else
    {
        runningP = pop();
        runningP->state = RUNNING;
        if (runningP->startTime == -1)
        {
            runningP->startTime = getClk();
            runningP->processId = forkPrcs(runningP->runningTime);
        }
        else
            kill(runningP->processId, SIGCONT);
    }
}

int forkPrcs(int runningTime)
{
    pid_t schdPid = getpid();
    pid_t prcsPid = fork();

    if(prcsPid == -1) //error happened when forking
    {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    }
    else if(prcsPid == 0) //execv'ing to process
    {
        char sRunningTime[5] = {0};
        char sPid[7] = {0};
        sprintf(sPid, "%d", schdPid);
        sprintf(sRunningTime, "%d", runningTime);
        char *const paramList[] = {"./process.out", sRunningTime, sPid, NULL};
        execv("./process.out", paramList);
        
        //if it executes what is under this line, then execv has failed
        perror("Error in execv'ing to clock");
        exit(EXIT_FAILURE);
    }
    return prcsPid;
}

void schdRR()
{
    if(runningP == NULL) //no process is running
    {
        runningP = dequeue();
        if(runningP == NULL)
            return;
    }

    runningP->remainingTime=runningP->remainingTime-1;
    
    if(( (runningP->runningTime)-(runningP->remainingTime) ) % quantum == 0 //process stopping running
         && (runningP->runningTime != runningP->remainingTime) && (runningP->state == RUNNING))
    {
        kill(runningP->processId,SIGSTOP);
        runningP->state = READY;
        writeSchedulerLog(runningP, getClk(), "stopped");
        enqueue(runningP);
        runningP=dequeue();
    }

    if(runningP->startTime==-1) //process running for the first time
    {
        runningP->startTime= getClk();
        runningP->state = RUNNING;
        runningP->processId =  forkPrcs(runningP->runningTime);
        writeSchedulerLog(runningP, getClk(), "started");
    }
    else if(runningP->state == READY)
    {
        runningP->state = RUNNING;
        kill(runningP->processId,SIGCONT);
        writeSchedulerLog(runningP, getClk(), "resumed");

    }
    
    printf("RUNNING: %d\n", runningP->id);
}

void createSchedulerLog()
{
    pLog = fopen("Scheduler.log.txt", "w");
    if (pLog ==NULL)
    {
        perror("Error creating/ openning Scheduler.log!");
        exit(EXIT_FAILURE);
    }
    fprintf(pLog, "#At time x process y state arr w total z remain y wait k\n");
    fclose(pLog);
}

void writeSchedulerLog(PCB* process, int time, char* state)
{
    pLog = fopen("Scheduler.log.txt", "a");
    int waitingTime = time-process->arrivalTime-(process->runningTime-process->remainingTime);
    if(state == "finished")
        fprintf(pLog, "At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %d\n"
         ,time , process->id, state, process->arrivalTime, process->runningTime, process->remainingTime, waitingTime, process->finishTime - process->arrivalTime, (process->finishTime - process->arrivalTime)/process->runningTime );
    else
        fprintf(pLog, "At time %d process %d %s arr %d total %d remain %d wait %d\n"
         ,time , process->id, state, process->arrivalTime, process->runningTime, process->remainingTime, waitingTime);
         
    fclose(pLog);
}


void enqueue(PCB *val)
{
    Node *newNode = malloc(sizeof(Node));
    
    newNode->data = val;

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

PCB * dequeue()
{
    if(front == NULL)
         return NULL;
    else
    {
        //make the front Node points to the next Node
        //logically removing the front element
        PCB *tmp = front->data;
        front = front->next;
        if(front == NULL)
            rear = NULL;
        return tmp;
    }
}

void handler(int signum)
{
    
    int status;
    int pid= wait(&status);
    printf("ZERO: %d\n", runningP->processId);
    if (runningP->processId == pid)
    {
        //deadQ.insert(runningP);
        runningP->finishTime = getClk();
        runningP->waitingTime=runningP->finishTime-runningP->arrivalTime;
        writeSchedulerLog(runningP, getClk(), "finished");
        runningP = NULL;
    }
}

void _printQueue(Node* front) //Internal function to test the correctness of inputting
{
    while(front)
    {
        printf("%d\n", front->data->id);
        front = front->next;
    }
}
