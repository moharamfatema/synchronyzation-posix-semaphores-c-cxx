#include <queue>
#include <unistd.h>
#include <stdlib.h>
#include <chrono>
#include <semaphore.h>
#include <fstream>

#define MAX_SLEEP_TIME 1000
#define MAX_RUN_TIME 100
#define SIZE_OF_BUFFER 50
#define NO_OF_COUNTER_THREADS 7



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
    sem_t *semaphore, *filesem, *timesem;
    unsigned int *counter;
    std::ofstream *fout;
    unsigned int threadNumber;
    unsigned short *maxTime;

};

struct monitorArgs
{
    sem_t *semaphore, *buffersem, *empty, *full, *filesem, *timesem;
    unsigned int *counter;
    std::ofstream *fout;
    unsigned short *maxTime;
    std::queue<unsigned int> *buffer;
};

struct collectorArgs
{
    sem_t *buffersem, *full, *empty, *filesem ,*timesem;
    std::queue<unsigned int> *buffer;
    unsigned short *maxTime;
    std::ofstream *fout;
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

    unsigned short flag;
    sem_wait(args->timesem);
    flag = *args->maxTime;
    sem_post(args->timesem);
    while (!flag)
    {
        /*go to sleep*/
        sleepRandom();

        /*write to file*/
        msg = "";
        msg = msg.append(
            "Counter Thread " + std::to_string(args->threadNumber) + " : recieved a message.\n");
        sem_wait(args->filesem);
        *args->fout << msg;
        sem_post(args->filesem);

        msg = "";
        msg = msg.append(
            "Counter Thread " + std::to_string(args->threadNumber) + " : waiting to write.\n");
        sem_wait(args->filesem);
        *args->fout << msg;
        sem_post(args->filesem);

        sem_wait(args->semaphore);
        /*critical section*/ {
            /*write to counter args.counter and*/
            currentCounterValue = ++(*args->counter);
            /*write to file args.fin current value of counter*/
            msg = "";
            msg = msg.append(
                "Counter Thread " + std::to_string(args->threadNumber) + " : now adding to counter,\n" +
                "counter value = " + std::to_string(currentCounterValue) + "\n");
        }

        sem_post(args->semaphore);

        sem_wait(args->filesem);
        *args->fout << msg;
        sem_post(args->filesem);

        sem_wait(args->timesem);
        flag = *args->maxTime;
        sem_post(args->timesem);
    }
    pthread_exit(nullptr);

}

void *mMonitor(void *argsptr)
{
    /*extracting arguments*/
    const monitorArgs *args = (monitorArgs *)argsptr;

    unsigned int currentCounterValue;
    std::string msg;

    unsigned short flag;
    sem_wait(args->timesem);
    flag = *args->maxTime;
    sem_post(args->timesem);
    while (!flag)
    {
        /*go to sleep*/
        sleepRandom();

        /*write to file*/
        msg = "";
        msg = msg.append(
            "Monitor Thread : waiting to read counter.\n");
        sem_wait(args->filesem);
        *args->fout << msg;
        sem_post(args->filesem);

        /*produce*/
        sem_wait(args->semaphore);
        /*critical section*/ {
            /*write to counter args.counter and*/
            currentCounterValue = (*args->counter);
            *args->counter = 0;
            /*write to file args.fin current value of counter*/
            msg = "";
            msg = msg.append(
                "Monitor Thread:reading a count value of " + std::to_string(currentCounterValue) + "\n");
        }

        sem_post(args->semaphore);

        sem_wait(args->filesem);
        *args->fout << msg;
        sem_post(args->filesem);

        /*append to buffer*/
        sem_wait(args->empty);
        sem_wait(args->buffersem);
        args->buffer->push(currentCounterValue);
        msg = "";
        msg = msg.append(
            "Monitor thread: writing to buffer at position" + std::to_string(args->buffer->size() - 1) + "\n");
        sem_post(args->buffersem);
        sem_post(args->full);

        sem_wait(args->filesem);
        *args->fout << msg;
        sem_post(args->filesem);

        sem_wait(args->timesem);
        flag = *args->maxTime;
        sem_post(args->timesem);
    

    }
    pthread_exit(nullptr);
}

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
        }/*end critical section*/
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

/*create n mCounter threads*/
pthread_t *createCounters(
    sem_t *semaphore,
    unsigned int *counter,
    std::ofstream *fout,
    unsigned short *maxTime,
    const unsigned int n,
    sem_t * filesem,
    sem_t * timesem)
{

    pthread_t *tid = new pthread_t[n];

    counterArgs *counterArgsArray = new counterArgs[n];

    for (int i = 0; i < n; i++)
    {
        /*setting the parameters*/
        counterArgsArray[i].counter = counter;
        counterArgsArray[i].fout = fout;
        counterArgsArray[i].semaphore = semaphore;
        counterArgsArray[i].threadNumber = i;
        counterArgsArray[i].maxTime = maxTime;
        counterArgsArray[i].filesem = filesem;
        counterArgsArray[i].timesem = timesem;

    }

    for (int j = 0; j < n; j++)
    {

        pthread_create(&tid[j], nullptr, mCounter, &counterArgsArray[j]);
    }

    return tid;
}

pthread_t initMonitor(
    sem_t *semaphore,
    unsigned int *counter,
    std::ofstream *fout,
    unsigned short *maxTime,
    std::queue<unsigned int> *buffer,
    sem_t *buffersem,
    sem_t *full,
    sem_t *empty,
    sem_t *filesem,
    sem_t * timesem)
{

    pthread_t tid;

    monitorArgs *args = new monitorArgs;

    /*setting the parameters*/
    args->counter = counter;
    args->fout = fout;
    args->semaphore = semaphore;
    args->maxTime = maxTime;
    args->buffer = buffer;
    args->buffersem = buffersem;
    args->empty = empty;
    args->full = full;
    args->filesem = filesem;
    args->timesem = timesem;

    pthread_create(&tid, nullptr, mMonitor, &args);

    return tid;
}

pthread_t initCollector(
    sem_t * buffersem,
    sem_t * full,
    sem_t * empty,
    std::queue<unsigned int> *buffer,
    std::ofstream * fout,
    unsigned short * maxTime,
    sem_t * filesem,
    sem_t * timesem
)
{
    collectorArgs * args= new collectorArgs;
    pthread_t tid;

    /*setting the parameters*/
    args->maxTime = maxTime;
    args->buffer = buffer;
    args->buffersem = buffersem;
    args->empty = empty;
    args->full = full;
    args->fout = fout;
    args->filesem = filesem;

    pthread_create(&tid, nullptr, mMonitor, &args);

    return tid;
}

int main()
{
    sem_t *semaphore = new sem_t;
    sem_init(semaphore, 0, 1);

    sem_t *buffersem = new sem_t;
    sem_init(buffersem, 0, 1);

    sem_t *empty = new sem_t;
    sem_init(empty, 0, SIZE_OF_BUFFER);

    sem_t *full = new sem_t;
    sem_init(full, 0, 0);

    sem_t * filesem = new sem_t;
    sem_init(filesem,0,1);

    sem_t * timesem = new sem_t;
    sem_init(timesem,0,1);

    std::ofstream *fout = new std::ofstream;
    fout->open("out.txt");

    unsigned int *count = new unsigned int;
    *count = 0;

    unsigned short *maxTime = new unsigned short;
    *maxTime = 0;

    Timer timer;

    std::queue<unsigned int> *buffer = new std::queue<unsigned int>;

    pthread_t * counterTid  = new pthread_t[NO_OF_COUNTER_THREADS];
    counterTid = createCounters(semaphore, count, fout, maxTime, NO_OF_COUNTER_THREADS,filesem, timesem);

    pthread_t monitorTid = initMonitor   (semaphore, count, fout, maxTime, buffer, buffersem, full, empty, filesem, timesem);

    //pthread_t collectorTid = initCollector (buffersem, full, empty, buffer, fout, maxTime, filesem, timesem);

    /*monitor the running time*/
    while (timer.getTimeElapsed() < MAX_RUN_TIME);
    sem_wait(timesem);
    *maxTime = 1;
    sem_post(timesem);

    /*join threads*/
    for(int i = 0; i < NO_OF_COUNTER_THREADS; i++){
        pthread_join(counterTid[i],0);
    }
    pthread_join(monitorTid,0);
    //pthread_join(collectorTid,0);

    /*cleanup*/
    fout->close();

    sem_destroy(semaphore);
    sem_destroy(buffersem);
    sem_destroy(full);
    sem_destroy(empty);
    sem_destroy(filesem);
    sem_destroy(timesem);

    delete fout;
    delete maxTime;
    delete count;
    delete buffer;
    delete[] counterTid;

    return 0;
}