/*
    linbench.c

    Written by Scott Robert Ladd (scott@coyotegulch.com)
    No rights reserved. This is public domain software, for use by anyone.

    A number-crunching benchmark using LUP-decomposition to solve a large
    linear equation.

    The code herein is design for the purpose of testing computational
    performance; error handling is minimal.

    Actual benchmark results can be found at:
            http://www.coyotegulch.com

    Please do not use this information or algorithm in any way that might
    upset the balance of the universe or otherwise cause a disturbance in
    the space-time continuum.
*/

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

// embedded random number generator; ala Park and Miller
static       long seed = 1325;
static const long IA   = 16807;
static const long IM   = 2147483647;
static const double AM = 4.65661287525E-10;
static const long IQ   = 127773;
static const long IR   = 2836;
static const long MASK = 123459876;

static double random_double()
{
    long k;
    double result;
    
    seed ^= MASK;
    k = seed / IQ;
    seed = IA * (seed - k * IQ) - IR * k;
    
    if (seed < 0)
        seed += IM;
    
    result = AM * seed;
    seed ^= MASK;
    
    return result;
}

static const int N   =  800;
static const int NM1 =  799; // N - 1
static const int NP1 =  801; // N + 1

static int * lup_decompose(double ** a)
{
    int i, j, k, k2, t;
    double p, temp;
    
    int * perm = (int *)malloc(sizeof(double) * N);
    
    for (i = 0; i < N; ++i)
        perm[i] = i;
    
    for (k = 0; k < NM1; ++k)
    {
        p = 0.0;
        
        for (i = k; i < N; ++i)
        {
            temp = fabs(a[i][k]);
            
            if (temp > p)
            {
                p = temp;
                k2 = i;  
            }
        }
        
        // check for invalid a
        if (p == 0.0)
            return NULL;
    
        // exchange rows
        t = perm[k];
        perm[k] = perm[k2];
        perm[k2] = t;

        for (i = 0; i < N; ++i)
        {
            temp = a[k][i];
            a[k][i] = a[k2][i];
            a[k2][i] = temp;
        }
        
        for (i = k + 1; i < N; ++i)
        {
            a[i][k] /= a[k][k];
            
            for (j = k + 1; j < N; ++j)
                a[i][j] -= a[i][k] * a[k][j];
        }
    }
    
    return perm;
}

static double * lup_solve(double ** a, int * perm, double * b)
{
    int i, j, j2;
    double sum, u;
    
    double * y = (double *)malloc(sizeof(double) * N);
    double * x = (double *)malloc(sizeof(double) * N);
    
    for (int i = 0; i < N; ++i)
    {
        y[i] = 0.0;
        x[i] = 0.0;
    }
    
    for (i = 0; i < N; ++i)
    {
        sum = 0.0;
        j2 = 0;
        
        for (j = 1; j <= i; ++j)
        {
            sum += a[i][j2] * y[j2];
            ++j2;
        }
        
        y[i] = b[perm[i]] - sum;
    }
    
    i = NM1;
    
    while (1)
    {
        sum = 0.0;
        u   = a[i][i];
        
        for (j = i + 1; j < N; ++j)
            sum += a[i][j] * x[j];
        
        x[i] = (y[i] - sum) / u;
        
        if (i == 0)
            break;
        
        --i;
    }
    
    free(y);
    
    return x;
}

int main(void)
{
    int i, j;

    // generate test data            
    double ** a = (double **)malloc(sizeof(double *) * N);
    
    for (i = 0; i < N; ++i)
    {
        a[i] = (double *)malloc(sizeof(double) * N);
        
        for (int j = 0; j < N; ++j)
            a[i][j] = random_double();
    }
    
    double * b = (double *)malloc(sizeof(double) * N);
    
    for (i = 0; i < N; ++i)
         b[i] = random_double();
    
    // get starting time    
    struct timespec start, stop;
    clock_gettime(CLOCK_REALTIME,&start);

    // what we're timing
    int * p = lup_decompose(a);
    double * r = lup_solve(a,p,b);
    
    // calculate run time
    clock_gettime(CLOCK_REALTIME,&stop);        
    double run_time = (stop.tv_sec - start.tv_sec) + (double)(stop.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    // clean up
    for (i = 0; i < N; ++i)
        free(a[i]);
    
    free(a);
    free(b);
    free(p);
    free(r);
    
    // send result
    fprintf(stdout,"%f",run_time);

    // done
    return 0;
}
