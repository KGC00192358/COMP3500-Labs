
/*****************************************************************************\
 * Laboratory Exercises COMP 3500                                              *
 * Author: Saad Biaz                                                           *
 * Updated 6/5/2017 to distribute to students to redo Lab 1                    *
 * Updated 5/9/2017 for COMP 3500 labs                                         *
 * Date  : February 20, 2009                                                   *
 \*****************************************************************************/

/*****************************************************************************\
 *                             Global system headers                           *
 \*****************************************************************************/


#include "common2.h"

/*****************************************************************************\
 *                             Global data types                               *
 \*****************************************************************************/

typedef enum {TAT,RT,CBT,THGT,WT, JQ} Metric;
typedef int bool;
enum { false, true };


/*****************************************************************************\
 *                             Global definitions                              *
 \*****************************************************************************/
#define MAX_QUEUE_SIZE 10 
#define FCFS            1 
#define RR              3 


#define MAXMETRICS      6 

#define OMAP            0
#define PAGING          1
#define BESTFIT         2
#define WORSTFIT        3
#define NONE            -1

#define PAGESIZE 	8192
#define FREEMEM		0
#define COMPACT		1


/*****************************************************************************\
 *                            Global data structures                           *
 \*****************************************************************************/
typedef struct FreeMemoryHoleTag {
	Memory Address;
	Memory Size;
	struct FreeMemoryHoleTag *previous;
	struct FreeMemoryHoleTag *next;
} FreeMemoryHole;

typedef struct MemoryQueueParmsTag {
	FreeMemoryHole *Head;
	FreeMemoryHole *Tail;
	Quantity NumberOfHoles;
} MemoryQueueParms;
FreeMemoryHole generic;
MemoryQueueParms* MemoryQueues[2]; //Free and Compaction
FreeMemoryHole* FreeHead = NULL;
FreeMemoryHole* FreeTail = NULL;


/*****************************************************************************\
 *                                  Global data                                *
 \*****************************************************************************/

Quantity NumberofJobs[MAXMETRICS]; // Number of Jobs for which metric was collected
Average  SumMetrics[MAXMETRICS]; // Sum for each Metrics
int MemoryPolicy = WORSTFIT;

int  pagesAvailable = 1048576 / PAGESIZE;
int  nonavailablePages = 0;

/*****************************************************************************\
 *                               Function prototypes                           *
 \*****************************************************************************/

void                 ManageProcesses(void);
void                 NewJobIn(ProcessControlBlock whichProcess);
void                 BookKeeping(void);
Flag                 ManagementInitialization(void);
void                 LongtermScheduler(void);
void                 IO();
void                 CPUScheduler(Identifier whichPolicy);
ProcessControlBlock *SRTF();
void                 Dispatcher();
bool		     lockMemory(ProcessControlBlock* p);
bool		     freeMemory(FreeMemoryHole** Head, FreeMemoryHole** Tail, ProcessControlBlock* p);
bool			 bestFitFind(FreeMemoryHole** Head, FreeMemoryHole** Tail, ProcessControlBlock* p);
bool			 worstFitFind(FreeMemoryHole** Head, FreeMemoryHole** Tail, ProcessControlBlock* p);

/*****************************************************************************\
 * function: main()                                                            *
 * usage:    Create an artificial environment operating systems. The parent    *
 *           process is the "Operating Systems" managing the processes using   *
 *           the resources (CPU and Memory) of the system                      *
 *******************************************************************************
 * Inputs: ANSI flat C command line parameters                                 *
 * Output: None                                                                *
 *                                                                             *
 * INITIALIZE PROGRAM ENVIRONMENT                                              *
 * START CONTROL ROUTINE                                                       *
 \*****************************************************************************/

int main (int argc, char **argv) {
	if (Initialization(argc,argv)){
		ManageProcesses();
	}
} /* end of main function */

/***********************************************************************\
 * Input : none                                                          *
 * Output: None                                                          *
 * Function: Monitor Sources and process events (written by students)    *
 \***********************************************************************/

void ManageProcesses(void){
	ManagementInitialization();
	MemoryQueueParms Free = {NULL, NULL};
	MemoryQueueParms Comp = {NULL, NULL};
	MemoryQueues[FREEMEM] = &Free;
	MemoryQueues[COMPACT] = &Comp;
	while (1) {
		IO();
		CPUScheduler(PolicyNumber);
		Dispatcher();
	}
}

/***********************************************************************\
 * Input : none                                                          *          
 * Output: None                                                          *        
 * Function:                                                             *
 *    1) if CPU Burst done, then move process on CPU to Waiting Queue    *
 *         otherwise (RR) return to rReady Queue                         *                           
 *    2) scan Waiting Queue to find processes with complete I/O          *
 *           and move them to Ready Queue                                *         
 \***********************************************************************/
void IO() {
	ProcessControlBlock *currentProcess = DequeueProcess(RUNNINGQUEUE); 
	if (currentProcess){
		if (currentProcess->RemainingCpuBurstTime <= 0) { // Finished current CPU Burst
			currentProcess->TimeEnterWaiting = Now(); // Record when entered the waiting queue
			EnqueueProcess(WAITINGQUEUE, currentProcess); // Move to Waiting Queue
			currentProcess->TimeIOBurstDone = Now() + currentProcess->IOBurstTime; // Record when IO completes
			currentProcess->state = WAITING;
		} else { // Must return to Ready Queue                
			currentProcess->JobStartTime = Now();                                               
			EnqueueProcess(READYQUEUE, currentProcess); // Mobe back to Ready Queue
			currentProcess->state = READY; // Update PCB state 
		}
	}

	/* Scan Waiting Queue to find processes that got IOs  complete*/
	ProcessControlBlock *ProcessToMove;
	/* Scan Waiting List to find processes that got complete IOs */
	ProcessToMove = DequeueProcess(WAITINGQUEUE);
	if (ProcessToMove){
		Identifier IDFirstProcess =ProcessToMove->ProcessID;
		EnqueueProcess(WAITINGQUEUE,ProcessToMove);
		ProcessToMove = DequeueProcess(WAITINGQUEUE);
		while (ProcessToMove){
			if (Now()>=ProcessToMove->TimeIOBurstDone){
				ProcessToMove->RemainingCpuBurstTime = ProcessToMove->CpuBurstTime;
				ProcessToMove->JobStartTime = Now();
				EnqueueProcess(READYQUEUE,ProcessToMove);
			} else {
				EnqueueProcess(WAITINGQUEUE,ProcessToMove);
			}
			if (ProcessToMove->ProcessID == IDFirstProcess){
				break;
			}
			ProcessToMove =DequeueProcess(WAITINGQUEUE);
		} // while (ProcessToMove)
	} // if (ProcessToMove)
}

/***********************************************************************\    
 * Input : whichPolicy (1:FCFS, 2: SRTF, and 3:RR)                      *        
 * Output: None                                                         * 
 * Function: Selects Process from Ready Queue and Puts it on Running Q. *
 \***********************************************************************/
void CPUScheduler(Identifier whichPolicy) {
	ProcessControlBlock *selectedProcess;
	if ((whichPolicy == FCFS) || (whichPolicy == RR)) {
		selectedProcess = DequeueProcess(READYQUEUE);
	} else{ // Shortest Remaining Time First 
		selectedProcess = SRTF();
	}
	if (selectedProcess) {
		selectedProcess->state = RUNNING; // Process state becomes Running                                     
		EnqueueProcess(RUNNINGQUEUE, selectedProcess); // Put process in Running Queue                         
	}
}

/***********************************************************************\                         
 * Input : None                                                         *                                     
 * Output: Pointer to the process with shortest remaining time (SRTF)   *                                     
 * Function: Returns process control block with SRTF                    *                                     
 \***********************************************************************/
ProcessControlBlock *SRTF() {
	/* Select Process with Shortest Remaining Time*/
	ProcessControlBlock *selectedProcess, *currentProcess = DequeueProcess(READYQUEUE);
	selectedProcess = (ProcessControlBlock *) NULL;
	if (currentProcess){
		TimePeriod shortestRemainingTime = currentProcess->TotalJobDuration - currentProcess->TimeInCpu;
		Identifier IDFirstProcess =currentProcess->ProcessID;
		EnqueueProcess(READYQUEUE,currentProcess);
		currentProcess = DequeueProcess(READYQUEUE);
		while (currentProcess){
			if (shortestRemainingTime >= (currentProcess->TotalJobDuration - currentProcess->TimeInCpu)){
				EnqueueProcess(READYQUEUE,selectedProcess);
				selectedProcess = currentProcess;
				shortestRemainingTime = currentProcess->TotalJobDuration - currentProcess->TimeInCpu;
			} else {
				EnqueueProcess(READYQUEUE,currentProcess);
			}
			if (currentProcess->ProcessID == IDFirstProcess){
				break;
			}
			currentProcess =DequeueProcess(READYQUEUE);
		} // while (ProcessToMove)
	} // if (currentProcess)
	return(selectedProcess);
}

/***********************************************************************\  
 * Input : None                                                         *   
 * Output: None                                                         *   
 * Function:                                                            *
 *  1)If process in Running Queue needs computation, put it on CPU      *
 *              else move process from running queue to Exit Queue      *     
 \***********************************************************************/
void Dispatcher() {
	double start;
	ProcessControlBlock *processOnCPU = Queues[RUNNINGQUEUE].Tail; // Pick Process on CPU
	if (!processOnCPU) { // No Process in Running Queue, i.e., on CPU
		return;
	}
	if(processOnCPU->TimeInCpu == 0.0) { // First time this process gets the CPU
		SumMetrics[RT] += Now()- processOnCPU->JobArrivalTime;
		NumberofJobs[RT]++;
		processOnCPU->StartCpuTime = Now(); // Set StartCpuTime
	}

	if (processOnCPU->TimeInCpu >= processOnCPU-> TotalJobDuration) { // Process Complete
		printf(" >>>>>Process # %d complete, %d Processes Completed So Far <<<<<<\n",
				processOnCPU->ProcessID,NumberofJobs[THGT]);   
		processOnCPU=DequeueProcess(RUNNINGQUEUE);
		EnqueueProcess(EXITQUEUE,processOnCPU);
		freeMemory(&FreeHead, &FreeTail, processOnCPU);
		NumberofJobs[THGT]++;
		NumberofJobs[TAT]++;
		NumberofJobs[WT]++;
		NumberofJobs[CBT]++;
		SumMetrics[TAT]     += Now() - processOnCPU->JobArrivalTime;
		SumMetrics[WT]      += processOnCPU->TimeInReadyQueue;


		// processOnCPU = DequeueProcess(EXITQUEUE);
		// XXX free(processOnCPU);

	} else { // Process still needs computing, out it on CPU
		TimePeriod CpuBurstTime = processOnCPU->CpuBurstTime;
		processOnCPU->TimeInReadyQueue += Now() - processOnCPU->JobStartTime;
		if (PolicyNumber == RR){
			CpuBurstTime = Quantum;
			if (processOnCPU->RemainingCpuBurstTime < Quantum)
				CpuBurstTime = processOnCPU->RemainingCpuBurstTime;
		}
		processOnCPU->RemainingCpuBurstTime -= CpuBurstTime;
		// SB_ 6/4 End Fixes RR 
		TimePeriod StartExecution = Now();
		OnCPU(processOnCPU, CpuBurstTime); // SB_ 6/4 use CpuBurstTime instead of PCB-> CpuBurstTime
		processOnCPU->TimeInCpu += CpuBurstTime; // SB_ 6/4 use CpuBurstTime instead of PCB-> CpuBurstTimeu
		SumMetrics[CBT] += CpuBurstTime;
	}
}

/***********************************************************************\
 * Input : None                                                          *
 * Output: None                                                          *
 * Function: This routine is run when a job is added to the Job Queue    *
 \***********************************************************************/
void NewJobIn(ProcessControlBlock whichProcess){
	ProcessControlBlock *NewProcess;
	/* Add Job to the Job Queue */
	NewProcess = (ProcessControlBlock *) malloc(sizeof(ProcessControlBlock));
	memcpy(NewProcess,&whichProcess,sizeof(whichProcess));
	NewProcess->TimeInCpu = 0; // Fixes TUX error
	NewProcess->RemainingCpuBurstTime = NewProcess->CpuBurstTime; // SB_ 6/4 Fixes RR
	EnqueueProcess(JOBQUEUE,NewProcess);
	DisplayQueue("Job Queue in NewJobIn",JOBQUEUE);
	LongtermScheduler(); /* Job Admission  */
}


/***********************************************************************\                                                   
 * Input : None                                                         *                                                    
 * Output: None                                                         *                                                    
 * Function:                                                            *
 * 1) BookKeeping is called automatically when 250 arrived              *
 * 2) Computes and display metrics: average turnaround  time, throughput*
 *     average response time, average waiting time in ready queue,      *
 *     and CPU Utilization                                              *                                                     
 \***********************************************************************/
void BookKeeping(void){
	double end = Now(); // Total time for all processes to arrive
	Metric m;

	// Compute averages and final results
	if (NumberofJobs[TAT] > 0){
		SumMetrics[TAT] = SumMetrics[TAT]/ (Average) NumberofJobs[TAT];
	}
	if (NumberofJobs[RT] > 0){
		SumMetrics[RT] = SumMetrics[RT]/ (Average) NumberofJobs[RT];
	}
	SumMetrics[CBT] = SumMetrics[CBT]/ Now();

	if (NumberofJobs[WT] > 0){
		SumMetrics[WT] = SumMetrics[WT]/ (Average) NumberofJobs[WT];
	}

	printf("\n********* Processes Managemenent Numbers ******************************\n");
	printf("Policy Number = %d, Quantum = %.6f   Show = %d\n", PolicyNumber, Quantum, Show);
	printf("Number of Completed Processes = %d\n", NumberofJobs[THGT]);
	printf("ATAT=%f   ART=%f  CBT = %f  T=%f AWT=%f JQ=%f\n ", 
			SumMetrics[TAT], SumMetrics[RT], SumMetrics[CBT], 
			NumberofJobs[THGT]/Now(), SumMetrics[WT], SumMetrics[JQ]);

	exit(0);
}

/***********************************************************************\
 * Input : None                                                          *
 * Output: None                                                          *
 * Function: Decides which processes should be admitted in Ready Queue   *
 *           If enough memory and within multiprogramming limit,         *
 *           then move Process from Job Queue to Ready Queue             *
 \***********************************************************************/
void LongtermScheduler(void){
	ProcessControlBlock *currentProcess = DequeueProcess(JOBQUEUE);
	while (currentProcess) {
		if(lockMemory(currentProcess) == true) {	
			currentProcess->TimeInJobQueue = Now() - currentProcess->JobArrivalTime; // Set TimeInJobQueue
			currentProcess->JobStartTime = Now(); // Set JobStartTime
			EnqueueProcess(READYQUEUE,currentProcess); // Place process in Ready Queue
			currentProcess->state = READY; // Update process state
			NumberofJobs[JQ]++;
			SumMetrics[JQ] += currentProcess->TimeInJobQueue;
			currentProcess = DequeueProcess(JOBQUEUE);
		} else 
		{
			EnqueueProcess(JOBQUEUE, currentProcess);
			return;
		}
	}

}


/***********************************************************************\
 * Input : None                                                          *
 * Output: TRUE if Intialization successful                              *
 \***********************************************************************/
Flag ManagementInitialization(void){
	Metric m;
	for (m = TAT; m < MAXMETRICS; m++){
		NumberofJobs[m] = 0;
		SumMetrics[m]   = 0.0;
	}
	return TRUE;
}
bool lockMemory(ProcessControlBlock *p){
	if(MemoryPolicy == OMAP) {
		if (AvailableMemory >= p->MemoryRequested) {
			AvailableMemory -= p->MemoryRequested;
		} else {
			EnqueueProcess(JOBQUEUE, p);
			printf("Not Enough Memory for process<<<<<<\n");
			return false;
		}
	} else if(MemoryPolicy == PAGING) {
		int pagesNeeded = (p->MemoryRequested / PAGESIZE);
		if (p->MemoryRequested % PAGESIZE > 0) {
			pagesNeeded++;
		}
		if(pagesNeeded <= pagesAvailable){ //if there are enough pages left
			pagesAvailable -= pagesNeeded;
			nonavailablePages += pagesNeeded;
		}
		else{   //not enough pages left
			printf(">>>>>Number of requested pages exceeds the amount of available pages<<<<<<\n");
			EnqueueProcess(JOBQUEUE, p);
			return false;
		}
	} else if (MemoryPolicy == BESTFIT) {
		if(bestFitFind(&FreeHead, &FreeTail, p) == false){
			printf("Not enough contigous memory\n");
			return false;
		}			
	} else if (MemoryPolicy == WORSTFIT) {
		if(worstFitFind(&FreeHead, &FreeTail, p) == false) {
			printf("Not enough contigous memory\n");
			return false;

		}	
	}else {
		//Memory is infinite
	}
	return true;
}

bool freeMemory(FreeMemoryHole** Head, FreeMemoryHole** Tail, ProcessControlBlock *p){
	if(MemoryPolicy == OMAP) {
		AvailableMemory += p->MemoryRequested;

	}
	else if (MemoryPolicy == PAGING) {
		int pagesUsed = (p->MemoryRequested / PAGESIZE);
		if (p->MemoryRequested % PAGESIZE > 0) {
			pagesUsed++;
		}
		pagesAvailable += pagesUsed;
		nonavailablePages -= pagesUsed;

	} else if (MemoryPolicy == BESTFIT || MemoryPolicy == WORSTFIT) {
		if(*Tail != NULL) {
			FreeMemoryHole* new = malloc(sizeof(FreeMemoryHole));
			new->Size = p->MemoryAllocated;
			new->Address=p->ProcessID;
			new->previous = *Tail;
			new->next = NULL;
			(*Tail)->next = new;
			*Tail = new;

		} else{
			FreeMemoryHole* new = malloc(sizeof(FreeMemoryHole));
			new->Size = p->MemoryAllocated;
			new->Address=p->ProcessID;
			new->previous = NULL;
			new->next = NULL;
			*Head = new;
			*Tail = new;

		}	
	}
	return true;
}

bool bestFitFind(FreeMemoryHole** Head, FreeMemoryHole** Tail, ProcessControlBlock* p) {
	if(AvailableMemory >= p->MemoryRequested) {
		AvailableMemory -= p->MemoryRequested;
		p->MemoryAllocated = p->MemoryRequested;
		return true;
	} else if (Head != NULL){
		FreeMemoryHole* test = *Head;
		FreeMemoryHole* bestFit = NULL;
		Memory bestDif = 1000000000;
		int i = 0;
		while(test != NULL) {
			i += 1;
			Memory memDif = (test->Size) - (p->MemoryRequested);
			if(test->Size == p->MemoryRequested) {
				bestFit = test;
			} else if (memDif <= bestDif && test->Size && memDif > 0) {
				bestDif= memDif;
				bestFit = test;
			}
			test = test->next;
		}
		if(bestFit != NULL) {
			if(bestFit->previous != NULL && bestFit->next != NULL) {
				bestFit->previous->next = bestFit->next;
				bestFit->next->previous = bestFit->previous;
			} else if (bestFit->previous != NULL) {
				bestFit->previous->next = NULL;
				*Tail = bestFit->previous;
			} else if (bestFit->next != NULL) {
				bestFit->next->previous = NULL;
			}
			p->MemoryAllocated = bestFit->Size;
			free(bestFit);
			return true;
		}
		test = *Head;
		Memory memSum = 0;
		while(test != NULL && memSum < p->MemoryRequested) {
			memSum += test->Size;
			test = test->next;
		}
		if(test != NULL) {
			p->MemoryAllocated = memSum;
			if(test->previous != NULL) {
				test->previous = NULL;
			}
			(*Head) = test;
			return true;
		}
	}
	return false;
}
bool worstFitFind(FreeMemoryHole** Head, FreeMemoryHole** Tail, ProcessControlBlock* p) {
	if(AvailableMemory >= p->MemoryRequested) {
		p->MemoryAllocated = AvailableMemory - p->MemoryRequested;
		AvailableMemory = p->MemoryRequested;
		return true;
	} else if (Head != NULL){
		FreeMemoryHole* test = *Head;
		FreeMemoryHole* bestFit = NULL;
		Memory bestDif = 0;
		int i = 0;
		while(test != NULL) {
			i += 1;
			Memory memDif = (test->Size) - (p->MemoryRequested);
			if (memDif >= bestDif) {
				bestDif= memDif;
				bestFit = test;
			}
			test = test->next;
		}
		if(bestFit != NULL) {
			if(bestFit->previous != NULL && bestFit->next != NULL) {
				bestFit->previous->next = bestFit->next;
				bestFit->next->previous = bestFit->previous;
			} else if (bestFit->previous != NULL) {
				bestFit->previous->next = NULL;
				*Tail = bestFit->previous;
			} else if (bestFit->next != NULL) {
				bestFit->next->previous = NULL;
			}
			p->MemoryAllocated = bestFit->Size;
			free(bestFit);
			return true;
		}
		test = *Head;
		Memory memSum = 0;
		while(test != NULL && memSum < p->MemoryRequested) {
			memSum += test->Size;
			test = test->next;
		}
		if(test != NULL) {
			p->MemoryAllocated = memSum;
			if(test->previous != NULL) {
				test->previous = NULL;
			}
			(*Head) = test;
			return true;
		}
	}
	return false;
}
