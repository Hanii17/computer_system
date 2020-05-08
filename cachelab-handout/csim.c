/* 
 * csim.c - A cache simulator that can replay traces from Valgrind
 *     and output statistics such as number of hits, misses, and
 *     evictions.  The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *  1. Each load/store can cause at most one cache miss. (I examined the trace,
 *  the largest request I saw was for 8 bytes).
 *  2. Instruction loads (I) are ignored, since we are interested in evaluating
 *  trans.c in terms of its data cache performance.
 *  3. data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus an possible eviction.
 *
 * The function printSummary() is given to print output.
 * Please use this function to print the number of hits, misses and evictions.
 * This is crucial for the driver to evaluate your work. 
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
   LRU is a counter used to implement LRU replacement policy  */
typedef struct cache_line {
    char valid;
    mem_addr_t tag;
    unsigned long long int lru;
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
unsigned long long int lru_counter = 1;

/* The cache we are simulating */
cache_t cache;  
mem_addr_t set_index_mask;

/* 
 * initCache - Allocate memory, write 0's for valid and tag and LRU
 * also computes the set_index_mask
 */
void initCache()
{
    int i,j;
    cache = (cache_set_t*) malloc(sizeof(cache_set_t) * S);
    for (i=0; i<S; i++){
        cache[i]=(cache_line_t*) malloc(sizeof(cache_line_t) * E);
        for (j=0; j<E; j++){
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].lru = 0;
        }
    }

    /* Computes set index mask */
    set_index_mask = (mem_addr_t) (pow(2, s) - 1);
}


/* 
 * freeCache - free allocated memory
 */
void freeCache()
{
    int i;
	for(i=0;i<S;i++)
	{
		free(cache[i]);//free each set pointer
	}
	free(cache);//free the whole cache pointer
}


/* 
 * accessData - Access data at memory address addr.
 *   If it is already in cache, increast hit_count
 *   If it is not in cache, bring it in cache, increase miss count.
 *   Also increase eviction_count if a line is evicted.
 */
void accessData(mem_addr_t addr)
{
    int i;
    unsigned long long int eviction_lru = 0;
    unsigned int eviction_line = 0;
    mem_addr_t set_index = (addr >> b) & set_index_mask;//得到组编号
    mem_addr_t tag = addr >> (s+b);//提取tag


    cache_set_t cache_set = cache[set_index];//提取出那一组
	int hit = 0;//是否命中
	for(i=0;i<E;i++)
	{
		if(cache_set[i].tag==tag && cache_set[i].valid==1)//命中
		{
			
			hit_count++;
			printf("hit ");
			cache_set[i].lru=0;
			hit=1;
		}
		else
		{
			if(cache_set[i].valid==1)cache_set[i].lru++;//其余行lru+1
			
	}}
		if(hit==0)//未命中
		{
			printf("miss ");
			int j;
			miss_count++;
			for(j=0;j<E;j++)//寻找需要替换的行
			{
				if(cache_set[j].valid==0)//有空闲位置
				{
					eviction_line=j;
					break;
				}
				else
				{
					if(cache_set[j].lru>eviction_lru)//将lru最大的行替换
					{
						eviction_lru=cache_set[j].lru;
						eviction_line=j;
					}
				}
			}
		
        
		 if(cache_set[eviction_line].valid!=0)//有淘汰
		 {
			 printf("eviction ");
			 eviction_count++;
		 }
		 cache_set[eviction_line].valid =1 ;
		 cache_set[eviction_line].lru=0;
		 cache_set[eviction_line].tag=tag;
}
}

/*
 * replayTrace - replays the given trace file against the cache 
 */
//trace_fn是读取的trace文件名
void replayTrace(char* trace_fn)
{
    mem_addr_t addr=0;
    unsigned int len=0;
    FILE* trace_fp = fopen(trace_fn, "r");
	char op=fgetc(trace_fp);

	while(op!=EOF)
	{
		switch(op)
		{
			case 'M': fscanf(trace_fp,"%X,%d",(unsigned int*)&addr,&len);
					  printf("%c %x %u ",op,(int)addr,len);
					  accessData(addr);
					  accessData(addr);
					  printf("\n");
					  break;
			case 'L':fscanf(trace_fp,"%X,%d",(unsigned int*)&addr,&len);
					 printf("%c %x %u ",op,(int)addr,len);
					 accessData(addr);
					 printf("\n");
					 break;
			case 'S':fscanf(trace_fp,"%X,%d",(unsigned int*)&addr,&len);
                     printf("%c %x %u ",op,(int)addr,len);
					 accessData(addr);
					 printf("\n");
					 break;
		    case 'I':fscanf(trace_fp,"%X,%d",(unsigned int*)&addr,&len);
					 printf("%c %x %u ",op,(int)addr,len);
					 printf("\n");
					 break;
			default: break;
		}
              op=fgetc(trace_fp);
	
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
 * main - Main routine 
 */
int main(int argc, char* argv[])
{
    char c;

    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
        switch(c){
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
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        printUsage(argv);
        exit(1);
    }

    /* Compute S, E and B from command line args */
    S = (unsigned int) pow(2, s);
    B = (unsigned int) pow(2, b);
 
    /* Initialize cache */
    initCache();

#ifdef DEBUG_ON
    printf("DEBUG: S:%u E:%u B:%u trace:%s\n", S, E, B, trace_file);
    printf("DEBUG: set_index_mask: %llu\n", set_index_mask);
#endif
 
    replayTrace(trace_file);

    /* Free allocated memory */
    freeCache();

    /* Output the hit and miss statistics for the autograder */
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
