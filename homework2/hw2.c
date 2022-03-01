#define _GNU_SOURCE
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

#define SNAME "/mysem"

// max number of threads/cpus this program can run on
#define NUM_THREADS (2)
#define NUM_CPUS (4)

// defines for return codes
#define ERROR (-1)
#define OK (0)

// pick the scheduling algo to use for this program
#define MY_SCHEDULER SCHED_FIFO
//#define MY_SCHEDULER SCHED_RR
//#define MY_SCHEDULER SCHED_OTHER

int numberOfProcessors=NUM_CPUS;

// thread parameter structure
typedef struct
{
    int threadIdx;
    sem_t* sem;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param main_param;
pthread_attr_t main_attr;
pid_t mainpid;

// semaphore decleration
sem_t sem;

void *process(void *threadp)
{
    int i;
    pthread_t thread;
    cpu_set_t cpuset;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    if(threadParams->threadIdx == 0)
    {
        printf("THREAD%d: WAITING FOR SEMAPHORE FROM THREAD1\n", threadParams->threadIdx);
        // wait for semaphore
        sem_wait(threadParams->sem);
        printf("THREAD%d: GOT SEMAPHORE FROM THREAD1\n", threadParams->threadIdx);
        printf("THREAD%d: TERMINATING\n", threadParams->threadIdx); 
    }
    else if (threadParams->threadIdx == 1)
    {
        printf("THREAD%d: GIVING SEMAPHORE TO THREAD0\n", threadParams->threadIdx);
        // wait for semaphore
        sem_post(threadParams->sem);
        printf("THREAD%d: TERMINATING\n", threadParams->threadIdx);    
    }




    pthread_exit(0);
}


void print_scheduler(void)
{
   int schedType, scope;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
     case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n");
           break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }

   pthread_attr_getscope(&main_attr, &scope);

   if(scope == PTHREAD_SCOPE_SYSTEM)
     printf("PTHREAD SCOPE SYSTEM\n");
   else if (scope == PTHREAD_SCOPE_PROCESS)
     printf("PTHREAD SCOPE PROCESS\n");
   else
     printf("PTHREAD SCOPE UNKNOWN\n");

}



int main (int argc, char *argv[])
{
    int rc, idx;
    int i;
    cpu_set_t threadcpu;
    int coreid;

    // get number of processors and print stats
    numberOfProcessors = get_nprocs_conf();
    printf("===PROCESSOR STATS===\n");
    printf("This system has %d processors with %d available\n", numberOfProcessors, get_nprocs());
    printf("The test thread created will be run on a specific core based on thread index\n");

    // get the processes ID
    mainpid=getpid();

    // // get the min and max priorities possible
    // rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    // rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    // //  display the scheduler being used
    // printf("===SCHEDULER STATS===\n");
    // print_scheduler();
    // rc=sched_getparam(mainpid, &main_param);
    // main_param.sched_priority=rt_max_prio;

    // // Attempt to change the scheduler if necessary
    // if(MY_SCHEDULER != SCHED_OTHER)
    // {
    //     if(rc=sched_setscheduler(getpid(), MY_SCHEDULER, &main_param) < 0)
    //         perror("******** WARNING: sched_setscheduler");
    // }

    // // print the new scheduler and the min and max priorities
    // printf("===NEW SCHEDULER STATS===\n");
    print_scheduler();
    // printf("rt_max_prio=%d\n", rt_max_prio);
    // printf("rt_min_prio=%d\n", rt_min_prio);

    // create semaphore
    sem_init(&sem, 0, 0);

    printf("===CREATING THREADS===\n");
    for(i=0; i < NUM_THREADS; i++)
    {
        CPU_ZERO(&threadcpu);

        coreid=i%numberOfProcessors;
        // printf("Setting thread %d to core %d\n", i, coreid);

        // // assign thread to specific CPU
        // CPU_SET(coreid, &threadcpu);

        // // set new thread attributes
        rc=pthread_attr_init(&rt_sched_attr[i]);
        // rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
        // rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], MY_SCHEDULER);
        // rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

        // // set schedule priority 
        // rt_param[i].sched_priority=rt_max_prio-1;

        // // set parameters
        // pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

        threadParams[i].threadIdx=i;
        threadParams[i].sem = &sem;

        // create threads
        printf("---Launching thread%d---\n", i);
        pthread_create(&threads[i],               // pointer to thread descriptor
                        &rt_sched_attr[i],         // use AFFINITY AND SCHEDULER attributes
                        process,                  // thread function entry point
                        (void *)&(threadParams[i]) // parameters to pass in
                        );

        // only perform delay before launching thread1.
        if(i < 1)
        {
            printf("[Press ENTER to launch THREAD1]\n");
            getchar();
            // printf("Sleeping for %d seconds before launching thread%d\n...\n", 5, i+1);
            // sleep(5);
        }


    }


    for(i=0;i<NUM_THREADS;i++)
        pthread_join(threads[i], NULL);

    printf("\nTEST COMPLETE\n");
}
