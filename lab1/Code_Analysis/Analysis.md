#**Code Analysis**

The code analysis must be split up into two sections. One section with an overview of the code already provided and a section discussing the code that was written as part of the experiment 

##**Pre-provided Code**

For the project, we were provided three files processesgenerator.o, common.h, and  processesmanagement.c. 
The only file we were responsible for editing was processesmanagement.c.The  processesgenerator.o works as a *"Black Box"* that generates processes and puts them into a queue to be ran. The common.h file contains a number of global data types, global variables, and the PCB block definition. 
The PCB block definition contains all of the information needed to control the scheduling of processes, as well as to gather the metrics at the end of running. 
The processesmanagement.c file contains functions used to maintain the simulation of a CPU and process scheduling.
Some of these functions were pre written and not to be changed, these processes were:
- EnqueueProcess
- DequeueProcess
- OnCPU
- Initialization
- DisplayProcess
- DisplayQueue

The other functions are described in the next section
###**Important variables**
1. NumberofJobs : an array that counts the number of jobs for things like CPU Busy Time (CBT) Return Time (RT) Turnaround Time (TAT) Wait Time (WT) and Throughput (THGT)
2. Sum metrics, and array that contains sums of all the above metrics
##**Code Written for the Expiriment**

In this lab we were responsible for writing several functions which will be described here in order of relative importance.

###**Dispatcher**
The dispatcher function controls how functions are actually run and records the data that will be compared at the end of the experiment.
It works in the following way:
1. Get the first process in the running queue.
2. If that process has a TimeInCPU of 0, then this is first time it has been run
	1. Increment NumberofJobs at CBT
	2. Increment NumberofJobs at RT
	3. Add the current System time - the processes arrival time to SumMetrics at RT
3. If that process has a TimeInCPU of Greater than or equal to that processes's TotalJobDurataion then it is done running
	1. Move the process to the exit queue
	2. Set its state to done
	3. update all the relevant metrics
4. Else there is still more computing to do.
	1. Set the Burst time of the currents process to Whatever it needs to be
		1. If the scheduling policy is Round Robin then the burst time is the Quanta
	2. Call OnCPU with the process and its burst time. 
	3. Update the processes remaining time and relevant metrics
	4. put the process back into the running queue	

###**Bookeeping**
All that was required to implement this function was the math to find the averages of each metric

##**Scheduling Policies**
This experiment required us to implement three scheduling policies, First Come First Served (FCFS), Shortest Job First (SJF), and Round Robing (RR)
Each on of these was implemented as a function inside of processesmanagement.c.

###**First Come First Served**
This policy is the simplest to implement, Dequeue the first process in the READYQUEUE and return it to the dispatcher.
It is a non preemptive policy and allows every job to run its full Burst Length in the dispatcher.

###**Shortest Job First**
This policy seeks to minimize waiting time by prioritizing the shortest jobs first. To do this, we Dequeue the first process in the READYQUEUE, and note its PID.
 We make a new pointer to this process and then put it back into the READYQUEUE. Then we Dequeue the next process. If there are two processes, we compare their Remaining Time. Whichever one is shorter becomes our main process and the longer is
put back into the READYQUEUE. We repeat this until our second process has the same PID as our original process. The we return the minimum process found by comparison to the dispatcher 

###**Round Robin**
This policy is mostly handled by the dispatcher as it controls the Burst Time each process is allowed, so all we do to implement it is Dequeue whoever is next in the READYQUEUE
The dispatcher sets any processes selected by this policy to have a burst time equal to the give quanta. 


