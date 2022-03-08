/*
*@File:threadsafemutex.c
*@author:Shreyan Prabhu Dhananjayan and Armando Pinales
*@brief: 
*@date:28th February, 2021 
*@Reference:https://stackoverflow.com/questions/46365448/pthread-mutex-timedlock-exiting-prematurely-without-waiting-for-timeout
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <syslog.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#define NUM_THREADS 3
#define MULTIPLIER 1000000000L		/*To convert in to nanseconds*/
struct thread_data
{
	pthread_mutex_t *mutex;
	pthread_t thread_id;
	int thread_priority;
};
typedef struct thread_data thread_data_t;
			
struct attitude_data				/*Altitude data*/
{
	double X;
	double Y;
	double Z;
	double acceleration;
	double roll;
	double pitch;
	double yawRate;
        struct timespec sampleTime;
};

pthread_attr_t rt_sched_attr[NUM_THREADS];
pthread_attr_t main_attr;
pid_t mainpid;
struct sched_param rt_param[NUM_THREADS];
struct sched_param main_param;
int maximum_priority;
thread_data_t threadparams[NUM_THREADS];
typedef struct attitude_data attitude_t;
attitude_t attitude;
pthread_mutex_t mutex1;
struct timespec start,end;

/**
*@function:update_timespec
*@brief:Staring point of the thread 0
*@argument:Thread parameters structure
*@return: Updated Thread paramters
*/


void *wait_mutex(void *thread_param)
{
	struct timespec wait;
	int status;
	thread_data_t *thread_param_t = (thread_data_t *) thread_param;
	clock_gettime(CLOCK_REALTIME, &start);
	while(1)
	{
		clock_gettime(CLOCK_REALTIME, &wait);
		wait.tv_sec+=10;
		status = pthread_mutex_timedlock(thread_param_t->mutex,&wait);
		if(status == ETIMEDOUT)
		{
			clock_gettime(CLOCK_REALTIME, &end);
			printf("NO NEW DATA AVAILABLE AT: %ldSeconds\n", end.tv_sec - start.tv_sec);	
		}
		else
		{
			printf("Lock is obtained\n");
		}
	
	}

}
void *update_timespec(void *thread_param)
{

	double initialAttitudeValue = 100;
	thread_data_t *thread_param_t = (thread_data_t *) thread_param;
	for(int i=0; i<3 ; i++)
	{
		pthread_mutex_lock(thread_param_t->mutex);    /*Critical Section starts*/
		printf("\n------------Attitude Values getting updated------------\n");
		attitude.X = initialAttitudeValue;
		attitude.Y = initialAttitudeValue;
		attitude.Z = initialAttitudeValue;
		attitude.acceleration = initialAttitudeValue;
		attitude.roll = initialAttitudeValue;
		attitude.pitch = initialAttitudeValue;
		attitude.yawRate = initialAttitudeValue;
		clock_gettime(CLOCK_MONOTONIC, &attitude.sampleTime);
	        pthread_mutex_unlock(thread_param_t->mutex); /*Critical Section ends*/
	        initialAttitudeValue += 5;
	        printf("------------------Attitude Values Updated---------------\n\n");
	        usleep(10);
	}
	
	return (thread_param);
}

/**
*@function:read_timespec
*@brief:Staring point of the thread 1
*@argument:Thread parameters structure
*@return: Thread paramters
*/
void *read_timespec(void *thread_param)
{
	thread_data_t *thread_param_t = (thread_data_t *) thread_param;
	for(int i=0; i<3 ; i++)
	{
		pthread_mutex_lock(thread_param_t->mutex); /*Critical Section starts*/
		printf("-----------Updated Values is read by another thead--------\n");
		printf("Value of X = %lf\n", attitude.X);
		printf("Value of Y = %lf\n", attitude.Y);
		printf("Value of Z = %lf\n", attitude.Z);
		printf("Value of acceleration = %lf\n", attitude.acceleration);
		printf("Value of Roll = %lf\n", attitude.roll);
		printf("Value of pitch  = %lf\n", attitude.pitch);
		printf("Value of Yaw Rate = %lf\n", attitude.yawRate);
		printf("Execution time since start = %ld nanoseconds\n", (MULTIPLIER* (attitude.sampleTime.tv_sec - start.tv_sec)) + (attitude.sampleTime.tv_nsec - start.tv_nsec));
		printf("-----------All the updated values are read----------------\n");
	        pthread_mutex_unlock(thread_param_t->mutex); /*Critical Section ends*/
		usleep(10);
	}
	return (thread_param);

}
int main()
{
	/*Setting up scheduling policy and priority for main thread*/
	mainpid=getpid();
	maximum_priority = sched_get_priority_max(SCHED_FIFO);
	int status = sched_getparam(mainpid, &main_param);
	main_param.sched_priority = maximum_priority;
	status = sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
	status=pthread_mutex_init(&mutex1 , NULL);
	if( status != 0)
	{
		syslog(LOG_ERR, "ERROR: Mutex Initialization");
		exit(-1);
	}
	
	for(int i=0; i < NUM_THREADS; i++)
	{
		threadparams[i].thread_id = i;
		threadparams[i].mutex = &mutex1;
		/*Setting up scheduling policy and priority for all the threads*/
		status = pthread_attr_init (&rt_sched_attr[i]);
		status = pthread_attr_setinheritsched (&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
		status = pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
		rt_param[i].sched_priority=maximum_priority - i;
		pthread_attr_setschedparam(&rt_sched_attr[i],&rt_param[i]);
		threadparams[i].thread_priority=rt_param[i].sched_priority;
	}
	
	/*Starting the execution time*/
	clock_gettime(CLOCK_MONOTONIC, &start);
	
	/*Creating and invoking threads*/
	pthread_create(&(threadparams[0].thread_id) ,&rt_sched_attr[0], update_timespec, (void *)&threadparams[0]);
	pthread_create(&(threadparams[1].thread_id) ,&rt_sched_attr[1], read_timespec, (void *)&threadparams[1]);
	/*Thread and Mutex cleanup*/
	for(int j=0; j < 2; j++)
	{
		pthread_join(threadparams[j].thread_id, NULL);
	}
	pthread_mutex_lock(&mutex1);
	pthread_create(&(threadparams[2].thread_id), &rt_sched_attr[2], wait_mutex, (void *)&threadparams[2]);
	pthread_join(threadparams[2].thread_id, NULL);
	
	pthread_mutex_destroy(&mutex1);	
	return 0;

}
