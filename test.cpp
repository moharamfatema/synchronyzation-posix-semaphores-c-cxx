#include "main.cpp"
#include <gtest/gtest.h>
#include <iostream>

TEST(TestUtil,TestTimer){
    Timer timer;
    sleep(5);
    float time = timer.getTimeElapsed()/1000; 
    EXPECT_NEAR(time,5,0.1);
}

TEST(TestCounters,TestCounter){
    const std::string outMessage = 
    "Counter Thread 1 : waiting to write. Counter Thread 1 : now adding to counter, counter value = 1  ";

    std::string fileOut, str;
    std::ifstream fin;

    sem_t semaphore;
    sem_init(&semaphore,0,1);
    std::ofstream fout;
    fout.open("out.txt");
    unsigned int count = 0;
    unsigned int threadNumber = 1;
    counterArgs args = counterArgs(semaphore,count,&fout,threadNumber);
    counter((void *)&args);
    fout.close();
    fin.open("out.txt");

    fileOut = "";
    while(fin){
        getline(fin,str);
        fileOut = fileOut.append(str + " ");
    }
    EXPECT_EQ(fileOut,outMessage);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}