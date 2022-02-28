#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define U32_T uint32_t

#define TRUE 1
#define FALSE 0

int deadline_monotonic(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[]);

int main() 
{
    U32_T ex2_period[] = {2, 5, 7, 13};
    U32_T ex2_wcet[] = {1, 1, 1, 2};
    U32_T deadline[] = {2, 3, 7, 15};
    int numServices = 4;

    if (deadline_monotonic(numServices, ex2_period, ex2_wcet, deadline) == 1)
        printf("FEASIBLE");
    else
        printf("INFEASIIBLE");
}


int deadline_monotonic(U32_T numServices, U32_T period[], U32_T wcet[], U32_T deadline[])
{
  double interference;
  double deadline_monotonic_check = 0;

  for (int i = 0; i < numServices; i++) {
        interference = 0;
        for (int j = 0; j<=i-1; j++) {
            interference +=  ceil(((double)(deadline[i]))/((double)period[j]))*wcet[j];
        }
        deadline_monotonic_check = ((double)(wcet[i]) + interference) / ((double)deadline[i]);

        if (deadline_monotonic_check > 1.0) 
            return FALSE;
  }
  return TRUE;
}