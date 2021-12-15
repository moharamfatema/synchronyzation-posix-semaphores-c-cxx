#include <queue>
#include <unistd.h>
#include <stdlib.h>
#include <chrono>
#include <semaphore.h>
#include <fstream>

#define MAX_SLEEP_TIME 1000

/*assume the longest runtime is 1 second for now*/
/*we need a timer class*/
/*assume the longest sleep time is 0.1 millisecond*/

/*to time any function*/
class Timer
{

    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::duration<float> duration;
    public:
    Timer(){
         start = std::chrono::high_resolution_clock::now();
    }
    
    float getTimeElapsed(){
        auto end  = std::chrono::high_resolution_clock::now();
        duration = end - start;
        
        return duration.count()*1000.00f;
    }
};

struct counterArgs{
    sem_t semaphore;
    unsigned int counter;
    std::ofstream * fout;
    unsigned int threadNumber;
    
    public:
    counterArgs(
        sem_t semaphore,
        unsigned int counter,
        std::ofstream * fout,
        const unsigned int threadNumber
        ){
        this->counter = counter;
        this->fout = fout;
        this->semaphore = semaphore;
        this->threadNumber = threadNumber;
    }
};


void counter(void * argsptr){
    /*extracting arguments*/
    counterArgs * args = (counterArgs *) argsptr;
    
    
    /*go to sleep*/
    srandom((int)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    usleep(random()%MAX_SLEEP_TIME);
    
    /*write to file*/
    *args->fout 
        << "Counter Thread "
        <<args->threadNumber
        <<" : recieved a message.\n";

    
    *args->fout 
        << "Counter Thread "
        <<args->threadNumber
        <<" : waiting to write.\n";

    sem_wait(&args->semaphore);
    /*critical section*/{
        /*write to counter args.counter*/
        args->counter++;
        /*write to file args.fin current value of counter*/
        *args->fout 
            << "Counter Thread "
            << args->threadNumber
            <<" : now adding to counter,\n"
            <<"counter value = "
            <<args->counter
            <<"\n";
    }

    /*end critical section*/
    sem_post(&args->semaphore);



}
