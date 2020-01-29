#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>

#include "15palindrome.h"

#define ENABLE_TRACE 0

#if ENABLE_TRACE
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(...) 
#endif

volatile int run = 1;

#define THREAD_COUNT 3

#if 0
volatile int barrier = THREAD_COUNT;
volatile int barrier_exit = THREAD_COUNT;

size_t wait_barrier()
{
        if (!run) {
                return 0;
        }

        size_t cnt = 0;

        int v = __atomic_sub_fetch(&barrier, 1, __ATOMIC_SEQ_CST);
        if (v == 0) {
                // last thread subtracted 1 from barrier receives 0
                // and resets barrier value
                v = THREAD_COUNT;
                __atomic_store(&barrier, &v, __ATOMIC_SEQ_CST);
        } else {
                // every other thread wailt until last thread reseted barrier
                while (v > 0 && v != THREAD_COUNT) {
                        cnt++;
                        __atomic_load(&barrier, &v, __ATOMIC_SEQ_CST);
                }
        }

        __atomic_thread_fence(__ATOMIC_SEQ_CST);

        v = __atomic_sub_fetch(&barrier_exit, 1, __ATOMIC_SEQ_CST);
        if (v == 0) {
                v = THREAD_COUNT;
                __atomic_store(&barrier_exit, &v, __ATOMIC_SEQ_CST);
        } else {
                while (v > 0 && v != THREAD_COUNT) {
                        cnt++;
                        __atomic_load(&barrier_exit, &v, __ATOMIC_SEQ_CST);
                }
        }

        return cnt;
}

void terminate_barrier()
{
        const int v = -1;
        __atomic_store(&barrier, &v, __ATOMIC_SEQ_CST);
        __atomic_store(&barrier_exit, &v, __ATOMIC_SEQ_CST);
}
#endif


volatile int barrier_0 = THREAD_COUNT;
volatile int barrier_1 = THREAD_COUNT;

size_t half_barrier_wait(volatile int* barrier)
{
        if (!run) {
                return 0;
        }

        size_t cnt = 0;

        int v = __atomic_sub_fetch(barrier, 1, __ATOMIC_RELAXED);
        if (v == 0) {
                // last thread subtracted 1 from barrier receives 0
                // and resets barrier value
                v = THREAD_COUNT;
                __atomic_store(barrier, &v, __ATOMIC_RELAXED);
        } else {
                // every other thread wailt until last thread reseted barrier
                while (v > 0 && v != THREAD_COUNT) {
                        cnt++;
                        __atomic_load(barrier, &v, __ATOMIC_RELAXED);
                }
        }

        //__atomic_thread_fence(__ATOMIC_SEQ_CST);

        return cnt;
}

void half_barrier_terminate()
{
        const int v = -1;
        __atomic_store(&barrier_0, &v, __ATOMIC_RELAXED);
        __atomic_store(&barrier_1, &v, __ATOMIC_RELAXED);
}


primesieve_iterator it;

#define PRIME_BUFFER_SIZE 1024*256

uint64_t prime_buffer[2][PRIME_BUFFER_SIZE];

void* prime_gen_routine(void* arg)
{
        size_t buffer_count = 0;
        size_t wait_count = 0;

        TRACE("gen: started\n");

        TRACE("gen: work 0\n");
        prime_fill_buffer(prime_buffer[0], PRIME_BUFFER_SIZE, &it);
        buffer_count++;

        TRACE("gen: wait\n");
        wait_count += half_barrier_wait(&barrier_0);
        TRACE("gen: work 1\n");
        prime_fill_buffer(prime_buffer[1], PRIME_BUFFER_SIZE, &it);
        buffer_count++;

        while(run) {
                TRACE("gen: wait\n");
                wait_count += half_barrier_wait(&barrier_1);
                if (!run) { break; }
                TRACE("gen: work 0\n");
                prime_fill_buffer(prime_buffer[0], PRIME_BUFFER_SIZE, &it);
                buffer_count++;

                TRACE("gen: wait\n");
                wait_count += half_barrier_wait(&barrier_0);
                if (!run) { break; }
                TRACE("gen: work 1\n");
                prime_fill_buffer(prime_buffer[1], PRIME_BUFFER_SIZE, &it);
                buffer_count++;
        }

        printf("gen: written %lu buffers, %lu bytes, %lu wait calls\n",
                buffer_count, buffer_count*PRIME_BUFFER_SIZE*sizeof(prime_buffer[0][0]), wait_count);
        return 0;
}
 
char base36_buffer[2][PRIME_BUFFER_SIZE * 10]; 

volatile size_t base36_buffer_size[2] = {0};

void* base36_routine(void* arg)
{
        size_t byte_count = 0;
        size_t wait_count = 0;
        size_t begin_wait_count = 0;

        TRACE("36: started\n");

        TRACE("36: wait\n");
        begin_wait_count = half_barrier_wait(&barrier_0);
        TRACE("36: work 0\n");
        base36_buffer_size[0] = base36_print_buffer_2(base36_buffer[0], prime_buffer[0], PRIME_BUFFER_SIZE);
        byte_count += base36_buffer_size[0];

        while(run) {
                TRACE("36: wait\n");
                wait_count += half_barrier_wait(&barrier_1);
                if (!run) { break; }
                TRACE("36: work 1\n");
                memcpy(base36_buffer[1], base36_buffer[0] + base36_buffer_size[0] - 14, 14);
                base36_buffer_size[1] = 14 + base36_print_buffer_2(base36_buffer[1] + 14, prime_buffer[1], PRIME_BUFFER_SIZE);
                byte_count += base36_buffer_size[1];

                TRACE("36: wait\n");
                wait_count += half_barrier_wait(&barrier_0);
                if (!run) { break; }
                TRACE("36: work 0\n");
                memcpy(base36_buffer[0], base36_buffer[1] + base36_buffer_size[1] - 14, 14);
                base36_buffer_size[0] = 14 + base36_print_buffer_2(base36_buffer[0] + 14, prime_buffer[0], PRIME_BUFFER_SIZE);
                byte_count += base36_buffer_size[0];
        }

        printf("36: written %lu bytes, %lu + %lu wait calls\n", byte_count, begin_wait_count, wait_count);
        return 0;
}

const char* palindrome = 0;
int success = 0;
size_t total_offset = 0;

void* search_routine(void* arg)
{
        size_t wait_count = 0;
        size_t begin_wait_count = 0;

        TRACE("search: started\n");

        TRACE("search: wait\n");
        begin_wait_count += half_barrier_wait(&barrier_0);
        TRACE("search: wait\n");
        begin_wait_count += half_barrier_wait(&barrier_1);

        while(run) {
                TRACE("search: work 0\n");
                ssize_t offset = search_15_palindrome(base36_buffer[0], base36_buffer[0] + base36_buffer_size[0]);
                if (offset >= 0) {
                        //printf("SUCCESS\n");
                        half_barrier_terminate();
                        run = 0;
                        success = 1;
                        total_offset += offset;
                        palindrome = base36_buffer[0] + offset;
                        break;
                }
                total_offset += base36_buffer_size[0] - 14;
                TRACE("search: wait\n");
                wait_count += half_barrier_wait(&barrier_0);


                TRACE("search: work 1\n");
                offset = search_15_palindrome(base36_buffer[1], base36_buffer[1] + base36_buffer_size[1]);
                if (offset >= 0) {
                        //printf("SUCCESS\n");
                        half_barrier_terminate();
                        run = 0;
                        success = 1;
                        total_offset += offset;
                        palindrome = base36_buffer[1] + offset;
                        break;
                }
                total_offset += base36_buffer_size[1] - 14;
                TRACE("search: wait\n");
                wait_count += half_barrier_wait(&barrier_1);


                if (total_offset > 1000*1000*1000) {
                        printf("STOP\n");
                        half_barrier_terminate();
                        run = 0;
                        break;
                }
        }

        printf("search: %lu + %lu wait calls\n", begin_wait_count, wait_count);
        return 0;
}


pthread_t gen_thread;
pthread_t base36_thread;


int main(int argc, char **argv) {
        clock_t begin, end;

        init_base36_r2();

        primesieve_init(&it);

        begin = clock();

        pthread_create(&gen_thread, NULL, &prime_gen_routine, NULL);
        cpu_set_t gen_cpuset;
        CPU_ZERO(&gen_cpuset);
        CPU_SET(0, &gen_cpuset);
        pthread_setaffinity_np(gen_thread, sizeof(gen_cpuset), &gen_cpuset);

        pthread_create(&base36_thread, NULL, &base36_routine, NULL);
        cpu_set_t base36_cpuset;
        CPU_ZERO(&base36_cpuset);
        CPU_SET(1, &base36_cpuset);
        pthread_setaffinity_np(base36_thread, sizeof(base36_thread), &base36_cpuset);

        pthread_t search_thread = pthread_self();
        cpu_set_t search_cpuset;
        CPU_ZERO(&search_cpuset);
        CPU_SET(2, &search_cpuset);
        pthread_setaffinity_np(search_thread, sizeof(search_cpuset), &search_cpuset);


        search_routine(0);

        pthread_join(gen_thread, NULL);
        pthread_join(base36_thread, NULL);

        end = clock();

        primesieve_free_iterator(&it);

        if (success) {
                printf("\n");
                printf("Palindrome found ");
                for (size_t i = 0; i < 15; i++) {
                        printf("%c", palindrome[i]);
                }
                printf( " at byte %lu\n", total_offset);
        } else {
                printf("Failed\n");
        }

        printf("Time spend: %lf sec\n", (double) (end - begin) / CLOCKS_PER_SEC);
}
