#include <set>
#include <vector>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "NetWork.h"
#include "Commander.h"

uint32_t ALL_RESNUMBER = 0;
uint32_t MACHINE_RESNUMBER = 0;
uint32_t RES_NUMBER = 0;

void easylogging_congfigure(el::Level level)
{
    // Prevent Fatal level logs from interrupting the program
    el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
    // Set Hierarchical Logging
    el::Loggers::addFlag(el::LoggingFlag::HierarchicalLogging);
    // Set the level threshold, and modify a parameter to control the log output
    el::Loggers::setLoggingLevel(level);
}
uint32_t now_ms2()
{
    timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}
void cmdloop(Commander &cmd)
{
    cmd.loop();
}
void NTO()
{
    easylogging_congfigure(el::Level::Info);
    NetWork network;
    int start = now_ms2();
    Commander cmd(network);
    std::thread cmd_th(std::bind(&Commander::loop, &cmd));
    network.Run_NTO(true);
    int end = now_ms2();
    std::cout << "Total time is: " << end - start << " ms" << std::endl;
    network.ALL_TO_DB();

    cmd.stop();
    cmd_th.join();
}
void NTO_LCL()
{
    easylogging_congfigure(el::Level::Unknown);
    NetWork network;
    int start = now_ms2();
    network.Run_NTO_LCL(true);
    int end = now_ms2();
    std::cout << "Total time is: " << end - start << " ms" << std::endl;
    network.ALL_TO_DB();
}
void Auto_Done()
{
    //https://github.com/amrayn/easyloggingpp
    //Unknown  Only applicable to hierarchical logging and is used to turn off logging completely.
    easylogging_congfigure(el::Level::Unknown);
    NetWork network;
    network.Run_Auto_Done(true);
    network.ALL_TO_DB();
    std::cout << "Game Over!!!" << std::endl;
}
void Auto_Done_cmd()
{
    easylogging_congfigure(el::Level::Unknown);
    NetWork network;
    Commander cmd(network);
    std::thread cmd_th(std::bind(&Commander::loop, &cmd));
    network.Run_Auto_Done(true);
    network.ALL_TO_DB();
    std::cout << "Game Over!!!" << std::endl;
    cmd.stop();
    cmd_th.join();
}
void Auto()
{
    easylogging_congfigure(el::Level::Unknown);
    NetWork network;
    int start = now_ms2();
    network.Run_Auto(true);
    int end = now_ms2();
    std::cout << "Total time is: " << end - start << " ms" << std::endl;
    network.ALL_TO_DB();
}
void Auto_cmd()
{
    easylogging_congfigure(el::Level::Info);
    NetWork network;
    int start = now_ms2();
    // Commander cmd(network);
    // std::thread cmd_th(std::bind(&Commander::loop, &cmd));
    network.Run_Auto2(true);
    int end = now_ms2();
    std::cout << "Total time is: " << end - start << " ms" << std::endl;
    // cmd.stop();
    // cmd_th.join();
    network.ALL_TO_DB();
}
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "./LCL resource_number" << std::endl;
        return -1;
    }

    RES_NUMBER = strtoul(argv[1], NULL, 10);
    //std::cout << res_number << std::endl;
    //sscanf(argv[1], "%u", &res_number);
    ALL_RESNUMBER = ALL_TRANSNUMBER * RES_NUMBER;
    //std::cout << ALL_RESNUMBER << std::endl;
    MACHINE_RESNUMBER = (uint32_t)(ALL_RESNUMBER / MACHINE_NUMBER);
    //std::cout << MACHINE_RESNUMBER << std::endl;

    Auto_Done();
    return 0;
}
