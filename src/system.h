#ifndef SYSTEM428_SYSTEM_H
#define SYSTEM428_SYSTEM_H

#include "parameters.h"
#include "easylogging++.h"

#include <chrono>
#include <unordered_set>
#include <list>
#include <mutex>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <atomic>
#include <pthread.h>
#include <thread>
#include <random>
#include <algorithm>
#include <cmath>

extern uint32_t ALL_RESNUMBER;
extern uint32_t MACHINE_RESNUMBER;

inline bool IsSameMachine(int trans_id, int res_id)
{
    return trans_id / MACHINE_TRANSNUMBER == res_id / MACHINE_RESNUMBER;
}
struct  TransInformation
{
	int TRANS_ID;
	std::string LONG_OR_SHORT;
	std::string WRITE_OR_READ;
	int SQL_NUM;
	std::string SQL_INFORMATION;
	int WAIT_TIME;
	int TOTAL_TIME;
    int COMMIT_TIME;
    std::string SUCCESS;
};

struct SQL_Disposition
{
    uint32_t res_num;
    bool write;
    std::vector<uint32_t> acquire_resource_id;
};
class TransactionProcess;

struct  CommitTrans
{
    int COMMIT_TIME;
};

struct  Detect_time
{
    int DETECT_TIME;
};

class Resource
{
public:
    Resource(int res_id);

    bool Acquire(TransactionProcess *transactionProcess);

    void Release();
    void Release(TransactionProcess *transactionProcess);

    void Get_cur_trans(TransactionProcess *&transactionProcess);

public:
    int res_id_;

private:
    pthread_rwlock_t res_rwlock_;
    TransactionProcess *cur_trans_;
    std::atomic<int> cur_trans_id_;
};


class TransactionProcess
{
public:
    TransactionProcess(int transprocess_id,int trans_id, std::atomic<int> &global_id);

    ~TransactionProcess();

    void newTrans(int trans_id);

    void newTrans(int trans_id,std::atomic<int> &PrAP);

    void Acquire(Resource *resource);

    void Release();

    void StopWait();

    void Rollback();

    void Get_wait_res_set(std::unordered_set<Resource *> &res_set);

    void Get_successor_trans_set(std::unordered_set<TransactionProcess *> &successor_trans_set);

    void Run_Transaction_NTO(std::vector<Resource *> &resource_pool);

    void Run_Transaction_Auto(std::vector<Resource *> &resource_pool);

    void Run_SQL(std::vector<Resource *> &resource_pool);

    uint32_t GRandom(std::normal_distribution<double> *distribution, uint32_t max, uint32_t min);
    
    uint32_t GRandom(std::exponential_distribution<double> *distribution, uint32_t max, uint32_t min);

    uint32_t GetRunTime_ms();

    uint32_t GetAllTime_ms();

    void OutPut();

    bool wait_empty(){
        return wait_res_set_.empty();
    }
    void record_detect_time();

public:
    int transprocess_id_;
    int trans_id_;
    std::atomic<bool>running;
    std::atomic<int> LCLV_;
    std::pair<std::atomic<int>, std::atomic<int>> Pu_AP_ID_;
    std::pair<std::atomic<int>, std::atomic<int>> Pr_AP_ID_;
    int do_sql_id;
    std::atomic<int> do_lclp;
    std::atomic<int> do_lcls;
    std::atomic<int> do_detect;


    std::vector<uint32_t>commit_times_;
    std::vector<uint32_t>detect_times_;
    std::mutex detect_mu;
    timespec transaction_process_create_time_point_;
private:

    pthread_rwlock_t trans_rwlock_;
    timespec transaction_start_time_point_;
    std::unordered_set<Resource *> hold_res_set_;
    std::unordered_set<Resource *> wait_res_set_;
    
    //Generate random numbers
    std::random_device rd;
	std::mt19937 gen;
    std::default_random_engine e_;

    //normal distribution
    //https://www.cplusplus.com/reference/random/normal_distribution/
    //std::normal_distribution<double> *sql_number_distribution_;
    std::normal_distribution<double> *resource_distribution_;  

    //exponential_distribution
    //https://www.cplusplus.com/reference/random/exponential_distribution/
    std::exponential_distribution<double> *sql_number_distribution_;    
    //std::exponential_distribution<double> *resource_distribution_;
    
    int sql_num;
    std::vector<SQL_Disposition> trans_disposition;
};

#endif //SYSTEM428_SYSTEM_H
