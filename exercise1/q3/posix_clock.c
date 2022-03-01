/****************************************************************************/
/* Function: nanosleep and POSIX 1003.1b RT clock demonstration             */
/*                                                                          */
/* Sam Siewert - 02/05/2011                                                 */
/*                                                                          */
/****************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define NSEC_PER_SEC (1000000000)
#define NSEC_PER_MSEC (1000000)
#define NSEC_PER_USEC (1000)
#define ERROR (-1)
#define OK (0)
#define TEST_SECONDS (0)
#define TEST_NANOSECONDS (NSEC_PER_MSEC * 10) // about 0.01 seconds or 10 msec

void end_delay_test(void);

static struct timespec sleep_time = {0, 0};
static struct timespec sleep_requested = {0, 0};
static struct timespec remaining_time = {0, 0};

static unsigned int sleep_count = 0;

pthread_t main_thread;
pthread_attr_t main_sched_attr; // thread-attributes object
int rt_max_prio, rt_min_prio, min;
struct sched_param main_param; // structure that describes scheduling parameters

// Prints the current running processes scheduling policy to console.
void print_scheduler(void)
{
   int schedType;

   // Obtain the current scheduling policy of the thread identified by pid.
   // getpid() returns the process ID of the calling process. 
   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
     // This is a first-in first-out policy
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
     case SCHED_OTHER:
     // This is the standard round-robin time-sharing policy
           printf("Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:
     // This is a round-robin policy
           printf("Pthread Policy is SCHED_RR\n");
           break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}

// converts delta time to a double.
double d_ftime(struct timespec *fstart, struct timespec *fstop)
{
  double dfstart = ((double)(fstart->tv_sec) + ((double)(fstart->tv_nsec) / 1000000000.0));
  double dfstop = ((double)(fstop->tv_sec) + ((double)(fstop->tv_nsec) / 1000000000.0));

  return(dfstop - dfstart); 
}

// calculates a delta in time structures stop and start. stores result in
// delta_t struct.
int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  // calculate the elapsed time between stop and start times for seconds and
  // nano seconds.
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  //printf("\ndt calcuation\n");

  // case 1 - less than a second of change
  if(dt_sec == 0)
  {
	  //printf("dt less than 1 second\n");

	  if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC)
	  {
	          //printf("nanosec greater at stop than start\n");
		  delta_t->tv_sec = 0;
		  delta_t->tv_nsec = dt_nsec;
	  }

	  else if(dt_nsec > NSEC_PER_SEC)
	  {
	          //printf("nanosec overflow\n");
		  delta_t->tv_sec = 1;
		  delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
	  }

	  else // dt_nsec < 0 means stop is earlier than start
	  {
	         printf("stop is earlier than start\n");
		 return(ERROR);  
	  }
  }

  // case 2 - more than a second of change, check for roll-over
  else if(dt_sec > 0)
  {
	  //printf("dt more than 1 second\n");

	  if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC)
	  {
	          //printf("nanosec greater at stop than start\n");
		  delta_t->tv_sec = dt_sec;
		  delta_t->tv_nsec = dt_nsec;
	  }

	  else if(dt_nsec > NSEC_PER_SEC)
	  {
	          //printf("nanosec overflow\n");
		  delta_t->tv_sec = delta_t->tv_sec + 1;
		  delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
	  }

	  else // dt_nsec < 0 means roll over
	  {
	          //printf("nanosec roll over\n");
		  delta_t->tv_sec = dt_sec-1;
		  delta_t->tv_nsec = NSEC_PER_SEC + dt_nsec;
	  }
  }

  return(OK);
}

static struct timespec rtclk_dt = {0, 0};
static struct timespec rtclk_start_time = {0, 0}; // stores the start time of test
static struct timespec rtclk_stop_time = {0, 0};
static struct timespec delay_error = {0, 0};

// System-wide clock that measures real (wall-clock) time. Clock is affected by:
// * discontinuous jumps in the system time
// * incremental adjustments preformed by adjtime
//#define MY_CLOCK CLOCK_REALTIME

// Clock that cannot be set and represents monotonic time since some unspecified
// starting point.
// This clock is not affected by:
// * discountinuous jumps in the system time
//
// This clock is affected by:
// * incremental adjustments performed by adjtime.
//#define MY_CLOCK CLOCK_MONOTONIC

// Provides access to a raw hardware-based time that is not subject to:
// * NTP adjustments 
// * incremental adjustments performed by adjtime
#define MY_CLOCK CLOCK_MONOTONIC_RAW

// A faster but less precise version of CLOCK_REALTIME. Use when you need a
// very fast but not finegrained timestamps.
//#define MY_CLOCK CLOCK_REALTIME_COARSE

// A faster but less precise version of CLOCK_MONOTONIC. Use when you need a
// very fast but not finegrained timestamps.
//#define MY_CLOCK CLOCK_MONOTONIC_COARSE

#define TEST_ITERATIONS (100)

// This is the function that is run when a new thread is spawned in main().
void *delay_test(void *threadID)
{
  int idx, rc;
  unsigned int max_sleep_calls=3;
  int flags = 0;
  struct timespec rtclk_resolution; // struct holding an interval broken down into seconds and nanoseconds

  sleep_count = 0;

  // clock_getres() finds the resolution (precision) of the specified clock
  // clockid [MY_CLOCK], and stores the result in the struct timespec pointed to
  // by res [rtclk_resolution].
  if(clock_getres(MY_CLOCK, &rtclk_resolution) == ERROR)
  {
      perror("clock_getres");
      exit(-1);
  }
  else
  {
      printf("\n\nPOSIX Clock demo using system RT clock with resolution:\n\t%ld secs, %ld microsecs, %ld nanosecs\n", rtclk_resolution.tv_sec, (rtclk_resolution.tv_nsec/1000), rtclk_resolution.tv_nsec);
  }

  for(idx=0; idx < TEST_ITERATIONS; idx++)
  {
      printf("test %d\n", idx);

      /* run test for defined seconds */
      sleep_time.tv_sec=TEST_SECONDS;
      sleep_time.tv_nsec=TEST_NANOSECONDS;
      sleep_requested.tv_sec=sleep_time.tv_sec;
      sleep_requested.tv_nsec=sleep_time.tv_nsec;

      /* start time stamp */ 
      // retrieve the time of the specified clock clockid [MY_CLOCK]
      clock_gettime(MY_CLOCK, &rtclk_start_time);

      /* request sleep time and repeat if time remains */
      do 
      {
          // suspends the calling thread until either:
          // * at least the time specified by *req [sleep_time] has elapsed.
          // * or the delivery of the signal which invocates a handler in the
          //   thread or terminates the process.
          //
          // returns 0 on sucessfully sleeping the requested time.
          // returns -1 if the call is interrupted by a signal handler
          // and writes the remaining time into rem [remaining_time]
          //
          // Loop until all time has been slept.
          if(rc=nanosleep(&sleep_time, &remaining_time) == 0) 
          {
            break;
          }
         
          sleep_time.tv_sec = remaining_time.tv_sec;
          sleep_time.tv_nsec = remaining_time.tv_nsec;
          sleep_count++;
      } 
      while (((remaining_time.tv_sec > 0) || (remaining_time.tv_nsec > 0))
		      && (sleep_count < max_sleep_calls));

      // retrieve the time of the specified clock clockid [MY_CLOCK]
      clock_gettime(MY_CLOCK, &rtclk_stop_time);

      // calculate the elapsed time
      delta_t(&rtclk_stop_time, &rtclk_start_time, &rtclk_dt);

      // calculate the difference between the actual time elapsed vs the
      // requested sleep time.
      delta_t(&rtclk_dt, &sleep_requested, &delay_error);

      end_delay_test();
  }

}

// displays test results with varying levels of precision.
// first print is the elapsed clock time for the test run
// the second print is the error compared to the requested sleep time.
void end_delay_test(void)
{
    double real_dt;

// debug print
#if 0
  printf("MY_CLOCK start seconds = %ld, nanoseconds = %ld\n", 
         rtclk_start_time.tv_sec, rtclk_start_time.tv_nsec);
  
  printf("MY_CLOCK clock stop seconds = %ld, nanoseconds = %ld\n", 
         rtclk_stop_time.tv_sec, rtclk_stop_time.tv_nsec);
#endif

  // prints the elapsed test time with varying degrees of precision
  real_dt=d_ftime(&rtclk_start_time, &rtclk_stop_time);
  printf("MY_CLOCK clock DT seconds = %ld, msec=%ld, usec=%ld, nsec=%ld, sec=%6.9lf\n", 
         rtclk_dt.tv_sec, rtclk_dt.tv_nsec/1000000, rtclk_dt.tv_nsec/1000, rtclk_dt.tv_nsec, real_dt);

// more debug prints
#if 0
  printf("Requested sleep seconds = %ld, nanoseconds = %ld\n", 
         sleep_requested.tv_sec, sleep_requested.tv_nsec);

  printf("\n");
  printf("Sleep loop count = %ld\n", sleep_count);
#endif
// prints the calculated error between the elapsed test time vs the requested
// sleep time with varying degrees of precision.
  printf("MY_CLOCK delay error = %ld, nanoseconds = %ld\n", 
         delay_error.tv_sec, delay_error.tv_nsec);
}

// this variable decides if the test is run in a seperate thread, or in the
// parent thread (i.e. this app/main loop)
#define RUN_RT_THREAD

void main(void)
{
   int rc, scope;

   printf("Before adjustments to scheduling policy:\n");
   print_scheduler();

#ifdef RUN_RT_THREAD
   // Init the 'thread attributes' object with default attribute values
   pthread_attr_init(&main_sched_attr); 

   // Sets the 'inherit-scheduler' attribute of the thread attributes object.
   //
   // The inherit-scheduler attribute determines whether a thread created using 
   // the thread attributes object attr will inherit its scheduling
   // attributes from the calling thread or whether it will take them
   // from attr.
   //
   // PTHREAD_EXPLICIT_SCHED: Threads created using [main_sched_attr] take their
   // scheduling attributes from the values specified by the attributes object.
   pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED); 

   // Sets the 'scheduling policy' attribute of the thread attributes object.
   //
   // The 'scheduling policy' attribute determines the scheduling policy of a
   // thread created using the [main_sched_attr] object. 
   //
   // SCHED_FIFO: - can be used only with *static priorities* higher than 0.
   //               which means that when a SCHED_FIFO thread becomes runnable, it
   //               will always preempt any currently running SCHED_OTHER,
   //               SCHED_BATCH, or SCHED_IDLE thread.
   //             - Simple scheduling algorithm *without* time slicing. 
   //             - See man pages for more rules
   pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);

   // Returns the maximum priority value that can be used with the scheduling
   // algorithm identified by [SCHED_FIFO]. 
   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   // Returns the minimum priority value that can be used with the scheduling
   // algorithm identified by [SCHED_FIFO]. 
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);

   main_param.sched_priority = rt_max_prio; // set priority for scheduling parameters struct
   
   // Sets both the scheduling policy and parameters for the thread whos ID is
   // specified in pid. 
   //
   // Essentially sets the current running thread's scheduling type to
   // SCHED_FIFO with the max priority value allowed for SCHED_FIFO.
   rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
   if (rc)
   {
       printf("ERROR; sched_setscheduler rc is %d\n", rc);
       perror("sched_setschduler"); exit(-1);
   }

   printf("After adjustments to scheduling policy:\n");
   print_scheduler();

   main_param.sched_priority = rt_max_prio;
   // Set the scheduling parameter attributes of the thread attributes object
   // [main_sched_attr] to the values specified in the buffer pointed to by
   // param [main_param].
   // 
   // i.e. SCHED_FIFO, and max priority allowed by SCHED_FIFO
   pthread_attr_setschedparam(&main_sched_attr, &main_param);

   rc = pthread_create(&main_thread, &main_sched_attr, delay_test, (void *)0);
   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror("pthread_create");
       exit(-1);
   }

   // wait for test thread to complete.
   // this is a blocking call.
   pthread_join(main_thread, NULL);

   // destroy the pthread attribute specified by attr [main_sched_attr].
   if(pthread_attr_destroy(&main_sched_attr) != 0)
     perror("attr destroy");
#else
   delay_test((void *)0);
#endif

   printf("TEST COMPLETE\n");
}

