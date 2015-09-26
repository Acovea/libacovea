/*
    evobench.c
    Standard C version
    25 March 2004

    Written by Scott Robert Ladd.
    No rights reserved. This is public domain software, for use by anyone.

    A evolutionary benchmark that can be used as a fitness test for
    evolving optimal compiler options via genetic algorithm.
    
    This benchmark uses a genetic algorithm to minimize a 2D function.
    The population size and generation count are ridiculously high, as
    thye answer can usually be found in a few dozen generations with
    a much smaller population. However, this is a benchmark, so I make
    if "work" harder to test various aspects of code generation.

    Note that the code herein is design for the purpose of testing 
    computational performance; error handling and other such "niceties"
    is virtually non-existent.
    
    Actual benchmark results can be found at:
            http://www.coyotegulch.com

    Please do not use this information or algorithm in any way that might
    upset the balance of the universe or in jurisdictions where evolution
    have been declared invalid. After all, if evolution doesn't exist, this
    program can't work, and you won't be running it, now can you?
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <math.h>

// adjust size of test for environment
static const int N = 200;

//-------------------------------------------------------------------
//	Marsaglia's MWC1038 random number generator
//    overkill for this program, but different from other tests.
//  
//  A 64-bit processor will handily outperform a 32-bit processor on
//    this test, given that this generator uses 64-bit integer math.

#define MWC1038_QN 1038
static uint32_t mwc1038_q[MWC1038_QN];
static uint32_t mwc1038_c;
static int      mwc1038_i;
static bool     mwc1038_inited = false;

//---------------------------------------------------------------------------
//  random number generator; constant seed so every run is identical
static double rand_double()
{
    if (!mwc1038_inited)
    {
        mwc1038_q[0] = 299792457UL;

        // Set the array using one of Knuth's generators
        for (int i = 1; i < MWC1038_QN; ++i)
            mwc1038_q[i] = 1812433253UL * (mwc1038_q[i - 1] ^ (mwc1038_q[i - 1] >> 30)) + i;

        mwc1038_c = mwc1038_q[MWC1038_QN-1] % 61137367UL;
        mwc1038_i = 1037;
        mwc1038_inited = true;
    }
    
    uint64_t temp = 611373678ULL * mwc1038_q[mwc1038_i]+ mwc1038_c;
    mwc1038_c = (temp >> 32);
    
    uint32_t result;
    
    if(--mwc1038_i)
        result = mwc1038_q[mwc1038_i] = temp;
    else
    {
        mwc1038_i = 1037;
        result = mwc1038_q[0] = temp;
    }
    
    // compiler should precalc constant division here
    return (double)result * (1.0 / 4294967296.0);
}

static double rand_gene()
{
    // a value in the range [-1,1]
    return rand_double() * 2.0 - 1.0;
}

//-------------------------------------------------------------------
//	function to be maximized
double formula(double x, double y)
{
    // run it through the formula
    static const double PI = 3.141592653589793238;

    // automatically reject out-of-range values    
    if ((x > 1.0) || (x < -1.0) || (y > 1.0) || (y < -1.0))
        return DBL_MIN;

    // number crunch -- chomp, chomp
    double z = 1.0 / (0.8 + (x + 0.5) * (x + 0.5)
                 + 2.0 * (y - 0.5) * (y - 0.5)
                 - 0.3 * cos(3.0 * PI * x)
                 - 0.4 * cos(4.0 * PI * y));
	 
    return z;
}

//-------------------------------------------------------------------
// a genetic algorithm to optimize the output of BlackBox
void optimize(size_t pop_size,
              size_t num_gens,
              float  cross_rate,
              float  mutate_rate)
{
    // adjust any invalid parameters
    if (pop_size < 10)
        pop_size = 10;

    if (num_gens < 1)
        num_gens = 1;

    if (cross_rate < 0.0F)
        cross_rate = 0.0F;
    else
        if (cross_rate > 1.0F)
            cross_rate = 1.0;

    if (mutate_rate < 0.0F)
        mutate_rate = 0.0F;
    else
        if (mutate_rate > 1.0F)
            mutate_rate = 1.0;

    // allocate pop_x and fitness buffers
    double * pop_x   = (double *)malloc(sizeof(double) * pop_size);
    double * pop_y   = (double *)malloc(sizeof(double) * pop_size);
    double * child_x = (double *)malloc(sizeof(double) * pop_size);
    double * child_y = (double *)malloc(sizeof(double) * pop_size);
    double * fitness = (double *)malloc(sizeof(double) * pop_size);
    
    // various variables
    double most_fit_x, most_fit_y, fit_high, fit_low;
    double fit_total, fit_avg;
    size_t i, counter, selection, father, mother, start;
    
    // create initial pop_x with random values
    for (i = 0; i < pop_size; ++i)
    {
        pop_x[i] = rand_gene();
        pop_y[i] = rand_gene();
    }
    
    // start with generation zero
    counter = 0;
    
    while (true) // loop breaks in the middle
    {
        // initialize for fitness testing
        fit_low   = DBL_MAX;
        fit_high  = DBL_MIN;
        fit_total = 0.0F;
        
        // fitness testing
        for (i = 0; i < pop_size; ++i)
        {
            // call fitness function and store result
            fitness[i] = formula(pop_x[i],pop_y[i]);
            
            // keep track of best fitness
            if (fitness[i] > fit_high)
            {
                fit_high   = fitness[i];
                most_fit_x = pop_x[i];
                most_fit_y = pop_y[i];
            }
            
            // keep track of least fitness
            if (fitness[i] < fit_low)
                fit_low = fitness[i];
            
            // total fitness
            fit_total += fitness[i];
        }
        
        // compute average fitness
        fit_avg = fit_total / (double)pop_size;

        // scale fitness values
        fit_low += 1.0; // ensures that the least fitness is one

        // recalculate total fitness to reflect scaled values
        fit_total = 0.0F;

        for (i = 0; i < pop_size; ++i)
        {
            fitness[i] -= fit_low;            // reduce by smallest fitness
            fitness[i] *= fitness[i];         // square result of above
            fit_total  += (double)fitness[i]; // add into total fitness
        }
        
        // exit if this is final generation
        if (counter == num_gens)
        {
            // uncomment below to see if correct reult (-0.655,0.5) is found)
            // printf("best = (%7.3f,%7.3f)\n",most_fit_x,most_fit_y);
            break;
        }
        
        // elitist selection, replace first item with best
        child_x[0] = most_fit_x;
        child_y[0] = most_fit_y;
        
        // create new population
        for (i = 1; i < pop_size; ++i)
        {
            if (rand_double() < mutate_rate)
                child_x[i] = rand_gene();
            else
            {
                // roulette-select parent
                selection = (size_t)(rand_double() * fit_total);
                mother    = 0;

                while (selection > fitness[mother])
                {
                    selection -= fitness[mother];
                    ++mother;
                }

                child_x[i] = pop_x[mother];
            }
            
            if (rand_double() < mutate_rate)
                child_y[i] = rand_gene();
            else
            {
                // roulette-select parent
                selection = (size_t)(rand_double() * fit_total);
                father    = 0;

                while (selection > fitness[father])
                {
                    selection -= fitness[father];
                    ++father;
                }

                child_y[i] = pop_y[father];
            }
        }
        
        // exchange old pop_x with new one
        double * temp = child_x;
        child_x = pop_x;
        pop_x   = temp;

        temp    = child_y;
        child_y = pop_y;
        pop_y   = temp;

        // increment generation
        ++counter;
    }
        
    // delete pop_x and fitness arrays
    free(pop_x);
    free(pop_y);
    free(child_x);
    free(child_y);
    free(fitness);
}
    
int main(int argc, char* argv[])
{
    // do we have verbose output?
    bool   ga_testing = false;
    
    if (argc > 1)
    {
        for (int n = 1; n < argc; ++n)
        {
            if (!strcmp(argv[n],"-ga"))
            {
                ga_testing = true;
                break;
            }
        }
    }
    
    // get starting time    
    struct timespec start, stop;
    clock_gettime(CLOCK_REALTIME,&start);
    
    // evolve
    optimize(1000,N,0.5,0.1);

    // get final time
    clock_gettime(CLOCK_REALTIME,&stop);        
    double run_time = (stop.tv_sec - start.tv_sec) + (double)(stop.tv_nsec - start.tv_nsec) / 1000000000.0;

    // report runtime
    if (ga_testing)
        printf("%f",run_time);
    else        
        printf("evobench (Std. C) run time: %f\n",run_time);
    
    fflush(stdout);
    
    //done
    return 0;
}
