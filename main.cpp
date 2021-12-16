#include <queue>
#include <unistd.h>
#include <stdlib.h>
#include <chrono>
#include <semaphore.h>
#include <fstream>
#include <iostream>
#define MAX_SLEEP_TIME 1000
#define MAX_RUN_TIME 1000

/*assume the longest runtime is 1 second for now*/
/*we need a timer class*/
/*assume the longest sleep time is 10 millisecond*/

/*to time any function*/
class Timer
{

    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::duration<float> duration;

public:
    Timer()
    {
        start = std::chrono::high_resolution_clock::now();
    }

    float getTimeElapsed()
    {
        auto end = std::chrono::high_resolution_clock::now();
        duration = end - start;

        return duration.count() * 1000.00f;
    }
};

/*the parameters passed to a new counter thread*/
struct counterArgs
{
    sem_t *semaphore;
    unsigned int *counter;
    std::ofstream *fout;
    unsigned int threadNumber;
    unsigned short *maxTime;
};

struct monitorArgs
{
    sem_t *semaphore;
    unsigned int *counter;
    std::ofstream *fout;
    unsigned short *maxTime;
};

void sleepRandom()
{
    /*go to sleep*/
    srandom((int)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    usleep(random() % MAX_SLEEP_TIME);
}

/*fuction to be passed into pthread create for a new counter thread*/
void *mCounter(void *argsptr)
{
    /*extracting arguments*/
    const counterArgs *args = (counterArgs *)argsptr;

    unsigned int currentCounterValue;
    std::string msg;

    while (!*args->maxTime)
    {
        /*go to sleep*/
        sleepRandom();

        /*write to file*/
        msg = "";
        msg = msg.append(
            "Counter Thread " + std::to_string(args->threadNumber) + " : recieved a message.\n");
        *args->fout << msg;

        msg = "";
        msg = msg.append(
            "Counter Thread " + std::to_string(args->threadNumber) + " : waiting to write.\n");
        *args->fout << msg;

        sem_wait(args->semaphore);
        /*critical section*/ {
            /*write to counter args.counter and*/
            currentCounterValue = ++(*args->counter);
            /*write to file args.fin current value of counter*/
            msg = "";
            msg = msg.append(
                "Counter Thread " + std::to_string(args->threadNumber) + " : now adding to counter,\n" +
                "counter value = " + std::to_string(currentCounterValue) + "\n");
            *args->fout << msg;
        }

        sem_post(args->semaphore);
    }
}

void *mMonitor(void *argsptr)
{
    /*extracting arguments*/
    const monitorArgs *args = (monitorArgs *)argsptr;

    unsigned int currentCounterValue;
    std::string msg;

    while (!*args->maxTime)
    {
        /*go to sleep*/
        sleepRandom();

        /*write to file*/
        msg = "";
        msg = msg.append(
            "Monitor Thread : waiting to write.\n");
        *args->fout << msg;

        sem_wait(args->semaphore);
        /*critical section*/ {
            /*write to counter args.counter and*/
            currentCounterValue = (*args->counter);
            *args->counter = 0;
            /*write to file args.fin current value of counter*/
            msg = "";
            msg = msg.append(
                "Monitor Thread now resetting counter,\nprevious counter value = " + std::to_string(currentCounterValue) + "\n");
            *args->fout << msg;
        }

        sem_post(args->semaphore);
    }
}

/*void *mMonitor(void *argsptr)
{
    monitorArgs *args = (monitorArgs *)argsptr;

    
    unsigned int counterValue;
    //std::string msg = "";
    int n = 0;
    while (n < 1)
    {
        n++;
        /*go to sleep*//*
        sleepRandom();

        *args->fout << "Monitor Thread: waiting to read counter.\n";
        sem_wait(args->semaphore);
        /*critical section*//* {
            counterValue = *args->counter;
            *args->counter = 0;
            /*msg = "";
            msg = msg.append(
                "Monitor Thread: reading a count value of " +
                std::to_string(counterValue) +
                "\n");
                *//*
            *args->fout << counterValue;
        }
        sem_post(args->semaphore);
    }
}*/

/*create n mCounter threads*/
pthread_t * createCounters(
    sem_t * semaphore,
    unsigned int * counter,
    std::ofstream * fout,
    unsigned short * maxTime,
    const unsigned int n){

    pthread_t * tid = new pthread_t[n];

    counterArgs * counterArgsArray = new counterArgs[n];

    Timer timer;

    for (int i = 0; i < n; i++)
    {
        /*setting the parameters*/
        counterArgsArray[i].counter = counter;
        counterArgsArray[i].fout = fout;
        counterArgsArray[i].semaphore = semaphore;
        counterArgsArray[i].threadNumber = i;
        counterArgsArray[i].maxTime = maxTime;
    }

    for (int j = 0; j < n; j++)
    {

        pthread_create(&tid[j], nullptr, mCounter, &counterArgsArray[j]);
    }

    return tid;
}

pthread_t initMonitor(
    sem_t * semaphore,
    unsigned int * counter,
    std::ofstream * fout,
    unsigned short * maxTime,
    std::queue<unsigned int> buffer,
    sem_t * full,
    sem_t * empty){

    pthread_t tid;

    monitorArgs * args = new monitorArgs;

    Timer timer;


        /*setting the parameters*/
        args->counter = counter;
        args->fout = fout;
        args->semaphore = semaphore;
        args->maxTime = maxTime;

        pthread_create(&tid, nullptr, mMonitor, &args);

    return tid;
}

int main()
{
    sem_t * semaphore = new sem_t;
    sem_init(semaphore, 0, 1);

    std::ofstream * fout = new std::ofstream;
    fout->open("out.txt");

    unsigned int * count = new unsigned int;
    *count = 0;

    unsigned short * maxTime = new unsigned short;
    * maxTime = 0;
    Timer timer;

    initMonitor   (semaphore, count, fout, maxTime);

    //createCounters(semaphore, count, fout, maxTime, 1);

    while (timer.getTimeElapsed() < MAX_RUN_TIME)
        * maxTime = 0;
    * maxTime = 1;

    //fout->close();
    //mout->close();
    sem_destroy(semaphore);
    
    /*delete fout; delete mout; delete maxTime; delete count;*/

    return 0;
}