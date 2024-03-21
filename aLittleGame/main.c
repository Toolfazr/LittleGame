#include "source.h"

#define LCD_PATH "/dev/input/event1"

int main(void)
{   
    shm_unlink(SHARED_MEM_NAME);
    int shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR | O_EXCL, 0666);
    
    sem_t* sem_read = sem_open(SEM_NAME_READ, O_CREAT, 0666, 0);
    sem_t* sem_write = sem_open(SEM_NAME_READ, O_CREAT, 0666, 1);
    sem_t* sem[2];
    sem[0] = sem_read;
    sem[1] = sem_write;
    lseek(shm_fd, 0, SEEK_SET);
    
    ftruncate(shm_fd, SHARED_MEM_SIZE);
    char* ptr = mmap(NULL, SHARED_MEM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    
    int pid;
    switch(pid = fork())
    {
        case -1:
        perror("fork");
        exit(-1);
        break;

        case 0:
        {}
        int difficulty;
        printf("You can choose difficulty: 1 ~ 3(from easy to hard in asending order)\r\n");
        scanf("%d", &difficulty);


        begin_print(difficulty, shm_fd, sem);
        break;

        default:
        {}

        int fd = open(LCD_PATH, O_RDONLY);
        if(fd < 0)
        {
            perror("open error");
            exit(-1);
        }
        slot slot_grp[5];
        for(;;)
        {
            get_slot_data(fd, slot_grp);

            data_handler(slot_grp, shm_fd, sem, pid);
        }
        break;
    }  
}