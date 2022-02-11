// Sam Siewert, August 2020
//
// This example code provides feasibiltiy decision tests for single core fixed priority rate monontic systems only (not dyanmic priority such as deadline driven
// EDF and LLF).  These are standard algorithms which either estimate feasibility (as the RM LUB does) or automate exact analysis (scheduling point, completion test) for
// a set services sharing one CPU core.  This can be emulated on Linux SMP multi-core systemes by use of POSIX thread affinity, to "pin" a thread to a specific core.
//
// Coded based upon standard definition of:
//
// 1) RM LUB based upon model by Liu and Layland
// 2) Scheduling Point - an exact feasibility algorithm based upon Lehoczky, Sha, and Ding exact analysis
// 3) Completion Test - an exact feasibility algorithm
//
// All 3 are also covered in RTECS with Linux and RTOS p. 84 to p. 89
//
// Original references for single core AMP systems:
//
// 1) RM LUB - Liu, Chung Laung, and James W. Layland. "Scheduling algorithms for multiprogramming in a hard-real-time environment." Journal of the ACM (JACM) 20.1 (1973): 46-61.
// 2) Scheduling Point - Lehoczky, John, Lui Sha, and Yuqin Ding. "The rate monotonic scheduling algorithm: Exact characterization and average case behavior." RTSS. Vol. 89. 1989.
// 3) Completion Test - Joseph, Mathai, and Paritosh Pandya. "Finding response times in a real-time system." The Computer Journal 29.5 (1986): 390-395.
//
// References for mulit-core systems:
//
// 1) Bertossi, Alan A., Luigi V. Mancini, and Federico Rossini. "Fault-tolerant rate-monotonic first-fit scheduling in hard-real-time systems."
//    IEEE Transactions on Parallel and Distributed Systems 10.9 (1999): 934-945.
// 2) Burchard, Almut, et al. "New strategies for assigning real-time tasks to multiprocessor systems." IEEE transactions on computers 44.12 (1995): 1429-1442.
// 3) Dhall, Sudarshan K., and Chung Laung Liu. "On a real-time scheduling problem." Operations research 26.1 (1978): 127-140.
//
//
// Deadline Montonic (not implemented in this example, but covered in class and notes):
//
// 1) Audsley, Neil C., et al. "Hard real-time scheduling: The deadline-monotonic approach." IFAC Proceedings Volumes 24.2 (1991): 127-132.
//
// Note that Deadline Monotoic simply uses the deadine interval, D(i) to assign priority, rather than the period interval, T(i) and relaxes T=D constraint.  Anlaysis can
// be done as it is done for RM, but with evaluation of feasbility based upon modified D(i) and with modified fixed priorities.  This is covered by manual analysis examples.
//
// For a more interactive tool, students can use Cheddar:
//
// http://beru.univ-brest.fr/~singhoff/cheddar/
//
// This open source tool handles single and multi-core and allows for modeling of the platform hardware, RTOS/OS, and scheduler with a particular fixed priority or dynamic
// priority policy.
//
// This code is provided primarily so students can learn the methods of worst case analysis and compare exact and estimated feasibility decision testing.
//

#include <math.h>
#include <stdio.h>
#include <stdint.h>

#define TRUE 1
#define FALSE 0
#define U32_T unsigned int

// U=0.7333
U32_T ex0_period[] = {2, 10, 15};
U32_T ex0_wcet[] = {1, 1, 2};

// U=0.9857
U32_T ex1_period[] = {2, 5, 7};
U32_T ex1_wcet[] = {1, 1, 2};

// U=0.9967
U32_T ex2_period[] = {2, 5, 7, 13};
U32_T ex2_wcet[] = {1, 1, 1, 2};

// U=0.93
U32_T ex3_period[] = {3, 5, 15};
U32_T ex3_wcet[] = {1, 2, 3};

// U=1.0
U32_T ex4_period[] = {2, 4, 16};
U32_T ex4_wcet[] = {1, 1, 4};

// ADDED BY STUDENT ///////////////////
// U=1.0
U32_T ex5_period[] = {2, 5, 10};
U32_T ex5_wcet[] = {1, 2, 1};

// U=0.9967
U32_T ex6_period[] = {2, 5, 7, 13};
U32_T ex6_wcet[] = {1, 1, 1, 2};

// U=1
U32_T ex7_period[] = {3, 5, 13};
U32_T ex7_wcet[] = {1, 2, 4};

// U=0.9967
U32_T ex8_period[] = {2, 5, 7, 13};
U32_T ex8_wcet[] = {1, 1, 1, 2};


// function declerations
int completion_time_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);
int scheduling_point_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);
int rate_monotonic_least_upper_bound(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);
int least_laxity_first(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);
int earliest_deadline_first(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);

void run_feasibility_test(
	U32_T numServices, 
	U32_T period[], 
	U32_T wcet[], 
	U32_T deadline[], 
	U32_T case_num, 
	int(*CTF)(U32_T,U32_T[],U32_T[],U32_T[]), 
	int(*RMLUB)(U32_T,U32_T[],U32_T[],U32_T[]), 
	int(*SPF)(U32_T,U32_T[],U32_T[],U32_T[]),
   int(*LLF)(U32_T,U32_T[],U32_T[],U32_T[]),
   int(*EDF)(U32_T,U32_T[],U32_T[],U32_T[])
)
{
	if(numServices == 3)
	{
		printf("Ex-%u U=%4.2f\% (C1=%u, C2=%u, C3=%u; T1=%u, T2=%u, T3=%u; T=D): \n",
			case_num,
			(
				((double)wcet[0]/(double)period[0])*100.0 + 
				((double)wcet[1]/(double)period[1])*100.0 + 
				((double)wcet[2]/(double)period[2])*100.0
			),
			wcet[0],
			wcet[1],
			wcet[2],
			period[0],
			period[1],
			period[2]
			
		);
		
	}
	else if(numServices == 4)
	{
		printf("Ex-%u U=%4.2f\% (C1=%u, C2=%u, C3=%u, C4=%u; T1=%u, T2=%u, T3=%u, T4=%u; T=D): \n",
			case_num,
			(
				((double)wcet[0]/period[0])*100 + 
				((double)wcet[1]/period[1])*100 + 
				((double)wcet[2]/period[2])*100 + 
				((double)wcet[3]/period[3])*100
			),
			wcet[0],
			wcet[1],
			wcet[2],
			wcet[3],
			period[0],
			period[1],
			period[2],
			period[3]
			
		);
	}
	
	printf("===RESULTS===\n");
	if(RMLUB != NULL)
	{
		if(RMLUB(numServices, period, wcet, period) == TRUE)
			printf("RM LUB: FEASIBLE\n");
		else
			printf("RM LUB: INFEASIBLE\n");
	}
	
	if(CTF != NULL)
	{
		if(CTF(numServices, period, wcet, period) == TRUE)
			printf("CTF   : FEASIBLE\n");
      else
			printf("CTF   : INFEASIBLE\n");
	}


	if(SPF != NULL)
	{
		if(SPF(numServices, period, wcet, period) == TRUE)
			printf("SPF   : FEASIBLE\n");
      else
			printf("SPF   : INFEASIBLE\n");
	}

   if(LLF != NULL)
   {
      if(LLF(numServices, period, wcet, period) == TRUE)
         printf("LLF   : FEASIBLE\n");
      else
         printf("LLF   : INFEASIBLE\n");
   }

   if(EDF != NULL)
   {
      if(EDF(numServices, period, wcet, period) == TRUE)
         printf("EDF   : FEASIBLE\n");
      else
         printf("EDF   : INFEASIBLE\n");
   }

		
	printf("\n");
	
}

int main(void)
{ 
    int i;
    U32_T numServices;
	
	
    /*****************************************************************************************************
     * 
     *****************************************************************************************************/
    printf("******** Completion Test Feasibility Example\n");
   
    // EX 0
    numServices = 3;
    run_feasibility_test(numServices, ex0_period, ex0_wcet, ex0_period, 0, completion_time_feasibility, rate_monotonic_least_upper_bound, scheduling_point_feasibility, least_laxity_first, earliest_deadline_first);

    // EX 1
    numServices = 3;
    run_feasibility_test(numServices, ex1_period, ex1_wcet, ex1_period, 1, completion_time_feasibility, rate_monotonic_least_upper_bound, scheduling_point_feasibility, least_laxity_first, earliest_deadline_first);

    // EX 2
    numServices = 4;
    run_feasibility_test(numServices, ex2_period, ex2_wcet, ex1_period, 2, completion_time_feasibility, rate_monotonic_least_upper_bound, scheduling_point_feasibility, least_laxity_first, earliest_deadline_first);

    // EX 3
    numServices = 3;
    run_feasibility_test(numServices, ex3_period, ex3_wcet, ex3_period, 3, completion_time_feasibility, rate_monotonic_least_upper_bound, scheduling_point_feasibility, least_laxity_first, earliest_deadline_first);

    // EX 4
    numServices = 3;
    run_feasibility_test(numServices, ex4_period, ex4_wcet, ex4_period, 4, completion_time_feasibility, rate_monotonic_least_upper_bound, scheduling_point_feasibility, least_laxity_first, earliest_deadline_first);

    // EX 5
    numServices = 3;
    run_feasibility_test(numServices, ex5_period, ex5_wcet, ex5_period, 5, completion_time_feasibility, rate_monotonic_least_upper_bound, scheduling_point_feasibility, least_laxity_first, earliest_deadline_first);
    
    // EX 6
    numServices = 4;
    run_feasibility_test(numServices, ex6_period, ex6_wcet, ex6_period, 6, completion_time_feasibility, rate_monotonic_least_upper_bound, scheduling_point_feasibility, least_laxity_first, earliest_deadline_first);

    // EX 7
    numServices = 3;
    run_feasibility_test(numServices, ex7_period, ex7_wcet, ex7_period, 7, completion_time_feasibility, rate_monotonic_least_upper_bound, scheduling_point_feasibility, least_laxity_first, earliest_deadline_first);
    
    // EX 8
    numServices = 4;
    run_feasibility_test(numServices, ex8_period, ex8_wcet, ex4_period, 8, completion_time_feasibility, rate_monotonic_least_upper_bound, scheduling_point_feasibility, least_laxity_first, earliest_deadline_first);
    //run_feasibility_test(numServices, ex8_period, ex8_wcet, ex4_period, 8, NULL, NULL, NULL, least_laxity_first);
    
    printf("\n\n");

}


int rate_monotonic_least_upper_bound(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[])
{
  double utility_sum=0.0, lub=0.0;
  int idx;

  // Sum the C(i) over the T(i)
  for(idx=0; idx < numServices; idx++)
  {
    utility_sum += ((double)wcet[idx] / (double)period[idx]);
    printf("for %d, wcet=%lf, period=%lf, utility_sum = %lf\n", idx, (double)wcet[idx], (double)period[idx], utility_sum);
  }

  // Compute LUB for number of services
  lub = (double)numServices * (pow(2.0, (1.0/((double)numServices))) - 1.0);
  
  
  // print stuff
  printf("===LUB STATS===\n");
  printf("For %d, Utility_sum = %lf\n", numServices, utility_sum);
  printf("LUB = %lf\n", lub);

  // Compare the utilty to the bound and return feasibility
  if(utility_sum <= lub)
	  return TRUE;
  else
	  return FALSE;
}

int completion_time_feasibility(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[])
{
  int i, j;
  U32_T an, anext;
  
  // assume feasible until we find otherwise
  int set_feasible=TRUE;
   
  //printf("numServices=%d\n", numServices);
 
  // For all services in the analysis 
  for (i=0; i < numServices; i++)
  {
       an=0; anext=0;
       
       for (j=0; j <= i; j++)
       {
           an+=wcet[j];
       }
       
	   //printf("i=%d, an=%d\n", i, an);

       while(1)
       {
             anext=wcet[i];
	     
             for (j=0; j < i; j++)
                 anext += ceil(((double)an)/((double)period[j]))*wcet[j];
		 
             if (anext == an)
                break;
             else
                an=anext;

			 //printf("an=%d, anext=%d\n", an, anext);
       }
       
	   //printf("an=%d, deadline[%d]=%d\n", an, i, deadline[i]);

       if (an > deadline[i])
       {
          set_feasible=FALSE;
       }
  }
  
  return set_feasible;
}

int scheduling_point_feasibility(U32_T numServices, U32_T period[], 
				 U32_T wcet[], U32_T deadline[])
{
   int rc = TRUE, i, j, k, l, status, temp;

   // For all services in the analysis
   for (i=0; i < numServices; i++) // iterate from highest to lowest priority
   {
      status=0;

      // Look for all available CPU minus what has been used by higher priority services
      for (k=0; k<=i; k++) 
      {
	  // find available CPU windows and take them
          for (l=1; l <= (floor((double)period[i]/(double)period[k])); l++)
          {
               temp=0;

               for (j=0; j<=i; j++) temp += wcet[j] * ceil((double)l*(double)period[k]/(double)period[j]);

	       // Can we get the CPU we need or not?
               if (temp <= (l*period[k]))
			   {
				   // insufficient CPU during our period, therefore infeasible
				   status=1;
				   break;
			   }
           }
           if (status) break;
      }

      if (!status) rc=FALSE;
   }
   return rc;
}

int earliest_deadline_first(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[])
{
   // variable declerations
   int ed_idx, ed_val_min, ed_val_current;
   uint16_t max_tick = 0xFFFF;
   int et_so_far[numServices];

   // initialize "execution time so far" array to zero
   for(int i = 0; i < numServices; i++)
   {
      et_so_far[i] = 0;
   }

   // loop for global tick simulation
   for(int current_tick = 0; current_tick < max_tick; current_tick++)
   {
      ed_val_min = 0xFFFF;
      ed_idx = -1;

      //printf("@TICK=%d\n", current_tick);

      // iterate through each service
      for(int idx = 0; idx < numServices; idx++)
      {
         // check if the current task has completed executing. 
         if (wcet[idx] == et_so_far[idx])
         {
            // if the current tick value is not evenly divisible by the current
            // service's period then skip this task because it has already
            // completed it's processing before it's deadline.  
            if(current_tick % period[idx] != 0)
            {
               continue;
            }
            else
            {
               // otherwise, it is the start of a new period for this task, and
               // it must start over. reset this tasks execution time counter.
               et_so_far[idx] = 0;
            }
            
         }

         // calculate time until deadline
         ed_val_current = (
            (period[idx] - (current_tick % period[idx]))
         );
         
         //printf("SERVICE_%d LAXITY=%d\n", idx+1, ll_val_current);

         // if the time remaining until the deadline is greater than the
         // remaining computation time for the task, then the task will fail to
         // meet it's deadline. 
         if(ed_val_current > (wcet[idx] - et_so_far[idx]))
         {
            // printf("Failed on service %d\n", idx+1);
            return FALSE;
         }

         // if the tasks deadline is sooner than the current minimum deadline,
         // set its value as the new minimum.
         if(ed_val_min > ed_val_current)
         {
            ed_idx = idx;
            ed_val_min = ed_val_current;
         }

         
      }
      // printf("\n");
      
      // increment the elapsed time (time it has been able to process) for the
      // task with least laxity. 
      et_so_far[ed_idx] += 1;

      // printf("Running task: %d @TICK=%d\n", ll_idx+1, current_tick);
   }

   return TRUE;
}

int least_laxity_first(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[])
{
   // variable declerations
   int ll_idx, ll_val_min, ll_val_current;
   uint16_t max_tick = 0xFFFF;
   int et_so_far[numServices];

   // initialize "execution time so far" array to zero
   for(int i = 0; i < numServices; i++)
   {
      et_so_far[i] = 0;
   }

   // loop for global tick simulation
   for(int current_tick = 0; current_tick < max_tick; current_tick++)
   {
      ll_val_min = 0xFFFF;
      ll_idx = -1;

      //printf("@TICK=%d\n", current_tick);

      // iterate through each service
      for(int idx = 0; idx < numServices; idx++)
      {
         // check if the current task has completed executing. 
         if (wcet[idx] == et_so_far[idx])
         {
            // if the current tick value is not evenly divisible by the current
            // service's period then skip this task because it has already
            // completed it's processing before it' deadline.  
            if(current_tick % period[idx] != 0)
            {
               continue;
            }
            else
            {
               // otherwise, it is the start of a new period for this task, and
               // it must start over. reset this tasks execution time counter.
               et_so_far[idx] = 0;
            }
            
         }

         // calculate lax time
         ll_val_current = (
            (period[idx] - (current_tick % period[idx])) - (wcet[idx] - et_so_far[idx])
         );
         
         //printf("SERVICE_%d LAXITY=%d\n", idx+1, ll_val_current);

         // if the laxity is negative, then the task cannot meet its deadline
         // and therfore the set of tasks is not feasable under LLF scheduling
         // policy. Return false in this case
         if(ll_val_current < 0)
         {
            // printf("Failed on service %d\n", idx+1);
            return FALSE;
         }

         // if the tasks least laxity is less than the current least laxity of
         // previous calculated tasks, then save its index and ll value.
         if(ll_val_min > ll_val_current)
         {
            ll_idx = idx;
            ll_val_min = ll_val_current;
         }

         
      }
      // printf("\n");
      
      // increment the elapsed time (time it has been able to process) for the
      // task with least laxity. 
      et_so_far[ll_idx] += 1;

      // printf("Running task: %d @TICK=%d\n", ll_idx+1, current_tick);
   }

   return TRUE;
}