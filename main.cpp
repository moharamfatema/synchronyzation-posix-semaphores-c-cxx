#include <queue>
#include <unistd.h>
#include <stdlib.h>
#include <chrono>
#include <semaphore.h>
#include <fstream>

#define MAX_SLEEP_TIME 20
#define MAX_RUN_TIME 100

/*assume the longest runtime is 1 second for now*/
/*we need a timer class*/
/*assume the longest sleep time is 0.1 millisecond*/

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

public:
    counterArgs()
    {
    }
    explicit counterArgs(
        sem_t *semaphore,
        unsigned int *counter,
        std::ofstream *fout,
        const unsigned int threadNumber)
    {
        this->counter = counter;
        this->fout = fout;
        this->semaphore = semaphore;
        this->threadNumber = threadNumber;
    }
};

/*fuction to be passed into pthread create for a new counter thread*/
void *mCounter(void *argsptr)
{
    /*extracting arguments*/
    counterArgs *args = (counterArgs *)argsptr;
    unsigned int currentCounterValue;
    std::string msg = "";
    while (1)
    {
        /*go to sleep*/
        srandom((int)std::chrono::high_resolution_clock::now().time_since_epoch().count());
        usleep(random() % MAX_SLEEP_TIME);

        /*write to file*/
        msg = "";
        msg = msg.append(
            "Counter Thread " + std::to_string(args->threadNumber) + " : recieved a message.\n");
        *args->fout << msg;
        msg = "";

        msg = msg.append(
            "Counter Thread " + std::to_string(args->threadNumber) + " : waiting to write.\n");
        *args->fout << msg;
        msg = "";

        sem_wait(args->semaphore);
        /*critical section*/ {
            /*write to counter args.counter and*/
            currentCounterValue = ++(*args->counter);
            /*write to file args.fin current value of counter*/
            msg = msg.append(
                "Counter Thread " + std::to_string(args->threadNumber) + " : now adding to counter,\n" + "counter value = " + std::to_string(currentCounterValue) + "\n");
            *args->fout << msg;
        }

        sem_post(args->semaphore);
    }
}

/*create n mCounter threads*/
int createCounters(sem_t *semaphore, unsigned int *counter, const unsigned int n, std::ofstream *fout)
{

    pthread_t tid[n];

    counterArgs counterArgsArray[n];

    Timer timer;

    for (int i = 0; i < n; i++)
    {
        /*setting the parameters*/
        counterArgsArray[i].counter = counter;
        counterArgsArray[i].fout = fout;
        counterArgsArray[i].semaphore = semaphore;
        counterArgsArray[i].threadNumber = i;
    }

    for (int j = 0; j < n; j++)
    {

        pthread_create(&tid[j], nullptr, mCounter, &counterArgsArray[j]);
    }

    
    while(timer.getTimeElapsed() < MAX_RUN_TIME);
    *fout << "\n---Exiting---";
    /*(*fout).close();
    */

    return 0;
}

