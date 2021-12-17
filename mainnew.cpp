#include <queue>
#include <unistd.h>
#include <stdlib.h>
#include <chrono>
#include <semaphore.h>
#include <fstream>

#define MAX_SLEEP_TIME 10
#define MAX_RUN_TIME 100
#define SIZE_OF_BUFFER 50
#define NO_OF_COUNTER_THREADS 1

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

void sleepRandom()
{
    /*go to sleep*/
    srandom((int)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    usleep(random() % MAX_SLEEP_TIME);
}

class MCounters
{

public:
    static sem_t semaphore, filesem, timesem;
    static unsigned int *counter;
    static std::ofstream *fout;
    static unsigned short *maxTime;
    static pthread_t mCountertidArray[NO_OF_COUNTER_THREADS];

    static void init()
    {
        unsigned int *threadNoArr = new unsigned int[NO_OF_COUNTER_THREADS];
        for (int j = 0; j < NO_OF_COUNTER_THREADS; j++)
        {
            threadNoArr[j] = j;
            pthread_create(&mCountertidArray[j], nullptr, MCounters::mCounter, &threadNoArr[j]);
        }
    }

    static void *mCounter(void *threadNoptr)
    {
        /*extracting arguments*/
        const unsigned int *threadNo = (unsigned int *)threadNoptr;

        unsigned int currentCounterValue;
        std::string msg;

        unsigned short flag;
        sem_wait(&timesem);
        flag = *maxTime;
        sem_post(&timesem);
        while (!flag)
        {
            /*go to sleep*/
            sleepRandom();

            /*write to file*/
            msg = "";
            msg = msg.append(
                "Counter Thread " + std::to_string(*threadNo) + " : recieved a message.\n");
            sem_wait(&filesem);
            *fout << msg;
            sem_post(&filesem);

            msg = "";
            msg = msg.append(
                "Counter Thread " + std::to_string(*threadNo) + " : waiting to write.\n");
            sem_wait(&filesem);
            *fout << msg;
            sem_post(&filesem);

            sem_wait(&semaphore);
            /*critical section*/ {
                /*write to counter args.counter and*/
                currentCounterValue = ++(*counter);
                /*write to file args.fin current value of counter*/
                msg = "";
                msg = msg.append(
                    "Counter Thread " + std::to_string(*threadNo) + " : now adding to counter,\n" +
                    "counter value = " + std::to_string(currentCounterValue) + "\n");
            }

            sem_post(&semaphore);

            sem_wait(&filesem);
            *fout << msg;
            sem_post(&filesem);

            sem_wait(&timesem);
            flag = *maxTime;
            sem_post(&timesem);
        }
        pthread_exit(nullptr);
    }

    static pthread_t *getmCountertidArray()
    {
        return mCountertidArray;
    }
};

class MMonitor
{
public:
    static sem_t buffersem, full, empty, semaphore, filesem, timesem;
    static unsigned int *counter;
    static std::ofstream *fout;
    static unsigned short *maxTime;
    static pthread_t tid;
    static std::queue<unsigned int> *buffer;

    static void init()
    {
        pthread_create(&tid, nullptr, MMonitor::mMonitor, nullptr);
    }
    static void *mMonitor(void *garbage)
    {
        unsigned int currentCounterValue;
        std::string msg;

        unsigned short flag;
        sem_wait(&timesem);
        flag = *maxTime;
        sem_post(&timesem);
        while (!flag)
        {
            /*go to sleep*/
            sleepRandom();

            /*write to file*/
            msg = "";
            msg = msg.append(
                "Monitor Thread : waiting to read counter.\n");
            sem_wait(&filesem);
            *fout << msg;
            sem_post(&filesem);

            /*produce*/
            sem_wait(&semaphore);
            /*critical section*/ {
                /*write to counter args.counter and*/
                currentCounterValue = (*counter);
                *counter = 0;
                /*write to file args.fin current value of counter*/
                msg = "";
                msg = msg.append(
                    "Monitor Thread : reading a count value of " + std::to_string(currentCounterValue) + "\n");
            }

            sem_post(&semaphore);

            sem_wait(&filesem);
            *fout << msg;
            sem_post(&filesem);

            /*append to buffer*/
            sem_wait(&empty);
            sem_wait(&buffersem);
            buffer->push(currentCounterValue);
            msg = "";
            msg = msg.append(
                "Monitor thread : writing to buffer at position " + std::to_string(buffer->size() - 1) + "\n");
            sem_post(&buffersem);
            sem_post(&full);

            sem_wait(&filesem);
            *fout << msg;
            sem_post(&filesem);

            sem_wait(&timesem);
            flag = *maxTime;
            sem_post(&timesem);
        }
        pthread_exit(nullptr);
    }
};

struct collectorArgs
{
    sem_t *buffersem, *full, *empty, *filesem, *timesem;
    std::queue<unsigned int> *buffer;
    unsigned short *maxTime;
    std::ofstream *fout;
};

void *mCollector(void *argsptr)
{
    /*extracting arguments*/
    const collectorArgs *args = (collectorArgs *)argsptr;

    unsigned int bufferVal;
    std::string msg = "";

    unsigned short flag;
    sem_wait(args->timesem);
    flag = *args->maxTime;
    sem_post(args->timesem);
    while (!flag)
    {
        /*go to sleep*/
        sleepRandom();

        /*consume*/

        sem_wait(args->full);
        sem_wait(args->buffersem);
        /*critical section*/ {
            if (!args->buffer->empty())
            {
                bufferVal = args->buffer->front();
                args->buffer->pop();
                msg = "Collector Thread: reading from the buffer at position 0\n";
            }
            else
            {
                msg = "Collector Thread: Nothing is in the buffer!\n";
            }
        } /*end critical section*/
        sem_post(args->buffersem);
        sem_post(args->empty);

        sem_wait(args->filesem);
        *args->fout << msg;
        sem_post(args->filesem);

        sem_wait(args->timesem);
        flag = *args->maxTime;
        sem_post(args->timesem);
    }
    pthread_exit(nullptr);
}


pthread_t initCollector(
    sem_t *buffersem,
    sem_t *full,
    sem_t *empty,
    std::queue<unsigned int> *buffer,
    std::ofstream *fout,
    unsigned short *maxTime,
    sem_t *filesem,
    sem_t *timesem)
{
    collectorArgs *args = new collectorArgs;
    pthread_t tid;

    /*setting the parameters*/
    args->maxTime = maxTime;
    args->buffer = buffer;
    args->buffersem = buffersem;
    args->empty = empty;
    args->full = full;
    args->fout = fout;
    args->filesem = filesem;

    pthread_create(&tid, nullptr, mCollector, &args);

    return tid;
}

unsigned int *MCounters::counter, *MMonitor::counter;
sem_t MCounters::filesem, MMonitor::filesem;
sem_t MCounters::timesem, MMonitor::timesem;
unsigned short *MCounters::maxTime , *MMonitor::maxTime;
std::ofstream *MCounters::fout , *MMonitor::fout;
sem_t MCounters::semaphore, MMonitor::semaphore, MMonitor::buffersem, MMonitor::empty, MMonitor::full;
pthread_t MCounters::mCountertidArray[NO_OF_COUNTER_THREADS], MMonitor::tid;
std::queue<unsigned int> * MMonitor::buffer;

int main()
{
    sem_t semaphore;
    sem_init(&semaphore, 0, 1);

    sem_t buffersem;
    sem_init(&buffersem, 0, 1);

    sem_t empty;
    sem_init(&empty, 0, SIZE_OF_BUFFER);

    sem_t full;
    sem_init(&full, 0, 0);

    sem_t filesem;
    sem_init(&filesem, 0, 1);

    sem_t timesem;
    sem_init(&timesem, 0, 1);

    std::ofstream fout;
    fout.open("out.txt");

    unsigned int *count = new unsigned int;
    *count = 0;

    unsigned short maxTime = 0;

    Timer timer;

    std::queue<unsigned int> buffer;

    static MCounters mCounters;
    static MMonitor  mMonitor;

    MCounters::counter   = MMonitor::counter   = count;
    MCounters::filesem   = MMonitor::filesem   = filesem;
    MCounters::timesem   = MMonitor::timesem   = timesem;
    MCounters::maxTime   = MMonitor::maxTime   = &maxTime;
    MCounters::fout      = MMonitor::fout      = &fout;
    MCounters::semaphore = MMonitor::semaphore = semaphore;
    MMonitor::buffersem  = buffersem;
    MMonitor::empty      = empty;
    MMonitor::full       = full;
    MMonitor::buffer     = &buffer;

    mCounters.init();
    mMonitor .init();

    // pthread_t monitorTid = initMonitor   (semaphore, count, fout, maxTime, buffer, buffersem, full, empty, filesem, timesem);

    // pthread_t collectorTid = initCollector (buffersem, full, empty, buffer, fout, maxTime, filesem, timesem);

    /*monitor the running time*/
    while (timer.getTimeElapsed() < MAX_RUN_TIME)
        ;
    sem_wait(&timesem);
    maxTime = 1;
    sem_post(&timesem);

    /*join threads*/
    for (int i = 0; i < NO_OF_COUNTER_THREADS; i++)
    {
        pthread_join(MCounters::getmCountertidArray()[i], 0);
    }
    pthread_join(MMonitor::tid,0);
    // pthread_join(collectorTid,0);

    /*cleanup*/
    fout.close();

    sem_destroy(&semaphore);
    sem_destroy(&buffersem);
    sem_destroy(&full);
    sem_destroy(&empty);
    sem_destroy(&filesem);
    sem_destroy(&timesem);

    delete count;

    return 0;
}