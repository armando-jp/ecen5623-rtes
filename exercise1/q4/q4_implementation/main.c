// 
// 
// Task Specs:
//      Task_1: T1 = 20 msec; C1 = 10 msec; U1 = 0.5
//      Task_2: T2 = 50 msec; C2 = 20 msec; U2 = 0.4
//      Utot = 0.9
// Scheduler Specs:
//      Scheduler: Tsched = 1 msec
//
// Sequencer - 1000 Hz 
//                   [gives semaphores to all other services]
// Service_1 - 50 Hz , every 20th Sequencer loop
//                   [calculates fib10]
// Service_2 - 20 Hz , every 50th Sequencer loop 
//                   [calculates fib20]
//
// With the above, priorities by RM policy would be:
//
// Sequencer = RT_MAX	@ 100 Hz
// Servcie_1 = RT_MAX-1	@ 50 Hz
// Service_2 = RT_MAX-2	@ 20 Hz
//


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <semaphore.h>

#include <syslog.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <errno.h>

#define USEC_PER_MSEC (1000)
#define NANOSEC_PER_SEC (1000000000)
#define NUM_CPU_CORES (1)
#define NUM_THREADS (2+1)
#define TRUE (1)
#define FALSE (0)

// variables to termiante running tasks
int abortTest=FALSE;
int abortS1=FALSE;
int abortS2=FALSE;

// semaphores for tasks
sem_t semS1;
sem_t semS2;

// variable to hold start application start time
struct timeval start_time_val;

// thread parameter struct
// contains thread ID and sequencer period (only used by scheduler).
typedef struct
{
    int threadIdx;
    unsigned long long sequencePeriods;
} threadParams_t;

// function declerations
void *Scheduler(void *threadp);
void *Fib20(void *threadp);
void *Fib10(void *threadp);
void print_scheduler(void);

void main(void)
{
    struct timeval current_time_val;
    int i, rc, scope;
    cpu_set_t threadcpu;
    pthread_t threads[NUM_THREADS];
    threadParams_t threadParams[NUM_THREADS];
    pthread_attr_t rt_sched_attr[NUM_THREADS];
    int rt_max_prio, rt_min_prio;
    struct sched_param rt_param[NUM_THREADS];
    struct sched_param main_param;
    pthread_attr_t main_attr;
    pid_t mainpid;
    cpu_set_t allcpuset;

    printf("Starting LCM Invariant Scheduler for Linux\n");

    // Save elapsed time to start and get current elapse time!
    gettimeofday(&start_time_val, (struct timezone *)0);
    gettimeofday(&current_time_val, (struct timezone *)0);
    syslog(LOG_CRIT, "Sequencer @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    // Display the number of processors we have available to us.
    // Worth mentioning: we will only be using 1 for this application.
    printf("System has %d processors configured and %d available.\n", get_nprocs_conf(), get_nprocs());
    
    // Clear the CPU set. We will only be using however many are specified by
    // the NUM_CPU_CORES define.
    CPU_ZERO(&allcpuset);
    for(i=0; i < NUM_CPU_CORES; i++)
    CPU_SET(i, &allcpuset);
    printf("Using CPUS=%d from total available.\n", CPU_COUNT(&allcpuset));



    // We now initialize the semaphores that will be used by the scheduler.
    // For this demo applciation, we only have 2 semaphores (1 for each task).
    if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
    if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
    
    // Record the main application's process ID
    mainpid=getpid();
    
    // Record the min and max priorities allowed with SCHED_FIFO
    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    // Set the main process priority to the max value allowed by SCHED_FIFO.
    // Set scheduler for main process to SCHED_FIFO
    rc=sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    if(rc < 0) perror("main_param");
    print_scheduler();

    // Get the contention scope for the threads of this application.
    pthread_attr_getscope(&main_attr, &scope);

    if(scope == PTHREAD_SCOPE_SYSTEM)
        printf("PTHREAD SCOPE SYSTEM\n");
    else if (scope == PTHREAD_SCOPE_PROCESS)
        printf("PTHREAD SCOPE PROCESS\n");
    else
        printf("PTHREAD SCOPE UNKNOWN\n");

    // Print the min and max priorities for SCHED_FIFO
    printf("rt_max_prio=%d\n", rt_max_prio);
    printf("rt_min_prio=%d\n", rt_min_prio);




    // For every thread (3 in this case) 
    // Set the thread parameters: SCHED_FIFO and priority.
    // Ultimately, this priority does not matter because we will individually
    // set priorities further down in the code.
    //
    // It is important that the scheduler thread be given the highest 
    // priority (in this case, from entry 0 of rt_param).
    for(i=0; i < NUM_THREADS; i++)
    {

        CPU_ZERO(&threadcpu);
        CPU_SET(3, &threadcpu); // we will run threads on CPU 3

        rc=pthread_attr_init(&rt_sched_attr[i]);
        rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
        rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
        //rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

        rt_param[i].sched_priority=rt_max_prio-i;
        pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

        threadParams[i].threadIdx=i;
    }
    printf("Service threads will run on %d CPU cores\n", CPU_COUNT(&threadcpu));





    // We now create service threads which will block

    // Service_1 = RT_MAX-1 @ 50 Hz
    //
    rt_param[1].sched_priority=rt_max_prio-1;
    pthread_attr_setschedparam(&rt_sched_attr[1], &rt_param[1]);
    rc=pthread_create(&threads[1],               // pointer to thread descriptor
                      &rt_sched_attr[1],         // use specific attributes
                      //(void *)0,               // default attributes
                      Fib10,                     // thread function entry point
                      (void *)&(threadParams[1]) // parameters to pass in
                     );
    if(rc < 0)
        perror("pthread_create for service 1");
    else
        printf("pthread_create successful for service 1\n");

    // Service_2 = RT_MAX-2	@ 20 Hz
    //
    rt_param[2].sched_priority=rt_max_prio-2;
    pthread_attr_setschedparam(&rt_sched_attr[2], &rt_param[2]);
    rc=pthread_create(&threads[2],               // pointer to thread descriptor
                      &rt_sched_attr[2],         // use specific attributes
                      //(void *)0,               // default attributes
                      Fib20,                     // thread function entry point
                      (void *)&(threadParams[2]) // parameters to pass in
                     );
    if(rc < 0)
        perror("pthread_create for service 2");
    else
        printf("pthread_create successful for service 2\n");


    // Create Sequencer thread, which is the highest priority.
    printf("Start sequencer\n");
    threadParams[0].sequencePeriods=900;

    // Sequencer = RT_MAX	@ 1000 Hz
    //
    rt_param[0].sched_priority=rt_max_prio;
    pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);
    rc=pthread_create(&threads[0],               // pointer to thread descriptor
                      &rt_sched_attr[0],         // use specific attributes
                      //(void *)0,               // default attributes
                      Scheduler,                 // thread function entry point
                      (void *)&(threadParams[0]) // parameters to pass in
                     );
    if(rc < 0)
        perror("pthread_create for scheduler service 0");
    else
        printf("pthread_create successful for scheduler service 0\n");



    // Wait for application to complete
    for(i=0;i<NUM_THREADS;i++)
        pthread_join(threads[i], NULL);

   printf("\nTEST COMPLETE\n");
}

void *Scheduler(void *threadp)
{
    struct timespec delay_time = {0,1000000}; // delay for 1 msec, 1000 Hz

    struct timeval current_time_val;
    struct timespec remaining_time;
    double current_time;
    double residual;
    int rc, delay_cnt=0;
    unsigned long long seqCnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    // Display elapsed time
    gettimeofday(&current_time_val, (struct timezone *)0);
    syslog(LOG_CRIT, "Scheduler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    printf("Scheduler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    // Enter main Scheduler loop
    // This loop is termianted when either abortTest is set to TRUE and 
    // the seqCnt reaches the specified limit (for now it is 900)
    do
    {
        delay_cnt=0; residual=0.0;

        // This secondary while loop is for sleeping the specified period time
        // (1 msec). 
        do
        {
            rc=nanosleep(&delay_time, &remaining_time);

            // Check if we were interrupted by a signal.
            // Increment looping delay.
            if(rc == EINTR)
            { 
                residual = remaining_time.tv_sec + ((double)remaining_time.tv_nsec / (double)NANOSEC_PER_SEC);

                if(residual > 0.0) printf("residual=%lf, sec=%d, nsec=%d\n", residual, (int)remaining_time.tv_sec, (int)remaining_time.tv_nsec);
 
                delay_cnt++;
            }
            else if(rc < 0)
            {
                perror("Sequencer nanosleep");
                exit(-1);
            }
           
        } while((residual > 0.0) && (delay_cnt < 100));

        // We get to this point when a sequencer period has completed.
        // We therefore increment seqQnt upon completion and log the time at
        // which it occured.
        seqCnt++;
        gettimeofday(&current_time_val, (struct timezone *)0);
        // syslog(LOG_CRIT, "Sequencer cycle %llu @ sec=%d, msec=%d\n", seqCnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
        // Also log the looping delay if any.s
        if(delay_cnt > 1) printf("Sequencer looping delay %d\n", delay_cnt);





        // Release each service at a sub-rate of the generic sequencer rate
        //

        // Servcie_1 = RT_MAX-1	@ 50 Hz
        if((seqCnt % 20) == 0) sem_post(&semS1);

        // Service_2 = RT_MAX-2	@ 20 Hz
        if((seqCnt % 50) == 0) sem_post(&semS2);

        //gettimeofday(&current_time_val, (struct timezone *)0);
        //syslog(LOG_CRIT, "Sequencer release all sub-services @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    } while(!abortTest && (seqCnt < threadParams->sequencePeriods));

    // Release all the semaphores so the tasks can exit.
    sem_post(&semS1); 
    sem_post(&semS2);
    abortS1=TRUE; 
    abortS2=TRUE; 

    pthread_exit((void *)0);
}

// Print's the current processes' scheduler type
void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
       case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
       case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n"); exit(-1);
         break;
       case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n"); exit(-1);
           break;
       default:
           printf("Pthread Policy is UNKNOWN\n"); exit(-1);
   }
}

// 10 MSEC LONG
void *Fib10(void *threadp)
{
    // THREAD PARAMS
    struct timeval current_time_val;
    double current_time;
    unsigned long long S1Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    // FIB PARAMS
    unsigned int idx = 0, jdx = 1;
    unsigned int seqCnt = 40;
    unsigned int iterCnt = 16500;
    volatile unsigned int fib = 0, fib0 = 0, fib1 = 1;

    gettimeofday(&current_time_val, (struct timezone *)0);
    syslog(LOG_CRIT, "FIB10 START @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    printf("FIB10 START @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    while(!abortS1)
    {
        sem_wait(&semS1);
        ++S1Cnt;
        gettimeofday(&current_time_val, (struct timezone *)0);
        syslog(LOG_CRIT, "FIB10 GET @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
        printf("FIB10 GET @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
        // Computation START
        for(idx=0; idx < iterCnt; idx++)
        {
            fib = fib0 + fib1;
            while(jdx < seqCnt)
            { 
                fib0 = fib1;
                fib1 = fib;
                fib = fib0 + fib1;
                jdx++;
            }

            jdx=0;
        }
        // Computation END

        // Display time at which computation completed
        gettimeofday(&current_time_val, (struct timezone *)0);
        syslog(LOG_CRIT, "FIB10 RELEASE %llu @ sec=%d, msec=%d\n", S1Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
        printf("FIB10 RELEASE %llu @ sec=%d, msec=%d\n", S1Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    }

    pthread_exit((void *)0);
}

// 20 MSEC LONG
void *Fib20(void *threadp)
{
    // THREAD PARAMS
    struct timeval current_time_val;
    double current_time;
    unsigned long long S2Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    // FIB PARAMS
    unsigned int idx = 0, jdx = 1;
    unsigned int seqCnt = 47;
    unsigned int iterCnt = 30000;
    volatile unsigned int fib = 0, fib0 = 0, fib1 = 1;

    gettimeofday(&current_time_val, (struct timezone *)0);
    syslog(LOG_CRIT, "FIB20 START @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    printf("FIB20 START @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    while(!abortS2)
    {
        sem_wait(&semS2);
        ++S2Cnt;
        gettimeofday(&current_time_val, (struct timezone *)0);
        syslog(LOG_CRIT, "FIB20 GET @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
        printf("FIB20 GET @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
        // Computation START
        for(idx=0; idx < iterCnt; idx++)
        {
            fib = fib0 + fib1;
            while(jdx < seqCnt)
            { 
                fib0 = fib1;
                fib1 = fib;
                fib = fib0 + fib1;
                jdx++;
            }

            jdx=0;
        }
        // Computation END

        gettimeofday(&current_time_val, (struct timezone *)0);
        syslog(LOG_CRIT, "FIB20 RELEASE %llu @ sec=%d, msec=%d\n", S2Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
        printf("FIB20 RELEASE %llu @ sec=%d, msec=%d\n", S2Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    }

    pthread_exit((void *)0);
}

