#include "main.cpp"
#include <gtest/gtest.h>

/*TEST(TestUtil,TestTimer){
    Timer timer;
    sleep(5);
    float time = timer.getTimeElapsed()/1000;
    EXPECT_NEAR(time,5,0.1);
}*/

/*TEST(TestCounters,TestCounter){
    const std::string outMessage =
    "Counter Thread 1 : recieved a message. Counter Thread 1 : waiting to write. Counter Thread 1 : now adding to counter, counter value = 1  ";

    std::string fileOut, str;
    std::ifstream fin;

    sem_t semaphore;
    sem_init(&semaphore,0,1);
    std::ofstream fout;
    fout.open("out.txt");
    unsigned int count = 0;
    const unsigned int threadNumber = 1;
    counterArgs args = counterArgs(semaphore,count,&fout,threadNumber);
    mCounter((void *)&args);
    fout.close();

    exit(0);

    fin.open("out.txt");

    fileOut = "";
    while(fin){
        getline(fin,str);
        fileOut = fileOut.append(str + " ");
    }
    EXPECT_EQ(fileOut,outMessage);
}*/

TEST(TestCounters, TestCreateCounters)
{

    sem_t semaphore;
    sem_init(&semaphore, 0, 1);
    std::ofstream fout;
    fout.open("out.txt");
    unsigned int *count = new unsigned int;
    *count = 0;
    bool maxTime = false;
    Timer timer;
    if (fork() == 0)
    {
        createCounters(&semaphore, count, 5, &fout, maxTime);
        while (timer.getTimeElapsed() < MAX_RUN_TIME);
        maxTime = true;
        return;
    }
    wait(nullptr);
    fout.close();
    EXPECT_NEAR(timer.getTimeElapsed(),MAX_RUN_TIME,100);
    EXPECT_GE(timer.getTimeElapsed(),MAX_RUN_TIME);
}
TEST(TestMonitorCounters,workInHarmony){
    
    sem_t semaphore;
    sem_init(&semaphore, 0, 1);
    std::ofstream fout;
    fout.open("out2.txt");
    std::ofstream mout;
    mout.open("monitorOut.txt");

    unsigned int *count = new unsigned int;
    *count = 0;
    bool maxTime = false;
    Timer timer;
    if (fork() == 0)
    {
        //initMonitor(&semaphore,count,&fout,maxTime);

        createCounters(&semaphore, count, 1, &fout, maxTime);

        while (timer.getTimeElapsed() < MAX_RUN_TIME);
        maxTime = true;
        return;
    }
    wait(nullptr);
    fout.close();
    mout.close();
    EXPECT_NEAR(timer.getTimeElapsed(),MAX_RUN_TIME,100);
    EXPECT_GE(timer.getTimeElapsed(),MAX_RUN_TIME);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}