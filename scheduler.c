#include "headers.h"



void initMsgQ()
{
    qid = msgget(QKEY, 0);
    if(qid == -1)
    {
        perror("Error initializing MSGQ");
        exit(EXIT_FAILURE);
    }
}

void freeMem(PCB** priorityQ, int maxSize);

bool insert(PCB** priorityQ, PCB* prcs, int* qSize, int maxSize, Algorithm algo);
PCB* pop(PCB** priorityQ, int *qSize, Algorithm algo);
PCB* peak(PCB** priorityQ, int qSize);

void cpyProcess(PCB* prcs1, const processData* rcvd);

/////////////////////////Internal PQ functions/////////////////////////
void _heapifySRTN(PCB** priorityQ, int qSize, int index);
void _heapifyHPF(PCB** priorityQ, int qSize, int index);
void _printQ(PCB** priorityQ, int qSize);
void _swap(PCB** priorityQ, int ind1, int ind2);

int main(int argc, char * argv[])
{
    Algorithm algo = atoi(argv[1]);
    int processesCnt = atoi(argv[2]);

    processData rcvd;
    PCB** priorityQ = calloc(processesCnt, sizeof(PCB*));
    PCB** deadQ = calloc(processesCnt, sizeof(PCB*));
    int qSize = 0;
    int deadSize = 0;
    
    initClk();
    initMsgQ();

    while(true)
    {
        if(msgrcv(qid, &rcvd, sizeof(processData), 0, IPC_NOWAIT) != -1)
        {
            PCB* prcs = (PCB*) malloc(sizeof(PCB));
            cpyProcess(prcs, &rcvd);
            insert(priorityQ, prcs, &qSize, processesCnt, algo);
            _printQ(priorityQ, qSize);
        }
    }
}

bool insert(PCB** priorityQ, PCB* prcs, int *qSize, int maxSize, Algorithm algo)
{
    if((*qSize) == maxSize)
        return false;
    priorityQ[++(*qSize)] = prcs;

    if(algo == RR || (*qSize) == 1)
        return true;
    else if(algo == SRTN)
        _heapifySRTN(priorityQ, (*qSize), (*qSize)/2);
    else if(algo == HPF)
        _heapifyHPF(priorityQ, (*qSize), (*qSize)/2);

    return true; //do not heapify in case of algo == RR
}

void cpyProcess(PCB* prcs1, const processData* rcvd)
{
    prcs1->id = rcvd->id;
    prcs1->arrivalTime =  rcvd->arrivaltime;
    prcs1->priority = rcvd->priority;
    prcs1->runningTime = rcvd->runningtime;
}

void _printQ(PCB** priorityQ, int qSize)
{
    for(int i = 1; i <= qSize; i++)
        printf("%d  %d  %d  %d\n", priorityQ[i]->id, priorityQ[i]->priority, priorityQ[i]->runningTime, priorityQ[i]->arrivalTime);
}

void _heapifySRTN(PCB** priorityQ, int qSize, int index)
{
    int least = index, left = 2 * index, right = left + 1;

    if((left <= qSize) && (priorityQ[left]->remainingTime < priorityQ[least]->remainingTime))
		least = left;
	
	if((right <= qSize) && (priorityQ[right]->remainingTime < priorityQ[least]->remainingTime))
		least = right;

	if(least == index)
		return;
    
	_swap(priorityQ, least, index);
	_heapifySRTN(priorityQ, qSize, least);
}
void _heapifyHPF(PCB** priorityQ, int index, int qSize)
{
    int least = index, left = 2 * index, right = left + 1;

    if((left <= qSize) && (priorityQ[left]->priority < priorityQ[least]->priority))
		least = left;
	
	if((right <= qSize) && (priorityQ[right]->priority < priorityQ[least]->priority))
		least = right;

	if(least == index)
		return;
    
	_swap(priorityQ, least, index);
	_heapifyHPF(priorityQ, qSize, least);
}

void _swap(PCB** priorityQ, int ind1, int ind2)
{
    PCB* tmp = priorityQ[ind1];
    priorityQ[ind1] = priorityQ[ind2];
    priorityQ[ind2] = tmp;
}

PCB* pop(PCB** priorityQ, int *qSize, Algorithm algo)
{
    if((*qSize) == 0)
        return NULL;
    PCB* prcs = priorityQ[1];
    priorityQ[1] = priorityQ[(*qSize)--];
    if((*qSize) > 1)
        if(algo == SRTN)
            _heapifySRTN(priorityQ, (*qSize), 1);
        else if(algo == HPF)
            _heapifyHPF(priorityQ, (*qSize), 1);

    return prcs;
}

PCB* peak(PCB** priorityQ, int qSize)
{
    if(qSize == 0)
        return NULL;
    
    return priorityQ[0];
}

void freeMem(PCB** priorityQ, int maxSize)
{
    for(int i = 0; i < maxSize; i++)
        free(priorityQ[i]);

    free(priorityQ);
}

// void SRTN(PCB** priorityQ)
// {
//     static PCB* runningP = NULL;
//     if (priorityQ->peak() == NULL) //PQ is empty
//     {
//         if (runningP)//if you have an already running process, decrement
//         {
//             runningP->remainingTime--;
//         }
//         return;
//     }
//     if (runningP)
//     {
//         runningP->remainingTime--;
        
//         if (runningP->remainingTime > priorityQ->peak()->remainingTime)
//         {
//             Kill(runningP->processId, SIGSTOP);
//             runningP->ProcessState = READY;

//             PCB* temp = runningP;
//             runningP = priorityQ->pop();
//             priorityQ->insert(temp);
            
//             runningP->ProcessState = RUNNING;

//             if (runningP->startTime == -1)
//             {
//                 runningP->startTime = getClk();
//                 char *const paramList[] = {"./process.out", NULL};
//                 execv("./process.out", paramList);
//                 //if it executes what is under this line, then execv has failed
//                 perror("Error in execv'ing to process");
//                 exit(EXIT_FAILURE);
//             }
//             else
//             {
//                 Kill(runningP->processId, SIGCONT);//
//             }
            
//         }
//     }
//     else
//     {
//         runningP = priorityQ->pop();
//         runningP->ProcessState = RUNNING;
//         if (runningP->startTime == -1)
//         {
//             runningP->startTime = getClk();
//             char *const paramList[] = {"./process.out", NULL};
//             execv("./process.out", paramList);
//             //if it executes what is under this line, then execv has failed
//             perror("Error in execv'ing to process");
//             exit(EXIT_FAILURE);
//         }
//         else
//         {
//             Kill(runningP->processId, SIGCONT);//
//         }
//     }

// }

