/**
 * @file posix_mq.c
 * @author Armando Pinales
 * @author Shreyan Prabhu Dhananjayan
 * @brief This code implements the heap message queue feature in Linux while also
 * using pthreads. This code builds off of Sam Siewert's 'heap_mq.c' which is
 * originally VxWorks code. 
 * @version 0.1
 * @date 2022-03-01
 * 
 * @copyright Copyright  Sam Siewert(c) 2022
 * 
 */

/****************************************************************************/
/*                                                                          */
/* Sam Siewert - 10/14/97                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
                                                                    
#include <mqueue.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define SNDRCV_MQ "/send_receive_mq"
#define NUM_THREADS (2)
#define MY_SCHEDULER SCHED_FIFO

struct mq_attr mq_attr;
static mqd_t mymq;

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


/* receives pointer to heap, reads it, and deallocate heap memory */

void *receiver(void* args)
{
  char buffer[sizeof(void *)+sizeof(int)];
  void *buffptr; 
  int prio;
  int nbytes;
  int count = 0;
  int id;
 
  while(1) {

    /* read oldest, highest priority msg from the message queue */

    printf("Reading %ld bytes\n", sizeof(void *));
  
    if((nbytes = mq_receive(mymq, buffer, (size_t)(sizeof(void *)+sizeof(int)), &prio)) == -1)
    {
      perror("mq_receive");
    }
    else
    {
      memcpy(&buffptr, buffer, sizeof(void *));
      memcpy((void *)&id, &(buffer[sizeof(void *)]), sizeof(int));
      printf("receive: ptr msg 0x%X received with priority = %d, length = %d, id = %d\n", (unsigned int)buffptr, prio, nbytes, id);

      printf("contents of ptr = \n%s\n", (char *)buffptr);

      free(buffptr);

      printf("heap space memory freed\n");

    }
    
  }

}


static char imagebuff[4096];

void *sender(void* args)
{
  char buffer[sizeof(void *)+sizeof(int)];
  void *buffptr;
  int prio;
  int nbytes;
  int id = 999;


  while(1) {

    /* send malloc'd message with priority=30 */

    buffptr = (void *)malloc(sizeof(imagebuff));
    strcpy(buffptr, imagebuff);
    printf("Message to send = %s\n", (char *)buffptr);

    printf("Sending %ld bytes\n", sizeof(buffptr));

    memcpy(buffer, &buffptr, sizeof(void *));
    memcpy(&(buffer[sizeof(void *)]), (void *)&id, sizeof(int));

    if((nbytes = mq_send(mymq, buffer, (size_t)(sizeof(void *)+sizeof(int)), 30)) == -1)
    {
      perror("mq_send");
    }
    else
    {
      printf("send: message ptr 0x%X successfully sent\n", (unsigned int)buffptr);
    }

    sleep(3);

  }
  
}


static int sid, rid;

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

void heap_mq(void)
{
  int i, j;
  int rc;
  char pixel = 'A';

  for(i=0;i<4096;i+=64) {
    pixel = 'A';
    for(j=i;j<i+64;j++) {
      imagebuff[j] = (char)pixel++;
    }
    imagebuff[j-1] = '\n';
  }
  imagebuff[4095] = '\0';
  imagebuff[63] = '\0';

  printf("buffer =\n%s", imagebuff);

  /* setup common message q attributes */
  mq_attr.mq_maxmsg = 100;
  mq_attr.mq_msgsize = sizeof(void *)+sizeof(int);
  mq_attr.mq_flags = 0;

  /* note that VxWorks does not deal with permissions? */
  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, 0, &mq_attr);

  if(mymq == (mqd_t) -1)
    perror("mq_open");

  printf("mymq: %d\r\n", mymq);

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

      printf("CRETING THREADs\r\n");
      if(i == 0)
      {
        printf("CRETING RECEIVER THREAD\r\n");
        // create threads
        pthread_create(&threads[i],               // pointer to thread descriptor
                      &rt_sched_attr[i],         // use AFFINITY AND SCHEDULER attributes
                      receiver,              // thread function entry point
                      NULL // parameters to pass in
                      );
      }
      else
      {
        printf("CRETING SENDER THREAD\r\n");
        // create threads
        pthread_create(&threads[i],               // pointer to thread descriptor
                      &rt_sched_attr[i],         // use AFFINITY AND SCHEDULER attributes
                      sender,              // thread function entry point
                      NULL // parameters to pass in
                      );
      }
   }
   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);

  mq_close(mymq);
}


void main()
{
  heap_mq();
}