/**
 * @file posix_mq.c
 * @author Armando Pinales
 * @author Shreyan Prabhu Dhananjayan
 * @brief This code implements the message queue feature in Linux while also
 * using pthreads. This code builds off of Sam Siewert's 'posix_mq.c' which is a
 * ported version of the VxWorks code. 
 * @version 0.1
 * @date 2022-02-24
 * 
 * @copyright Copyright  Sam Siewert(c) 2022
 * 
 */

/****************************************************************************/
/* Function: Basic POSIX message queue demo                                 */
/*                                                                          */
/* Sam Siewert, 02/05/2011                                                  */
/****************************************************************************/

//   Mounting the message queue file system
//       On  Linux,  message queues are created in a virtual file system.
//       (Other implementations may also provide such a feature, but  the
//       details  are likely to differ.)  This file system can be mounted
//       (by the superuser) using the following commands:

//           # mkdir /dev/mqueue
//           # mount -t mqueue none /dev/mqueue


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#define SNDRCV_MQ "/send_receive_mq"
#define MAX_MSG_SIZE 128
#define ERROR (-1)

#define NUM_THREADS (2)
#define MY_SCHEDULER SCHED_FIFO
// #define MY_SCHEDULER SCHED_RR
// #define MY_SCHEDULER SCHED_OTHER

struct mq_attr mq_attr;

// POSIX thread declarations and scheduling attributes
typedef struct
{
    int threadIdx;
} threadParams_t;

pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param main_param;
pthread_attr_t main_attr;
pid_t mainpid;

void *receiver(void *threadp)
{
  mqd_t mymq;
  char buffer[MAX_MSG_SIZE];
  int prio;
  int nbytes;

  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, S_IRWXU, &mq_attr);

  if(mymq == (mqd_t)ERROR)
  {
    perror("receiver mq_open");
    exit(-1);
  }

  /* read oldest, highest priority msg from the message queue */
  if((nbytes = mq_receive(mymq, buffer, MAX_MSG_SIZE, &prio)) == ERROR)
  {
    perror("mq_receive");
  }
  else
  {
    buffer[nbytes] = '\0';
    printf("receive: msg %s received with priority = %d, length = %d\n",
           buffer, prio, nbytes);
  }
    
}

static char canned_msg[] = "this is a test, and only a test, in the event of a real emergency, you would be instructed ...";

void *sender(void *threadp)
{
  mqd_t mymq;
  int prio;
  int nbytes;

  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, S_IRWXU, &mq_attr);

  if(mymq < 0)
  {
    perror("sender mq_open");
    exit(-1);
  }
  else
  {
    printf("sender opened mq\n");
  }

  /* send message with priority=30 */
  if((nbytes = mq_send(mymq, canned_msg, sizeof(canned_msg), 30)) == ERROR)
  {
    perror("mq_send");
  }
  else
  {
    printf("send: message successfully sent\n");
  }
  
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

void main(void)
{
  int rc;
  int i;

  /* setup common message q attributes */
  mq_attr.mq_maxmsg = 10;
  mq_attr.mq_msgsize = MAX_MSG_SIZE;
  mq_attr.mq_flags = 0;


  // Create two communicating processes right here
  rt_max_prio = sched_get_priority_max(SCHED_FIFO);
  rt_min_prio = sched_get_priority_min(SCHED_FIFO);

  print_scheduler();
  rc = sched_getparam(mainpid, &main_param);
  main_param.sched_priority=rt_max_prio;

  if(MY_SCHEDULER != SCHED_OTHER)
  {
    if(rc=sched_setscheduler(getpid(), MY_SCHEDULER, &main_param) < 0)
      perror("******** WARNING: sched_setscheduler");
  }
  print_scheduler();
  printf("rt_max_prio=%d\n", rt_max_prio);
  printf("rt_min_prio=%d\n", rt_min_prio);

   for(i=0; i < NUM_THREADS; i++)
   {
      //  CPU_ZERO(&threadcpu);

      //  coreid=i%numberOfProcessors;
      //  printf("Setting thread %d to core %d\n", i, coreid);

      //  // assign thread to specific CPU
      //  CPU_SET(coreid, &threadcpu);

       // set new thread attributes
       rc=pthread_attr_init(&rt_sched_attr[i]);
       rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
       rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], MY_SCHEDULER);
      //  rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

       // set schedule priority 
       rt_param[i].sched_priority=rt_max_prio-i-1;
       // set parameters
       pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

       threadParams[i].threadIdx=i;

      if(i == 0)
      {
        // create threads
        pthread_create(&threads[i],               // pointer to thread descriptor
                      &rt_sched_attr[i],         // use AFFINITY AND SCHEDULER attributes
                      sender,              // thread function entry point
                      (void *)&(threadParams[i]) // parameters to pass in
                      );
      }
      else
      {
        // create threads
        pthread_create(&threads[i],               // pointer to thread descriptor
                      &rt_sched_attr[i],         // use AFFINITY AND SCHEDULER attributes
                      receiver,              // thread function entry point
                      (void *)&(threadParams[i]) // parameters to pass in
                      );
      }


   }


   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);

   printf("\nTEST COMPLETE\n");
   
}
