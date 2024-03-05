#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_RESOURCES 5
int available_resources = MAX_RESOURCES;
pthread_mutex_t mtx;

clock_t start;
struct timespec start_time, stop_time;

struct doctor{
    int id, patientNumber;
    bool isAvailable;
} doctors[5];

struct patient{
    int id, doctorIndex;
    float arrivalTime, waitTime, consultationTime;
    bool isConsulted;
}patients[1000];

bool isAnyAvailable()
{
    for(int i = 0; i < 5; i++)
        if(doctors[i].isAvailable)
            return true;
    return false;
}

int getDoctor()
{
    for(int i = 0; i < 5; i++)
        if(doctors[i].isAvailable)
            return i;
    return -1;
}

int occupyDoctor(int patientIndex)
{
    pthread_mutex_lock(&mtx);
    if(!isAnyAvailable())
    {
        pthread_mutex_unlock(&mtx);
        return -1;
    }

    int dIndex = getDoctor();
    patients[patientIndex].doctorIndex = dIndex;
    doctors[dIndex].isAvailable = false;

    clock_gettime(CLOCK_REALTIME, &stop_time);
    patients[patientIndex].waitTime = (stop_time.tv_sec - start_time.tv_sec) + (stop_time.tv_nsec - start_time.tv_nsec) / 1.0e9 - patients[patientIndex].arrivalTime;

    printf("Patient %d goes to the doctor %d after waiting for %.2f seconds.\n", patients[patientIndex].id, doctors[dIndex].id, patients[patientIndex].waitTime);

    float consTime=(float)rand()/(float)(RAND_MAX/9) + 1;
    patients[patientIndex].consultationTime = consTime;

    pthread_mutex_unlock(&mtx);
    usleep(consTime * 1000000);
    return 0;
}

int freeDoctor(int patientIndex)
{
    pthread_mutex_lock(&mtx);

    int dIndex = patients[patientIndex].doctorIndex;
    doctors[dIndex].isAvailable = true;
    doctors[dIndex].patientNumber++;
    patients[patientIndex].isConsulted = true;

    printf("Patient %d finished their consultation to the doctor %d.\nConsulation lasted %.2f seconds.\n", patients[patientIndex].id, doctors[dIndex].id, patients[patientIndex].consultationTime);
    
    pthread_mutex_unlock(&mtx);
    return 0;
}

void *
newPatient(void *v)
{
    pthread_mutex_lock(&mtx);

    int* curPatient = (int*) v;
    
    clock_gettime(CLOCK_REALTIME, &stop_time);
    patients[curPatient[0]].arrivalTime = (stop_time.tv_sec - start_time.tv_sec) + (stop_time.tv_nsec - start_time.tv_nsec) / 1.0e9;
    
    printf("Patient %d arrives at the clinic at %.2f seconds.\n", patients[curPatient[0]].id, patients[curPatient[0]].arrivalTime);
    
    pthread_mutex_unlock(&mtx);

    while(occupyDoctor(curPatient[0])==-1);
    freeDoctor(curPatient[0]);
    
    return NULL;
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        perror(NULL);
        return errno;
    }

    int N=atoi(argv[1]);
    if(N > 1000)
    {
        perror(NULL);
        return errno;
    }

    pthread_t thr[N];
    int v[N][1];

    for(int i = 0; i < 5; i++)
    {
        doctors[i].id = i + 1;
        doctors[i].isAvailable = true;
        doctors[i].patientNumber = 0;
    }

    for(int i = 0; i < N; i++)
    {
        patients[i].id = i + 1000;
        patients[i].doctorIndex = -1;
        patients[i].isConsulted = false;
        v[i][0] = i;
    }

    if(pthread_mutex_init(&mtx, NULL)) {
        perror(NULL);
        return errno;
    }

    srand(time(NULL));
    
    clock_gettime(CLOCK_REALTIME, &start_time);

    for(int i = 0; i < N; i++)
    {
        void *arg = (void *)v[i];

        float waitNewPatient=(float)rand()/(float)(RAND_MAX/2);
        usleep(waitNewPatient * 1000000);

        if(pthread_create(&thr[i], NULL, newPatient, arg))
        {
            perror(NULL);
            return errno;
        }
    }

    for(int i = 0; i < N; i++)
    {
        void *rez;
        if(pthread_join(thr[i], &rez))
        {
            perror(NULL);
            return errno;
        }
    }

    pthread_mutex_destroy(&mtx);

    printf("\n");
    for(int i = 0; i < 5; i++)
        printf("Doctor %d had %d patients.\n", doctors[i].id, doctors[i].patientNumber);

    return 0;
}