#ifndef __SOURCE_H_
#define __SOURCE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/input.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>

#define VALID 1
#define MOVING -2
#define MAX_SLOT_NUM 5
#define MAX_DEPTH 4

#define SHARED_MEM_NAME "/aGoodMem"
#define SEM_NAME_READ "/aGoodSem"
#define SEM_NAME_WRITE "/anotherGoodSem"
#define SHARED_MEM_SIZE 4096

typedef struct
{
    int *count;
    sem_t** sem;
    int total_count;
    int shm_fd;
    int ussleep;
}thread_arg;

typedef struct 
{
    int x, y;
} coord_grp;

typedef struct
{
    int valid, id;
    coord_grp coord;
} slot;

void get_slot_data(const int fd, slot* slot_grp);
void data_handler(slot* slot_grp, int shm_fd, sem_t** sem, pid_t pid);

void begin_print(int difficulty, int shm_fd, sem_t** sem);
void* Tproducer(void* arg);
void* Tconsumer(void* arg);
#endif