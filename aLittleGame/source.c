#include "source.h"

void get_slot_data(const int fd, slot* slot_grp)
{

    static int cur_slot;
    static coord_grp coord[MAX_SLOT_NUM];

    memset(slot_grp, 0, sizeof(slot) * MAX_SLOT_NUM);

    struct input_event in_event = {0};

    int i = 0;
    for(i = 0; i < MAX_SLOT_NUM; i++)
    {
        slot_grp[i].id = MOVING;//if id > 0, then the slot has a binding touch point,if id = -1,then ts si deleted
    }

    while(1)
    {
        if(sizeof(struct input_event) != read(fd, &in_event, sizeof(struct input_event)))
        {
            perror("failed to read\r\n");
            exit(-1);
        }

        switch(in_event.type)
        {
            case EV_ABS:
            switch(in_event.code)
            {
                case ABS_MT_SLOT:
                cur_slot = in_event.value;
                break;

                case ABS_MT_POSITION_X:
                coord[cur_slot].x = in_event.value;
                slot_grp[cur_slot].valid = VALID;
                break;

                case ABS_MT_POSITION_Y:
                coord[cur_slot].y = in_event.value;
                slot_grp[cur_slot].valid = VALID;
                break;

                case ABS_MT_TRACKING_ID:
                slot_grp[cur_slot].id = in_event.value;
                slot_grp[cur_slot].valid = VALID;
                break;
            }
            break;
            
            case EV_SYN:
            if(in_event.code == SYN_REPORT)
            {
                int i = 0;
                for(i = 0; i < MAX_SLOT_NUM; i++)
                {
                    slot_grp[i].coord = coord[i];
                }
            }
            return;
        }
    }
    
}

void data_handler(slot* slot_grp, int shm_fd, sem_t** sem, pid_t pid)
{
    int i = 0;
    static int success_times = 0;
    char temp;
    int offset;
    for(i = 0; i < MAX_SLOT_NUM; i++)
    {
        if(slot_grp[i].valid == VALID)
        {
            if(slot_grp[i].id == MOVING)
            {
                //printf("<slot: %d> MOVING, current coordinate: (%d, %d)\r\n", i, slot_grp[i].coord.x, slot_grp[i].coord.y);
            }

            if(slot_grp[i].id >= 0)
            {
                sem_wait(sem[0]);
                sem_wait(sem[1]);

                lseek(shm_fd, -1, SEEK_CUR);
                
                read(shm_fd, &temp, 1);

                lseek(shm_fd, 1, SEEK_CUR);


                sem_post(sem[1]);

                switch(temp)
                {
                    case '>':
                    success_times++;
                    printf("One shoot\r\n");
                    break;

                    case '<':
                    success_times--;
                    printf("Miss one\r\n");
                    break;

                    default:
                    printf("error!\r\n");
                }

                if(success_times == 5)
                {
                    printf("Succese game\r\n");
                    kill(pid, SIGKILL);
                    exit(0);
                }
                if(success_times == -2)
                {
                    printf("Lose\r\n");
                    kill(pid, SIGKILL);
                    exit(0);
                }
            }

            if(slot_grp[i].id == -1)
            {
                //printf("<slot: %d> RELEASED\r\n", i);
            }
        }
    }
}
//following is child process functions
pthread_cond_t cos_to_pro = PTHREAD_COND_INITIALIZER;
pthread_cond_t pro_to_cos = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void begin_print(int difficulty, int shm_fd, sem_t** sem)
{
    static thread_arg arg;
    int count = 0;
    arg.count = &count;
    arg.sem = sem;
    arg.shm_fd = shm_fd;
    arg.total_count = 0;

    pthread_t tid[6];
    int ussleep[3] = {310000, 215000, 140000};
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    arg.ussleep = ussleep[difficulty - 1];
    int i = 0;
    for(i = 0; i < difficulty; ++i)
    {
        pthread_create(&tid[i], &attr, Tproducer, &arg);
        pthread_create(&tid[i + 3], &attr, Tconsumer, &arg);
    }

    pthread_exit(NULL);
}

void* Tproducer(void* arg)
{
    thread_arg* t_arg = (thread_arg*)arg;
    int* count = t_arg->count;
    sem_t** sem = t_arg->sem;
    int shm_fd = t_arg->shm_fd;
    int sem_value, sem_value_another;
    int* total_count = &(t_arg->total_count);
    char temp = '>';

    while(1)
    {
        sem_getvalue(sem[1], &sem_value);
        if(sem_value != 0)
        {
            pthread_mutex_lock(&mutex);

            while(*count == MAX_DEPTH)
                pthread_cond_wait(&cos_to_pro, &mutex);
        
            sem_getvalue(sem[0], &sem_value_another);
            if(sem_value_another == 1)
                        sem_wait(sem[0]);
            
            (*count)++;
            putchar('>');
            fflush(stdout);
            
            write(shm_fd, &temp, 1);
            
            sem_getvalue(sem[0], &sem_value_another);
            if(sem_value_another == 0)
                        sem_post(sem[0]);
            ++(*total_count);
            if(*total_count == SHARED_MEM_SIZE)
            {
                printf("lose\r\n");
                exit(0);
            }

            usleep(t_arg->ussleep);

            pthread_cond_signal(&pro_to_cos);
            pthread_mutex_unlock(&mutex);
        }     
    }
    
}

void* Tconsumer(void* arg)
{
    thread_arg* t_arg = (thread_arg*)arg;
    int* count = t_arg->count;
    sem_t** sem = t_arg->sem;
    int shm_fd = t_arg->shm_fd;
    int sem_value;
    int sem_value_another;
    int* total_count = &(t_arg->total_count);
    char temp = '<';
    
    while(1)
    {
        sem_getvalue(sem[1], &sem_value);
        if(sem_value != 0)
        {    
            pthread_mutex_lock(&mutex);

            while(*count == 0)
                pthread_cond_wait(&pro_to_cos, &mutex);
            
            sem_getvalue(sem[0], &sem_value_another);
            if(sem_value_another == 1)
                        sem_wait(sem[0]);
            
            (*count)--;

            putchar('<');
            fflush(stdout);

            ++(*total_count);
            write(shm_fd, &temp, 1);

            sem_getvalue(sem[0], &sem_value_another);
            if(sem_value_another == 0)
                        sem_post(sem[0]);

            if(*total_count == SHARED_MEM_SIZE)
            {
                printf("lose\r\n");
                exit(0);
            }
            usleep(t_arg->ussleep);

            pthread_cond_signal(&cos_to_pro);
            pthread_mutex_unlock(&mutex);
        }

        
    }
}