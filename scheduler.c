#include "headers.h"

Algorithm algo;
int processesCnt;
int quantum;

int deadSize = 0;
PCB** deadQ;
PCB* runningP = NULL;
FILE *pLog;

void runAlgo();

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
void schdHPF();
void handler(int signum);

void _printQueue(Node* front);

void initializePrcs(PCB* prcs1, const processData* rcvd);
float calculateSD(float data[],int n);
void createSchedulerLog();
void writeSchedulerLog();
void writeSchedulerPerf();

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
    
    int currentClk = 1;
    while(true)
    {
        if(msgrcv(qid, &rcvd, sizeof(processData) - sizeof(long), 0, IPC_NOWAIT) != -1)
        {
            PCB* prcs = (PCB*) malloc(sizeof(PCB));
            initializePrcs(prcs, &rcvd);
            if(algo == RR)
                enqueue(prcs);
            else
                insert(prcs);
        }
        if(currentClk != getClk())
        {
            currentClk = getClk();
            runAlgo();
            if(deadSize == processesCnt)
            {
                //output file
                //close file descriptor
                writeSchedulerPerf();
                freeMem();          //freeing allocated dynamic memory
                destroyClk(false);  //destroying clock
                exit(EXIT_SUCCESS); //exitting program
            }
        }
        
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
        free(deadQ[i]);
    
    if(algo != RR)
        free(priorityQ);
    free(deadQ);
}

void schdSRTN()
{
    if (peak() == NULL)
        return;

    if (runningP) //There is a process already running
    {
        runningP->remainingTime--;
        if (runningP->remainingTime > peak()->remainingTime) //if there is a process better than the currently running, switch
        {
            //stop the running process
            writeSchedulerLog(runningP, getClk(), "stopped");
            kill(runningP->processId, SIGSTOP);
            runningP->state = READY;

            //switch the two processes
            insert(runningP);
            runningP = pop();

            //change state of prcs that has just been popped
            runningP->state = RUNNING;

            if (runningP->startTime == -1) //process did not run before
            {
                writeSchedulerLog(runningP, getClk(), "started");
                runningP->startTime = getClk();
                runningP->processId = forkPrcs(runningP->runningTime);
            }
            else //process did run before
            {
                writeSchedulerLog(runningP, getClk(), "resumed");
                kill(runningP->processId, SIGCONT);
            }
        }
    }
    else
    {
        runningP = pop();
        runningP->state = RUNNING;
        if (runningP->startTime == -1)
        {
            printf("clock: %d\n", getClk());
            writeSchedulerLog(runningP, getClk(), "started");
            runningP->startTime = getClk();
            runningP->processId = forkPrcs(runningP->runningTime);
        }
        else
        {
            writeSchedulerLog(runningP, getClk(), "resumed");
            kill(runningP->processId, SIGCONT);
        }
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
    runningP->remainingTime=runningP->remainingTime-1;
}

void schdHPF()
{
    if(runningP == NULL)
    {
        runningP = pop();
        if(runningP == NULL)
            return;
    }
    
    if(runningP->startTime==-1)
    {
        runningP->startTime= getClk();
        runningP->processId =  forkPrcs(runningP->runningTime);
        writeSchedulerLog(runningP, getClk(), "started");
    }
    
    runningP->remainingTime=runningP->remainingTime-1;

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
        fprintf(pLog, "At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %f\n"
         ,time , process->id, state, process->arrivalTime, process->runningTime, process->remainingTime, waitingTime, process->finishTime - process->arrivalTime, (process->finishTime - process->arrivalTime)/(float)process->runningTime );
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
        PCB *data = (PCB*)malloc(sizeof(PCB));
        //make the front Node points to the next Node
        //logically removing the front element
        Node* tmp = front;
        data->arrivalTime = front->data->arrivalTime;
        data->finishTime = front->data->finishTime;
        data->id = front->data->id;
        data->priority = front->data->priority;
        data->processId = front->data->processId;
        data->remainingTime = front->data->remainingTime;
        data->runningTime = front->data->runningTime;
        data->startTime = front->data->startTime;
        data->state = front->data->state;
        data->waitingTime = front->data->waitingTime;
        front = front->next;
        if(front == NULL)
            rear = NULL;
        free(tmp);

        return data;
    }
}
void runAlgo()
{
    if(algo == SRTN)
        schdSRTN();
    else if (algo == RR)
        schdRR();
    else if (algo == HPF)
        schdHPF();
}
void handler(int signum)
{
    
    int status;
    int pid= wait(&status);

    if (runningP->processId == pid)
    {
        deadQ[deadSize++] = runningP;
        runningP->finishTime = getClk();
        runningP->remainingTime = 0;
        runningP->waitingTime=runningP->finishTime-runningP->arrivalTime-runningP->runningTime;
        writeSchedulerLog(runningP, getClk(), "finished");
        runningP = NULL;
    }

    //call your algo
    runAlgo();
}

void _printQueue(Node* front) //Internal function to test the correctness of inputting
{
    while(front)
    {
        printf("%d\n", front->data->id);
        front = front->next;
    }
}

float calculateSD(float data[],int n) {
    float sum = 0.0, mean, SD = 0.0;
    int i;
    for (i = 0; i < n; ++i) {
        sum += data[i];
    }
    mean = sum / n;
    for (i = 0; i < n; ++i) {
        SD += ((data[i] - mean) * (data[i] - mean));
    }
    return sqrt(SD / (n-1));
}

void writeSchedulerPerf()
{
    FILE *pPerf;
    PCB* finishedP = NULL;
    pPerf = fopen("Scheduler.perf.txt", "w");
    if (pPerf ==NULL)
    {
        perror("Error creating/ openning Scheduler.perf!");
        exit(EXIT_FAILURE);
    }
    int utilization=100;
    float avgWTA =0;
    float avgWaiting=0;
    int TotalWaiting=0;
    float stdWTA=0;
    float TA=0;
    float TAp = 0;
    float WTA =0;
    float excTime = 0;
    float* arrWTA= malloc(sizeof(float)*processesCnt);

    for(int i = 0 ; i < processesCnt ; i++)
    {
        finishedP = deadQ[i];
         if(finishedP == NULL)
            break;

         TotalWaiting = TotalWaiting + finishedP->waitingTime;
         TAp = (finishedP->finishTime - finishedP->arrivalTime);
         TA = TA + (finishedP->finishTime - finishedP->arrivalTime);
         excTime = excTime + finishedP->runningTime;
         WTA = WTA + TAp/ (float)(finishedP->runningTime);
         arrWTA[i]= TAp / (float)(finishedP->runningTime);
    }

    avgWaiting = (float)(TotalWaiting)/processesCnt;
    avgWTA = WTA / processesCnt;
    stdWTA = calculateSD(arrWTA,processesCnt);

    fprintf(pPerf, "CPU utilization =  %d\n",utilization);
    fprintf(pPerf, "Avg WTA = %f \n",avgWTA);
    fprintf(pPerf, "Avg Waiting = %f\n",avgWaiting);
    fprintf(pPerf, "Std WTA = %f\n", stdWTA);
    fclose(pPerf);
}
