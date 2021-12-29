#ifndef NETWORK_H
#define NETWORK_H
#include "system.h"
#include <unistd.h>
#include <condition_variable>
#include <queue>
#include <map>
#include <random>
#include <unordered_map>
#include <set>
#include <memory>
#include <algorithm>

extern uint32_t ALL_RESNUMBER;
extern uint32_t MACHINE_RESNUMBER;
extern uint32_t RES_NUMBER;

struct DeadLockInformation{
    int DETECT_ID;
    int PREDECESSOR_ID;
    int PREDECESSOR_LCLV;
    std::string PREDECESSOR_PU_AP_ID;
    std::string PREDECESSOR_PR_AP_ID;    
    int SUCCESSOR_ID;
    int SUCCESSOR_LCLV;
    std::string SUCCESSOR_PU_AP_ID;
    std::string SUCCESSOR_PR_AP_ID;
    std::string DL_INFORMATION;
	int DETECT_TIME;
};

struct GlobalInformation{
    int TRANS_ID;
    std::string WAIT_IDS;
};

class NodeImpl;
class NetWork
{
public:
    NetWork();
    virtual ~NetWork();

    void LCLP_Trans_new(TransactionProcess *cur_trans, timespec deadline);
    void LCLS_Trans_new(TransactionProcess *cur_trans, timespec deadline,bool rollback);
    void Detect_Trans_new(TransactionProcess *cur_trans, timespec deadline,bool rollback);
    void DLO(TransactionProcess *cur_trans, int detect_id,int LCLV, std::pair<int, int> Pu_AP_ID,std::unordered_set<TransactionProcess *> &visited_set);
    bool DLO2(TransactionProcess *detect_trans);

    void Thread_Run_LCLP(int machine_id,timespec LCLP_deadline);
    void Thread_Run_LCLS(int machine_id,timespec LCLS_deadline);
    void Thread_Run_Detect(int machine_id,timespec LCLS_deadline);
    void Thread_Run_Help()
    {
        int notdone = INT32_MAX;
        while (!time_out_)
        {
            sleep(10);
        }

        while (!done_)
        {
            int running_count = 0;
            for (int i = 0; i < ALL_TRANSNUMBER; i++)
            {
                if (trans_pool_[i]->running)
                    running_count++;
            }
            if (running_count < notdone)
            {
                notdone = running_count;
            }
            else
            {
                pthread_rwlock_wrlock(&base_rwlock_);
                base += 2;
                pthread_rwlock_unlock(&base_rwlock_);
            }
            usleep(T*base*1000);
        }
    }
    void DeadlockDetect();

    //Constantly generate new transactions
    void Thread_Run_NTO(int trans_begin_id);
    void Run_NTO(bool deadlock_output);
    void Run_NTO_LCL(bool deadlock_output);

    /*No new transaction is generated and no 
    timeout is used to verify correctness
    */
    void RunTransactionProcess_Auto(int i);
    void RunTransactionProcess_Auto_Done(int i);
    void Thread_Run_Auto(int trans_begin_id);
    void Thread_Run_Auto_Done(int trans_begin_id);
    void Run_Auto(bool deadlock_output);
    void Run_Auto2(bool deadlock_output);
    void Run_Auto_Done(bool deadlock_output);

    //Deadlock detection call
    void Thread_Run_DeadLockDetect_Once(int machine_id,timespec DeadLock_start_time);
    void Thread_Run_DeadLockDetect_Once_Doit(int machine_id,timespec DeadLock_start_time,int x);
    void Thread_Run_DeadLockDetect_Periodic(int machine_id,timespec DeadLock_start_time);
    void Thread_Run_DeadLockDetect_Periodic2(int machine_id,timespec DeadLock_start_time);

    void PrintGraph();
    void LCL_Once(bool rollback);
    void LCL_Once_Doit(int x);
    void LCL_Periodic();

    uint32_t GetAllTime_ms();
    void ALL_TO_DB();
    void ALL_TO_DB_easy();
public:
    //When the transaction is completed, the deadlock detection is notified.
    std::atomic<bool> done_{false};
    std::atomic<bool> time_out_{false};
    std::vector<std::thread> DeadLock_workers;
private:
    bool deadlock_output_;
    timespec start_time_point_;

    std::atomic<int> global_id_;
    std::atomic<int> global_trans_id_;
    std::atomic<int> AP_pool_[MACHINE_NUMBER];
    std::vector<TransactionProcess *> trans_pool_;
    std::vector<Resource *> resource_pool_;

    std::vector<std::thread> workers;

    std::mutex myMutex_;
    std::condition_variable condition_;
    std::vector<DeadLockInformation> deadlock_informations_;
    std::mutex deadlock_informations_mu_;
    std::vector<std::pair<int,int>> deadlock_loops_;
    std::mutex deadlock_loops_mu_;

    int base;
    pthread_rwlock_t base_rwlock_;

};
#endif
