/**
 * @file deadlock_timeout.c
 * @author Armando Pinales
 * @author Shreyan Prabhu Dhananjayan
 * @brief This code implements the random back-off solution for blocking
 *        conditions. This code is built off of code written by Sam Siewert as
 *        well as heavily referencing Sam Siewert's solution titled
 *        "deadlock_timeout.c". 
 * @version 0.1
 * @date 2022-02-24
 * 
 * @copyright Copyright  Sam Siewert(c) 2022
 * 
 */

#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#define NUM_THREADS 2
#define THREAD_1 1
#define THREAD_2 2

// create a structure to pass args to thread
typedef struct
{
    int thread_id;
} threadParams_t;

// create an array of the arg structures and an array of type pthread to hold
// thread IDs.
threadParams_t thread_args[NUM_THREADS];
pthread_t threads[NUM_THREADS];

// create the mutexes to be used by THREAD1 and THREAD2
pthread_mutex_t rsrcA; 
pthread_mutex_t rsrcB;

volatile int count_rsrcA = 0;
volatile int count_rsrcB = 0; 
volatile int no_wait   = 0;

// function decleration
void *get_resources(void *args);


int main (int argc, char *argv[])
{
  int rv;
  count_rsrcA = 0; 
  count_rsrcB = 0; 
  no_wait     = 0;

  // Set default protocol for mutex which is unlocked to start
  pthread_mutex_init(&rsrcA, NULL);
  pthread_mutex_init(&rsrcB, NULL);

  // CREATE THREAD 1
  printf("Creating thread %d\n", THREAD_1);
  thread_args[THREAD_1].thread_id = THREAD_1;
  rv = pthread_create(
    &threads[0], 
    NULL, 
    get_resources, 
    (void *)&thread_args[THREAD_1]
  );
  if(rv) 
  {
     printf("ERROR; pthread_create() rv is %d\n", rv); 
     perror(NULL); 
     exit(-1);
  }


  // CREATE THREAD 2
  printf("Creating thread %d\n", THREAD_2);
  thread_args[THREAD_2].thread_id=THREAD_2;
  rv = pthread_create(
    &threads[1], 
    NULL, 
    get_resources, 
    (void *)&thread_args[THREAD_2]
  );
  if(rv) 
  {
    printf("ERROR; pthread_create() rv is %d\n", rv); 
    perror(NULL); 
    exit(-1);
  }

  // ATTEMPT TO JOIN THREAD1
  printf("Attempting to join both threads.\n");
  if(pthread_join(threads[0], NULL) == 0)
  {
    printf("Thread 1 joined to main\n");
  }
  else
  {
    perror("Thread 1");
  }

  // ATTEMPT TO JOIN THREADA2
  if(pthread_join(threads[1], NULL) == 0)
  {
    printf("Thread 2 joined to main\n");
  }
  else
  {
    perror("Thread 2");
  }

  // Finally, destroy both mutexes
  if(pthread_mutex_destroy(&rsrcA) != 0)
  {
    perror("mutex A destroy");
  }
  if(pthread_mutex_destroy(&rsrcB) != 0)
  {
    perror("mutex B destroy");
  }
    

  printf("TEST DONE\n");

  return 0;
}


// Function to be run for threads,
void *get_resources(void *args)
{
  // declare necessary variables
  int rv;
  struct timespec current_time;
  struct timespec timeout_rsrcA;
  struct timespec timeout_rsrcB;

  // cast the thread arguments and save the thread_id
  threadParams_t *thread_args = (threadParams_t *)args;
  int thread_id = thread_args->thread_id;

  // Debug prints
  if(thread_id == THREAD_1) 
  {
    printf("Thread 1 started\n");
  }
  else if(thread_id == THREAD_2) 
  {
    printf("Thread 2 started\n");
  }
  else 
  {
    // big error if we end up here
    printf("Unknown thread started\n");
  }

  // get the current clock time
  clock_gettime(CLOCK_REALTIME, &current_time);

  // Create timeouts for rsrcA and rsrcB
  timeout_rsrcA.tv_sec  = current_time.tv_sec + 2;
  timeout_rsrcA.tv_nsec = current_time.tv_nsec;
  timeout_rsrcB.tv_sec  = current_time.tv_sec + 3;
  timeout_rsrcB.tv_nsec = current_time.tv_nsec;


  if(thread_id == THREAD_1)
  {
    // Attempt to grab rsrcA
    printf("THREAD 1 grabbing resource A @ %d sec and %d nsec\n", 
      (int)current_time.tv_sec, 
      (int)current_time.tv_nsec
    );
    if((rv=pthread_mutex_lock(&rsrcA)) != 0)
    {
        printf("Thread 1 ERROR\n");
        pthread_exit(NULL);
    }
    else
    {
        printf("Thread 1 GOT A\n");
        count_rsrcA++;
        printf("count_rsrcA=%d, count_rsrcB=%d\n", count_rsrcA, count_rsrcB);
    }

    // intentionally sleep in order for thread2 to grab rsrcB, to intentionally
    // cause a deadlock condition.
    if(!no_wait) usleep(1000000);

    // get the current time
    clock_gettime(CLOCK_REALTIME, &current_time);
    timeout_rsrcB.tv_sec  = current_time.tv_sec + 3;
    timeout_rsrcB.tv_nsec = current_time.tv_nsec;

    printf("THREAD 1 got A, trying for B @ %d sec and %d nsec\n", 
      (int)current_time.tv_sec, 
      (int)current_time.tv_nsec
    );

    //Attempt to grab rsrcB
    rv = pthread_mutex_timedlock(&rsrcB, &timeout_rsrcB);
    if(rv == 0)
    {
        clock_gettime(CLOCK_REALTIME, &current_time);
        printf("Thread 1 GOT B @ %d sec and %d nsec with rv=%d\n", 
          (int)current_time.tv_sec, 
          (int)current_time.tv_nsec, rv
        );
        count_rsrcB++;
        printf("count_rsrcA=%d, count_rsrcB=%d\n", count_rsrcA, count_rsrcB);
    }
    else if(rv == ETIMEDOUT)
    {
        printf("Thread 1 TIMEOUT ERROR\n");
        count_rsrcA--;
        pthread_mutex_unlock(&rsrcA);
        pthread_exit(NULL);
    }
    else
    {
        printf("Thread 1 ERROR\n");
        count_rsrcA--;
        pthread_mutex_unlock(&rsrcA);
        pthread_exit(NULL);
    }

    printf("THREAD 1 got A and B\n");
    count_rsrcB--;
    pthread_mutex_unlock(&rsrcB);
    count_rsrcA--;
    pthread_mutex_unlock(&rsrcA);
    printf("THREAD 1 done\n");
  }

  else // WE ARE THREAD 2
  {
    printf("THREAD 2 grabbing resource B @ %d sec and %d nsec\n", 
    (int)current_time.tv_sec, 
    (int)current_time.tv_nsec
    );

    // Attempt to get rsrcB
    if((rv=pthread_mutex_lock(&rsrcB)) != 0)
    {
        printf("Thread 2 ERROR\n");
        pthread_exit(NULL);
    }
    else
    {
        printf("Thread 2 GOT B\n");
        count_rsrcB++;
        printf("count_rsrcA=%d, count_rsrcB=%d\n", count_rsrcA, count_rsrcB);
    }

    // intentionally sleep in order for thread1 to grab rsrcA, to intentionally
    // cause a deadlock condition.
    if(!no_wait) usleep(1000000);

    // get the current clock time
    clock_gettime(CLOCK_REALTIME, &current_time);
    timeout_rsrcA.tv_sec = current_time.tv_sec + 2;
    timeout_rsrcA.tv_nsec = current_time.tv_nsec;

    printf("THREAD 2 got B, trying for A @ %d sec and %d nsec\n", 
      (int)current_time.tv_sec, 
      (int)current_time.tv_nsec
    );

    // attempt to grab rsrcA using a TIMED LOCK
    rv=pthread_mutex_timedlock(&rsrcA, &timeout_rsrcA);
    if(rv == 0)
    {
        clock_gettime(CLOCK_REALTIME, &current_time);
        printf("Thread 2 GOT A @ %d sec and %d nsec with rv=%d\n", 
          (int)current_time.tv_sec, 
          (int)current_time.tv_nsec, 
          rv
        );
        count_rsrcA++;
        printf("count_rsrcA=%d, count_rsrcB=%d\n", count_rsrcA, count_rsrcB);
    }
    else if(rv == ETIMEDOUT)
    {
        printf("Thread 2 TIMEOUT ERROR\n");
        count_rsrcB--;
        pthread_mutex_unlock(&rsrcB);
        pthread_exit(NULL);
    }
    else
    {
        printf("Thread 2 ERROR\n");
        count_rsrcB--;
        pthread_mutex_unlock(&rsrcB);
        pthread_exit(NULL);
    }

    printf("THREAD 2 got B and A\n");
    count_rsrcA--;
    pthread_mutex_unlock(&rsrcA);
    count_rsrcB--;
    pthread_mutex_unlock(&rsrcB);
    printf("THREAD 2 done\n");
  }

  // terminate thread(self)
  pthread_exit(NULL);
}