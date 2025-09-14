
#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sched.h>     
#include <time.h>       
#include <pthread.h>
#include <sched.h>
#include "c_augury.h"

#define MSB_MASK 0x8000000000000000

uint64_t get_time_nano(int32_t zero_dependency)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void pin_cpu(size_t core_no)
{
    if (core_no >= 4) { 
        assert(0 && "error! make sure 0 <= core_no <= 3");
    }
    
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_no, &cpuset);
    
    int result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        perror("pthread_setaffinity_np");
        assert(0 && "Failed to set CPU affinity");
    }
}

uint64_t c_sleep(uint64_t duration, uint64_t __trash)
{
    uint64_t T1 = 0, T2 = 0;
    uint64_t train_time;
    uint64_t base = 1;

    T1 = get_time_nano(__trash & MSB_MASK);
    __trash = (__trash + T1) & MSB_MASK;

    do
    {
        T2 = get_time_nano(__trash & MSB_MASK);
        train_time = (T2 - T1) | (__trash & MSB_MASK);
        
        uint64_t count = 0;
        do
        {
            count += base | (__trash & MSB_MASK);
        } while(count < 9999);
    } while(train_time < duration);

    return train_time;
}

uint64_t c_thrash_cache(uint64_t* thrash_addr, \
    uint32_t size_of_thrash_array, uint64_t __trash)
{
    volatile uint64_t *thrash_arr = thrash_addr;

    for (uint32_t j = 0; j < size_of_thrash_array / sizeof(uint64_t) - 2;
            j++) {
        __trash += (thrash_arr[j] ^ __trash) & 0b1111;
        __trash += (thrash_arr[j + 1] ^ __trash) & 0b1111;
        __trash += (thrash_arr[j + 2] ^ __trash) & 0b1111;
    }
    return __trash;
}

uint64_t flush_evset(uint64_t* evset_start, uint32_t num_of_ptrs)
{
    volatile uint64_t **aop = (uint64_t**)evset_start;
    uint64_t __trash = 0;
    for (uint32_t j = 0; j < num_of_ptrs; j++) {
        __trash = *aop[j % num_of_ptrs];
        __trash = *aop[(j+1) % num_of_ptrs];
        __trash = *aop[(j+2) % num_of_ptrs];
        __trash = *aop[(j+3) % num_of_ptrs];
        __trash = *aop[(j+4) % num_of_ptrs];
        __trash = *aop[(j+5) % num_of_ptrs];
        __trash = *aop[(j+6) % num_of_ptrs];
        __trash = *aop[(j+7) % num_of_ptrs];
        __trash = *aop[(j+8) % num_of_ptrs];
        __trash = *aop[(j+9) % num_of_ptrs];
        __trash = *aop[(j+10) % num_of_ptrs];
        __trash = *aop[(j+11) % num_of_ptrs];
        __trash = *aop[(j+12) % num_of_ptrs];
    }
    return __trash;
}

// openssl constant-time swap for uint64
void constant_time_cond_swap_64(uint64_t mask, uint64_t *a,
                                uint64_t *b)
{
    uint64_t xor = *a ^ *b;

    xor &= mask;
    *a ^= xor;
    *b ^= xor;
}