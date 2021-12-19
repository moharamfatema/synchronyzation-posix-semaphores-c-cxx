#include <queue>
#include <unistd.h>
#include <stdlib.h>
#include <chrono>
#include <semaphore.h>
#include <fstream>

#define MAX_SLEEP_TIME 1
#define MAX_RUN_TIME 1
#define SIZE_OF_BUFFER 100
#define NO_OF_COUNTER_THREADS 1

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
    static sem_t 
        *semaphore, 
        *filesem;
    static unsigned int *counter;
    static std::ofstream *fout;
    static pthread_t *mCountertidArray;
    static unsigned int *threadNoArr;

    static void init()
    {
        mCountertidArray = new pthread_t[NO_OF_COUNTER_THREADS];
        threadNoArr = new unsigned int [NO_OF_COUNTER_THREADS];

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

        while (1)
        {
            /*go to sleep*/
            sleepRandom();

            /*write to file*/
            msg = "";
            msg = msg.append(
                "Counter Thread " + std::to_string(*threadNo) + " : recieved a message.\n");
            sem_wait(filesem);
            *fout << msg;
            sem_post(filesem);

            msg = "";
            msg = msg.append(
                "Counter Thread " + std::to_string(*threadNo) + " : waiting to write.\n");
            sem_wait(filesem);
            *fout << msg;
            sem_post(filesem);

            sem_wait(semaphore);
            /*critical section*/ {
                /*write to counter args.counter and*/
                currentCounterValue = ++(*counter);
                /*write to file args.fin current value of counter*/
                msg = "";
                msg = msg.append(
                    "Counter Thread " + std::to_string(*threadNo) + " : now adding to counter,\n" +
                    "counter value = " + std::to_string(currentCounterValue) + "\n");
            }

            sem_post(semaphore);

            sem_wait(filesem);
            *fout << msg;
            sem_post(filesem);
        }
        pthread_exit(nullptr);
    }

    static pthread_t *getmCountertidArray()
    {
        return mCountertidArray;
    }

    ~MCounters(){
        delete[] mCountertidArray;
        delete[] threadNoArr;
    }
};

class MMonitor
{
public:
    static sem_t
        *buffersem,
        *full,
        *empty,
        *semaphore,
        *filesem;
    static unsigned int *counter;
    static std::ofstream *fout;
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
        u_short fullFlag;

        while (1)
        {
            /*go to sleep*/
            sleepRandom();

            /*write to file*/
            msg = "";
            msg = msg.append(
                "Monitor Thread : waiting to read counter.\n");
            sem_wait(filesem);
            *fout << msg;
            sem_post(filesem);

            /*produce*/
            sem_wait(semaphore);
            /*critical section*/ {
                /*write to counter args.counter and*/
                currentCounterValue = (*counter);
                *counter = 0;
            }
            sem_post(semaphore);

            /*write to file args.fin current value of counter*/
            msg = "";
            msg = msg.append(
                "Monitor Thread : reading a count value of " + std::to_string(currentCounterValue) + "\n");

            sem_wait(filesem);
            *fout << msg;
            sem_post(filesem);

            /*append to buffer*/
            msg = "";
            sem_wait(buffersem);
            fullFlag = buffer->size() >= SIZE_OF_BUFFER;
            sem_post(buffersem);

            if (!fullFlag)
            {
                sem_wait(empty);
                sem_wait(buffersem);

                buffer->push(currentCounterValue);
                msg = msg.append(
                    "Monitor thread : writing to buffer at position " + std::to_string(buffer->size() - 1) + "\n");

                sem_post(buffersem);
                sem_post(full);
            }
            else
            {
                msg = msg.append(
                    "Monitor thread : Buffer full!\n");
            }
            sem_wait(filesem);
            *fout << msg;
            sem_post(filesem);
        }
        pthread_exit(nullptr);
    }
};

class MCollector
{
public:
    static sem_t
        *buffersem,
        *full,
        *empty,
        *filesem;
    static std::ofstream *fout;
    static pthread_t tid;
    static std::queue<unsigned int> *buffer;

    static void init()
    {
        pthread_create(&tid, nullptr, MCollector::mCollector, nullptr);
    }
    static void *mCollector(void *garbage)
    {
        unsigned int bufferVal;
        std::string msg;
        u_short emptyFlag;
        while (1)
        {
            /*go to sleep*/
            sleepRandom();

            /*take from buffer*/
            msg = "";

            sem_wait(buffersem);
            emptyFlag = buffer->empty();
            sem_post(buffersem);

            if (!emptyFlag)
            {
                sem_wait(full);
                sem_wait(buffersem);

                bufferVal = buffer->front();
                buffer->pop();

                sem_post(buffersem);
                sem_post(empty);

                msg = msg.append(
                    "Collector thread : reading from the buffer at position 0 \n");

            }
            else
            {
                msg = msg.append(
                    "Collector thread : nothing is in the buffer!\n");
            }

            sem_wait(filesem);
            *fout << msg;
            sem_post(filesem);


        }
        pthread_exit(nullptr);
    }
};

/*declaration of static members*/
unsigned int
    *MCounters::counter,
    *MMonitor::counter,
    *MCounters::threadNoArr;
sem_t
    *MCounters::filesem,
    *MMonitor::filesem,
    *MCollector::filesem,
    *MCounters::semaphore,
    *MMonitor::semaphore,
    *MMonitor::buffersem,
    *MCollector::buffersem,
    *MMonitor::empty,
    *MCollector::empty,
    *MMonitor::full,
    *MCollector::full;
std::ofstream
    *MCounters::fout,
    *MMonitor::fout,
    *MCollector::fout;

pthread_t
    *MCounters::mCountertidArray,
    MMonitor::tid,
    MCollector::tid;
std::queue<unsigned int>
    *MMonitor::buffer,
    *MCollector::buffer;

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

    sem_t *filesem = new sem_t;
    sem_init(filesem, 0, 1);

    std::ofstream fout;
    fout.open("out.txt");

    unsigned int *count = new unsigned int;
    *count = 0;

    Timer timer;

    std::queue<unsigned int> *buffer = new std::queue<unsigned int>;

    MCounters mCounters;
    MCollector mCollector;
    MMonitor mMonitor;

    MCounters::counter = MMonitor::counter = count;
    MCounters::filesem = MMonitor::filesem = MCollector::filesem = filesem;
    MCounters::fout = MMonitor::fout = MCollector::fout = &fout;
    MCounters::semaphore = MMonitor::semaphore = semaphore;
    MMonitor::buffersem = MCollector::buffersem = buffersem;
    MMonitor::empty = MCollector::empty = empty;
    MMonitor::full = MCollector::full = full;
    MMonitor::buffer = MCollector::buffer = buffer;

    mCounters.init();
    mMonitor.init();
    mCollector.init();

    /*monitor the running time*/
    while (timer.getTimeElapsed() < MAX_RUN_TIME)
        usleep(1+MAX_RUN_TIME/100);

    /*join threads*/
    for (int i = 0; i < NO_OF_COUNTER_THREADS; i++)
    {
        pthread_cancel(MCounters::getmCountertidArray()[i]);
        pthread_join(MCounters::getmCountertidArray()[i], 0);
    }

    pthread_cancel(MMonitor::tid);
    pthread_join(MMonitor::tid, 0);

    pthread_cancel(MCollector::tid);
    pthread_join(MCollector::tid, 0);

    /*cleanup*/
    fout.close();

    sem_destroy(semaphore);
    sem_destroy(buffersem);
    sem_destroy(full);
    sem_destroy(empty);
    sem_destroy(filesem);

    delete count,
        semaphore,
        filesem,
        buffersem,
        empty,
        full,
        buffer;

    return 0;
}