/*
 * File:        csim.c
 * Description: A cache simulator that can replay traces from Valgrind and
 *              output statistics such as number of hits, misses, and evictions
 *              The replacement policy is Most-Recently Used (MRU).
 *
 * The function printSummary() is given to print output. You MUST use this to
 *     print the number of hits, misses, and evictions incurred by your
 *     simulator. This is crucial for the driver to evaluate your work.
 */
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include "cachelab.h"

//#define DEBUG_ON
#define ADDRESS_LENGTH 64

/* Type: Memory address */
typedef unsigned long long int mem_addr_t;

/* Type: Cache line
   Member mru is a counter used to implement MRU replacement policy */
typedef struct cache_line {
    char valid;
    mem_addr_t tag;
    unsigned long long int mru;
} cache_line_t;

typedef cache_line_t* cache_set_t;
typedef cache_set_t* cache_t;

/* Globals set by command line args */
int verbosity = 0; /* print trace if set */
int s = 0; /* set index bits */
int b = 0; /* block offset bits */
int E = 0; /* associativity */
char* trace_file = NULL;

/* Derived from command line args */
int S; /* number of sets */
int B; /* block size (bytes) */

/* Counters used to record cache statistics */
int miss_count = 0;
int hit_count = 0;
int eviction_count = 0;
unsigned long long int mru_counter = 1;

/* The cache we are simulating */
cache_t cache;
mem_addr_t set_index_mask;

/*
 * initCache - Allocate memory, write 0's for valid, tag, and MRU. Also computes
 *             the set_index_mask.
 */
void initCache()
{
    // TODO: Fill this in
    set_index_mask = S - 1;
    cache = malloc(S * sizeof(cache_set_t));
    for (int i = 0; i < S; i++){
      cache[i] = malloc(E * sizeof(cache_line_t));
      for (int j = 0; j < E; j++) {
        cache[i][j].valid = 0;
        cache[i][j].tag = 0;
        cache[i][j].mru = 0;
      }
    }
}



/*
 * freeCache - Free allocated memory.
 */
void freeCache()
{
    int i;
    for (i = 0; i < S; i++)
    {
        free(cache[i]);
    }
    free(cache);
}

/*
 * accessData - Access data at memory address addr. If it is already in the
 *              cache, increment hit_count. If it is not in the cache, bring it
 *              in the cache and increment miss count instead. Also, increment
 *              eviction_count if a line is evicted.
 */
 void accessData(mem_addr_t addr){
   int set_bit = (addr >> b) & set_index_mask;
   int tag_bit = addr >> (b+s);
   int mru_bit = cache[set_bit][0].mru;
   int m = 0;
   mru_counter++;

   for (int i = 0; i < E; i++) {
     if (cache[set_bit][i].valid && cache[set_bit][i].tag == tag_bit) {
       cache[set_bit][i].mru = mru_counter;
       hit_count++;
       return;
     }
     if (!cache[set_bit][i].valid) {
       cache[set_bit][i].tag = tag_bit;
       cache[set_bit][i].mru = mru_counter;
       cache[set_bit][i].valid = 1;
       miss_count++;
       return;
     }
     if(cache[set_bit][i].mru > mru_bit) {
       mru_bit = cache[set_bit][i].mru;
       m = i;
     }
   }

   cache[set_bit][m].tag = tag_bit;
   cache[set_bit][m].mru = mru_counter;
   miss_count++;
   eviction_count++;
 }

/*
 * replayTrace - Replays the given trace file against the cache.
 */
void replayTrace(char* trace_fn)
{
    char buf[1000];
    mem_addr_t addr=0;
    unsigned int len=0;
    FILE* trace_fp = fopen(trace_fn, "r");

    if (!trace_fp)
    {
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);
    }

    while( fgets(buf, 1000, trace_fp) != NULL )
    {
        /* buf[Y] gives the Yth byte in the trace line */

        /*
         * Read address and length from the trace using sscanf
         *     E.g. sscanf(buf+3, "%llx,%u", &addr, &len);
         */

        /*
         * Access the cache, i.e. call accessData
		 *    NOTE: Be careful to handle 'M' accesses
         */
         if (buf[1] == 'S' || buf[1] == 'L' || buf[1] == 'M'){
           sscanf(buf+3, "%llx,%u", &addr, &len);
           accessData(addr);
           if (buf[1] == 'M'){
             accessData(addr);
         }
         }


}
    fclose(trace_fp);
}


/*
 * printUsage - Print usage info
 */
void printUsage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

/*
 * main - Main routine.
 */
int main(int argc, char* argv[])
{
    char c;

    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1 )
    {
        switch (c)
        {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            case 'v':
                verbosity = 1;
                break;
            case 'h':
                printUsage(argv);
                exit(0);
            default:
                printUsage(argv);
                exit(1);
        }
    }

    /* Make sure that all required command line args were specified */
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL)
    {
        printf("%s: Missing required command line argument\n", argv[0]);
        printUsage(argv);
        exit(1);
    }

    /* Compute S, E, and B from command line args */
    //S =  ?
    //B =  ?
    S = pow(2, s);
    B = pow(2, b);
    /* Initialize cache */
    initCache();

#ifdef DEBUG_ON
    printf("DEBUG: S:%u E:%u B:%u trace:%s\n", S, E, B, trace_file);
    printf("DEBUG: set_index_mask: %llu\n", set_index_mask);
#endif

	/* Read the trace and access the cache */
    replayTrace(trace_file);

    /* Free allocated memory */
    freeCache();

    /* Output the hit and miss statistics for the autograder */
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
