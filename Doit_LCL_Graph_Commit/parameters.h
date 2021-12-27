#ifndef DEADLOCK_PARAMETERS_H
#define DEADLOCK_PARAMETERS_H
#include <stdint.h>

//Unit: ms
//In order to facilitate adjustment of the cycle settings.
const uint32_t x = 6*2; 
const uint32_t interval=0; 
//LCLP
const uint32_t LCLP_start_time = 0;
const uint32_t LCLP_AllTime = 100 * x;
//LCLS
const uint32_t LCLS_start_time = interval*x + LCLP_AllTime; 
const uint32_t LCLS_AllTime = 100 * x;         
//Detect
const uint32_t Detect_start_time = interval * x * 2 + LCLP_AllTime + LCLS_AllTime; 
const uint32_t Detect_AllTime = 20 * x;         
//Cycle time
const uint32_t T = LCLP_AllTime + LCLS_AllTime + Detect_AllTime + interval * x * 3;

//Total number of transaction processors
const uint32_t ALL_TRANSNUMBER = 127000;
//Total number of resources
extern uint32_t ALL_RESNUMBER;

//Machine configuration
const uint32_t MACHINE_NUMBER = 127;
//The number of transactions on a machine
const uint32_t MACHINE_TRANSNUMBER = ALL_TRANSNUMBER / MACHINE_NUMBER; 

extern uint32_t MACHINE_RESNUMBER;

//Transaction processor processing thread
const uint32_t TRANSACTION_THREAD_NUMBER = 80;
const uint32_t TRANSACTION_NUMBER_TRANS_THREAD=ALL_TRANSNUMBER/TRANSACTION_THREAD_NUMBER;


//Deadlock detection
//Number of deadlock detection threads
const uint32_t DEADLOCK_THREAD_NUMBER = 15;
//The number of transaction processors that each deadlock thread detects 
const uint32_t TRANSACTION_NUMBER_DEADLOCK = ALL_TRANSNUMBER/DEADLOCK_THREAD_NUMBER;


//SQL related parameters
//Ratio of read and write SQL
const uint32_t SQL_READ_PERCENT = 50;
//The average number of SQL is set to 5, 10, 20, 50, 100.
//SQL_MEAN for normal and exponential distribution
const double SQL_MEAN = 30;
const uint32_t SQL_STDDEV = 10;
const uint32_t SQL_MAX = 50;
const uint32_t SQL_MIN = 10;

//Resource requests follow normal distribution
//RESOURCE_MEAN for normal and exponential distribution
const double RESOURCE_MEAN = 1.2;
const double RESOURCE_STDDEV = 0.65;
const uint32_t RESOURCE_MAX = 5;
const uint32_t RESOURCE_MIN = 1;

//Deadlock detection threshold
const uint32_t DETECTION_THRESHOLD = 5000;

//Transaction timeout time 
//After the timeout, the transaction processor no longer generates new transactions.
const uint32_t TIME_OUT = 300000;
//No matter whether the transaction is completed or not, the system is stopped.
const uint32_t FINAL_TIME_OUT = 5000000;
#endif //DEADLOCK_PARAMETERS_H