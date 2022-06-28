#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <time.h>
#define MEM 4096
ucontext_t Main;
int remainingThreads = 5;
typedef struct ThreadInfo{
    ucontext_t context;
    int state; // 0 for empty 1 for ready 2 for running 3 for I/O 4 for finished.
    int CPUcounter;	// It increases +2 every counter value.
    int IOcounter;	// It increases +2 every i/o operation.
    int CPUbursttime;	// CPU Burst time (changes every step)
    int IObursttime;
    int statechange; // odd - > from CPU to I/O even - > from I/O to CPU
    int index;
}ThreadInfo;
struct ThreadInfo **Threads;
struct ThreadInfo **ThreadsArray;
int size = 6;		// The size of the ThreadArray
int cpuBurstTimes0[5],ioBurstTimes0[5];	// Holds the CPU burst && IO Time for step 1. (cpu1,io1) times
int cpuBurstTimes1[5],ioBurstTimes1[5];	// Holds the CPU burst && IO Time for step 2. (cpu2,io2) times
int cpuBurstTimes2[5],ioBurstTimes2[5];	// Holds the CPU burst && IO Time for step 3. (cpu3,io3) times
// The functions
void displayfunc(int i);			// Displays the current execution
void counter(int i);
void initializeThread();
int createThread();
void io();
void runThread(int index);
void exitThread(int index);
void SRTFScheduler();
// end
void displayfunc(int i){
    char m[5*(i+1)];
    for(int j = 0; j < 5*i+4; j++){
        m[j] = ' ';
    }
    m[5*(i) + 4] = '\0';
    printf("%s T%d: %d\n",m,i,ThreadsArray[i]->CPUcounter);
}
void counter(int i){
    // No need to put n here, since I created in the thread as CPUbursttime.
    if(ThreadsArray[i]->state == 2){
        printf("running>T%d ready>",i);
        for(int m = 1; m < size; m++){
            if(ThreadsArray[m]->state == 1){
                printf("T%d ",m);
            }
        }
        printf("finished>");
        for(int m = 1; m < size; m++){
            if(ThreadsArray[m]->state == 4){
                printf("T%d ",m);
            }
        }
        printf(" ");
        io();
        if(ThreadsArray[i]->CPUcounter == ThreadsArray[i]->CPUbursttime - 2){
            // If T[i] ends CPU operation.
            ThreadsArray[i]->CPUcounter = ThreadsArray[i]->CPUcounter + 1;
            displayfunc(i);
            ThreadsArray[i]->CPUcounter = ThreadsArray[i]->CPUcounter + 1;
            displayfunc(i);
            ThreadsArray[i]->state = 3; // I/O state.
            ThreadsArray[i]->IOcounter = 0; // Reset the IO counter
            ThreadsArray[i]->statechange = ThreadsArray[i]->statechange + 1; // CPU to IO.
        }
        else{
            // If T[i] continues to CPU operation.
            ThreadsArray[i]->CPUcounter = ThreadsArray[i]->CPUcounter + 1;
            displayfunc(i);
            ThreadsArray[i]->CPUcounter = ThreadsArray[i]->CPUcounter + 1;
            displayfunc(i);
            ThreadsArray[i]->state = 1; // Ready state.
        }
    }
}
void initializeThread(){
    Threads = malloc(sizeof(ThreadInfo*) * size);
    ThreadsArray = malloc(sizeof(ThreadInfo*) * size);
    for (int i = 0; i < size; i++){
        Threads[i] = malloc(sizeof(ThreadInfo));
        Threads[i]->state = 0; // 0 means empty --> Put them to array in the createThread() function
        ThreadsArray[i] = malloc(sizeof(ThreadInfo));
        ThreadsArray[i] = NULL; // not assigned.
    }
}
int createThread(){
    // ThreadsArray[0] is the main thread. So no need to adjust its variables.
    for(int i = 1; i < size; i++){
        if(ThreadsArray[i] == NULL){
            for(int j = 1; j < size; j++){
                if(Threads[j]->state == 0){
                    Threads[j]->CPUbursttime = cpuBurstTimes0[i - 1];	// first step cpu burst time
                    Threads[j]->IObursttime = ioBurstTimes0[i - 1];		// first step io burst time
                    Threads[j]->index = i;
                    Threads[j]->state = 1; // READY state
                    ThreadsArray[i] = Threads[j];
                    getcontext(&Threads[j]->context);
                    Threads[j]->context.uc_link = &Threads[j]->context;
                    Threads[j]->context.uc_stack.ss_sp = malloc(MEM);
                    Threads[j]->context.uc_stack.ss_size = MEM;
                    makecontext(&Threads[j]->context, (void*) runThread, 1, j);	// runThread() is their function when they called.
                    return 1;	// ThreadArray[i] is succesfull.
                }
            }
        }
    }
    return -1;			// ThreadArray[i] is not assigned, so it indicates an error.
}
void io(){
    printf("IO>");
    for(int k = 1; k < size ; k++)
    {
        if(ThreadsArray[k]->state == 3){
            //Which meaning that I O operation.
            printf("T%d ",k);
            ThreadsArray[k]->IOcounter = ThreadsArray[k]->IOcounter + 2;
            if(ThreadsArray[k]->IOcounter == ThreadsArray[k]->IObursttime){
                // If this is the last I/O (statechange == 5) --> then process is finished.
                if(ThreadsArray[k]->statechange == 5){
                    ThreadsArray[k]->state = 4; // Finish
                    remainingThreads = remainingThreads - 1;
                    //exitThread(k);
                }
                else{
                    // Thread goes to ready queue, not finished.
                    ThreadsArray[k]->state = 1; // READY
                    ThreadsArray[k]->statechange = ThreadsArray[k]->statechange + 1;
                    ThreadsArray[k]->CPUcounter = 0;
                    // Thread goes to new cpu/io pair. New bursttime and iobursttimes are assigned.
                    if(ThreadsArray[k]->statechange == 2){
                    	ThreadsArray[k]->CPUbursttime = cpuBurstTimes1[k - 1];
                    	ThreadsArray[k]->IObursttime = ioBurstTimes1[k - 1];
                    }
                    else if(ThreadsArray[k]->statechange == 4){
                    	ThreadsArray[k]->CPUbursttime = cpuBurstTimes2[k - 1];
                    	ThreadsArray[k]->IObursttime = ioBurstTimes2[k - 1];
                    }
                }
            }
        }   
    }
    printf("\n");
}
void runThread(int index){
    alarm(2);
    // Below if function just for make sure, the Thread that is not in ready queue can not be in here.
    // If the thread index state is ready then it will running and goes to counter function.
    if(ThreadsArray[index]->state == 1){
        ThreadsArray[index]->state = 2;
        counter(index);
    }
    sleep(2);
    swapcontext(&Threads[index]->context,&Threads[0]->context);
}
void exitThread(int index){
    // Remove the context from mem.
    free(ThreadsArray[index]->context.uc_stack.ss_sp);
    if(remainingThreads == 0){
        for(int i = 1; i < size; i++){
            ThreadsArray[index]->state = 0; // Array is empty now.
        }
    }
}
void SRTFScheduler()
{
    int runIndex = 0;
    int bignumber = 10000;
    for(int i = 1; i < size ; i++){
    	if(ThreadsArray[i]->state == 1){
    	    if((ThreadsArray[i]->CPUbursttime - ThreadsArray[i]->CPUcounter) < bignumber){
    	        bignumber = ThreadsArray[i]->CPUbursttime - ThreadsArray[i]->CPUcounter;
    	        runIndex = i;
    	    }
    	}
    }
    // RunIndex determines the next threadsArray index to be executed in CPU.
    // If RunIndex != 0 --> Goes to runThread and does CPU op.
    // If RunIndex == 0, meaning that there are no ready Threads, there are two cases;
    // 1) Also if remainingThreads == 0 --> All of the process are completely done. (No need to go runThread() func.) Finish.
    // 2) If remainingThreads != 0 --> There are proces(s) that in the I/O state. Do them.
    if(runIndex == 0){
        if(remainingThreads == 0){
            printf("running> ready> finished>T1 T2 T3 T4 T5  IO>");
        }
        else{
            // I/O check. No CPU burst. Alarmı tekrar aktifleştirmek lazım.
            printf("running> ready> finished> ");
            for(int i = 1; i < size; i++){
                if(ThreadsArray[i]->state == 4){
                    printf("T%d ",i);
                }
            }
            io();
            if(remainingThreads == 0){
            	printf("running> ready> finished>T1 T2 T3 T4 T5  IO>");
            }
            else{
            	alarm(2);
            }
            sleep(2);
            printf("\n");
        }
    }
    else{
        swapcontext(&Threads[0]->context,&Threads[runIndex]->context);
    }
}
int main()
{   
	// Thread 0 is the main
	// Taking the inputs.
	printf("Please write the first cpu and io burst times respectively from T1 to T5: ");
	scanf("%d %d %d %d %d %d %d %d %d %d",&cpuBurstTimes0[0],&cpuBurstTimes0[1],&cpuBurstTimes0[2],&cpuBurstTimes0[3],&cpuBurstTimes0[4],&ioBurstTimes0[0],&ioBurstTimes0[1],&ioBurstTimes0[2],&ioBurstTimes0[3],&ioBurstTimes0[4]);
	printf("\nPlease write the second cpu and io burst times respectively from T1 to T5: ");
	scanf("%d %d %d %d %d %d %d %d %d %d",&cpuBurstTimes1[0],&cpuBurstTimes1[1],&cpuBurstTimes1[2],&cpuBurstTimes1[3],&cpuBurstTimes1[4],&ioBurstTimes1[0],&ioBurstTimes1[1],&ioBurstTimes1[2],&ioBurstTimes1[3],&ioBurstTimes1[4]);
	srand(time(NULL));
	printf("\nPlease write the third cpu and io burst times respectively from T1 to T5: ");
	scanf("%d %d %d %d %d %d %d %d %d %d",&cpuBurstTimes2[0],&cpuBurstTimes2[1],&cpuBurstTimes2[2],&cpuBurstTimes2[3],&cpuBurstTimes2[4],&ioBurstTimes2[0],&ioBurstTimes2[1],&ioBurstTimes2[2],&ioBurstTimes2[3],&ioBurstTimes2[4]);
	srand(time(NULL));
	signal(SIGALRM,SRTFScheduler);
	initializeThread();
	for(int i = 1 ; i < 6; i++)
	{
	    int error = createThread();
	}
	getcontext(&Threads[0]->context);
	Threads[0]->context.uc_link = &Threads[0]->context;
    	Threads[0]->context.uc_stack.ss_sp = malloc(MEM);
	Threads[0]->context.uc_stack.ss_size = MEM;
	SRTFScheduler();
	while(1){
	}
	return 0;
}
