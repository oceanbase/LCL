#include "NetWork.h"
#include <mutex>
#include <algorithm>
#include <sys/time.h>
#include <sstream>

std::atomic<int> lclp_count[DEADLOCK_THREAD_NUMBER];
std::atomic<int> lclp_node[DEADLOCK_THREAD_NUMBER];
std::atomic<int> lclp_rounds[DEADLOCK_THREAD_NUMBER];
std::atomic<int> lcls_count[DEADLOCK_THREAD_NUMBER];
std::atomic<int> lcls_node[DEADLOCK_THREAD_NUMBER];
std::atomic<int> lcls_rounds[DEADLOCK_THREAD_NUMBER];
std::atomic<int> detect_count[DEADLOCK_THREAD_NUMBER];
std::atomic<int> detect_node[DEADLOCK_THREAD_NUMBER];
std::atomic<int> detect_rounds[DEADLOCK_THREAD_NUMBER];

uint32_t now_ms()
{
    timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

timespec TimeAdd(const timespec &time1, uint32_t time2_ms)
{
    timespec result;
    result.tv_nsec = time1.tv_nsec + (time2_ms % 1000) * 1000000;
    result.tv_sec = time1.tv_sec + time2_ms / 1000 + result.tv_nsec / 1000000000;
    result.tv_nsec = result.tv_nsec % 1000000000;
    return result;
}
bool operator<(const timespec &time1, const timespec &time2)
{
    if (time1.tv_sec < time2.tv_sec)
        return true;
    else if (time1.tv_sec > time2.tv_sec)
        return false;
    else
        return time1.tv_nsec < time2.tv_nsec;
}

NetWork::NetWork() : global_id_(0), global_trans_id_(0), base(1)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time_point_);
    pthread_rwlock_init(&base_rwlock_, NULL);
    srandom(start_time_point_.tv_nsec);
    trans_pool_.resize(ALL_TRANSNUMBER);
    LOG(INFO) << "ALL_TRANSNUMBER is: " << ALL_TRANSNUMBER;

    for (uint32_t i = 0; i < ALL_TRANSNUMBER; i++)
    {
        trans_pool_[i] = new TransactionProcess(i, global_trans_id_++, global_id_);
    }
    resource_pool_.resize(ALL_RESNUMBER);
    for (uint32_t i = 0; i < ALL_RESNUMBER; i++)
    {
        resource_pool_[i] = new Resource(i);
    }

    for (uint32_t i = 0; i < MACHINE_NUMBER; i++)
    {
        AP_pool_[i] = 0;
    }
    // workers.resize(TOTAL_THREAD_NUM);

    LOG(INFO) << "Constructed completely";
}

NetWork::~NetWork()
{
    LOG(DEBUG) << "destory NetWork";

    LOG(DEBUG) << "Start destructuring trans_run_pool";
    for (uint32_t i = 0; i < ALL_TRANSNUMBER; i++)
    {
        LOG(DEBUG) << "Release trans:" << trans_pool_[i]->trans_id_;

        delete trans_pool_[i];
        trans_pool_[i] = nullptr;
    }
    LOG(DEBUG) << "Released trans";

    for (uint32_t i = 0; i < ALL_RESNUMBER; i++)
    {
        delete resource_pool_[i];
        resource_pool_[i] = nullptr;
    }
    LOG(DEBUG) << "Released res";
}

void NetWork::LCLP_Trans_new(TransactionProcess *cur_trans, timespec deadline)
{
    // Recursive downstream nodes
    std::unordered_set<TransactionProcess *> successor_trans_set;
    cur_trans->Get_successor_trans_set(successor_trans_set);
    for (TransactionProcess *successor_trans : successor_trans_set)
    {
        lclp_count[cur_trans->transprocess_id_ / TRANSACTION_NUMBER_DEADLOCK]++;
        if (successor_trans->LCLV_ < cur_trans->LCLV_ + 1)
        {
            successor_trans->LCLV_ = cur_trans->LCLV_ + 1;
        }
    }
}

void NetWork::LCLS_Trans_new(TransactionProcess *cur_trans, timespec deadline, bool rollback)
{
    // Recursive downstream nodes
    std::unordered_set<TransactionProcess *> successor_trans_set;
    cur_trans->Get_successor_trans_set(successor_trans_set);
    for (TransactionProcess *successor_trans : successor_trans_set)
    {
        lcls_count[cur_trans->transprocess_id_ / TRANSACTION_NUMBER_DEADLOCK]++;
        if (successor_trans->LCLV_ < cur_trans->LCLV_)
        {
            successor_trans->LCLV_ = cur_trans->LCLV_.load();
        }
        if (successor_trans->LCLV_ == cur_trans->LCLV_)
        {
            if (successor_trans->Pu_AP_ID_ < cur_trans->Pu_AP_ID_)
            {
                successor_trans->Pu_AP_ID_.first = cur_trans->Pu_AP_ID_.first.load();
                successor_trans->Pu_AP_ID_.second = cur_trans->Pu_AP_ID_.second.load();
            }
        }
    }
}

void NetWork::Detect_Trans_new(TransactionProcess *cur_trans, timespec deadline, bool rollback)
{
    std::unordered_set<TransactionProcess *> successor_trans_set;
    cur_trans->Get_successor_trans_set(successor_trans_set);
    for (TransactionProcess *successor_trans : successor_trans_set)
    {
        detect_count[cur_trans->transprocess_id_ / TRANSACTION_NUMBER_DEADLOCK]++;
        if (successor_trans->LCLV_ == cur_trans->LCLV_)
        {
            if (successor_trans->Pu_AP_ID_ == successor_trans->Pr_AP_ID_ 
            && successor_trans->Pu_AP_ID_ == cur_trans->Pu_AP_ID_)
            {
                // std::cout << "-------DLO_Start Deadlock detected Trans is: " 
                // << successor_trans->trans_id_ << " --------"
                // " My upstream node: "<< cur_trans->trans_id_<<std::endl;
                // std::cout << "id:_" << cur_trans->trans_id_ << "_LCLV:" << cur_trans->LCLV_ 
                // << "_<" << cur_trans->Pu_AP_ID_.first << ","
                //    << cur_trans->Pu_AP_ID_.second << ">|<"
                //    << cur_trans->Pr_AP_ID_.first << "," << cur_trans->Pr_AP_ID_.second << ">_————>_id:_"
                //    << successor_trans->trans_id_ << "_LCLV:" << cur_trans->LCLV_ << "_<"
                //    << successor_trans->Pu_AP_ID_.first
                //    << "," << successor_trans->Pu_AP_ID_.second << ">|<"
                //    << successor_trans->Pr_AP_ID_.first << "," << successor_trans->Pr_AP_ID_.second << ">"<<std::endl;;

                std::unordered_set<TransactionProcess *> visited_set;
                if (deadlock_output_)
                {
                    DLO(successor_trans, successor_trans->trans_id_, successor_trans->LCLV_, 
                    std::make_pair(successor_trans->Pu_AP_ID_.first.load(), successor_trans->Pu_AP_ID_.second.load()), visited_set);
                    // ALL_TO_DB();
                    // std::cout<<"DLO Done"<<std::endl;
                    bool roll = DLO2(successor_trans);
                    if (roll)
                    {
                        successor_trans->Rollback();
                        std::cout << "-------DLO_Start Deadlock detected Trans is: " << successor_trans->trans_id_
                                  << " -------- My upstream node: "
                                  << cur_trans->trans_id_ << std::endl;
                        successor_trans->record_detect_time();
                    }
                }
                // if (rollback)
                // {
                //     successor_trans->Rollback();
                // }
            }
        }
    }
}

void NetWork::DLO(TransactionProcess *cur_trans, int detect_id, int LCLV, std::pair<int, int> Pu_AP_ID, 
                  std::unordered_set<TransactionProcess *> &visited_set)
{

    if (visited_set.count(cur_trans))
    {
        // std::cout<<"return trans id: "<<cur_trans->trans_id_<<std::endl;
        return;
    }
    visited_set.insert(cur_trans);
    // std::cout<<"DLO cur trans is:"<<cur_trans->trans_id_<< "_LCLV:" << cur_trans->LCLV_ 
    // << "_<" << cur_trans->Pu_AP_ID_.first << ","
    //                << cur_trans->Pu_AP_ID_.second << ">|<"
    //                << cur_trans->Pr_AP_ID_.first << "," << cur_trans->Pr_AP_ID_.second << ">_"<<std::endl;
    if (cur_trans->LCLV_ == LCLV && cur_trans->Pu_AP_ID_.first == Pu_AP_ID.first &&
        cur_trans->Pu_AP_ID_.second == Pu_AP_ID.second)
    {
        TransactionProcess *successor_trans;
        std::unordered_set<Resource *> wait_res_set;
        cur_trans->Get_wait_res_set(wait_res_set);
        // if(wait_res_set.empty()){
        //     std::cout<<"cur trans is: "<<cur_trans->trans_id_<<" waitres is null"<<std::endl;
        // }
        for (Resource *wait_res : wait_res_set)
        {
            successor_trans = nullptr;
            wait_res->Get_cur_trans(successor_trans);
            if (successor_trans == nullptr)
            {
                // std::cout<<"successtrans is null"<<std::endl;
                continue;
            }
            // std::cout<<"DLO successor_trans is:"<<successor_trans->trans_id_<< "_LCLV:" 
            // << successor_trans->LCLV_ << "_<" << successor_trans->Pu_AP_ID_.first << ","
            //        << successor_trans->Pu_AP_ID_.second << ">|<"
            //        << successor_trans->Pr_AP_ID_.first << "," << successor_trans->Pr_AP_ID_.second << ">_"<<std::endl;
            if (successor_trans->LCLV_ == cur_trans->LCLV_ && successor_trans->Pu_AP_ID_ == cur_trans->Pu_AP_ID_)
            {
                std::stringstream ss;
                ss << "id:_" << cur_trans->trans_id_ << "_LCLV:" << cur_trans->LCLV_ << "_<" << cur_trans->Pu_AP_ID_.first 
                   << "," << cur_trans->Pu_AP_ID_.second << ">|<"
                   << cur_trans->Pr_AP_ID_.first << "," << cur_trans->Pr_AP_ID_.second << ">_————>_id:_"
                   << successor_trans->trans_id_ << "_LCLV:" << cur_trans->LCLV_ << "_<"
                   << successor_trans->Pu_AP_ID_.first
                   << "," << successor_trans->Pu_AP_ID_.second << ">|<"
                   << successor_trans->Pr_AP_ID_.first << "," << successor_trans->Pr_AP_ID_.second << ">";
                std::string dl_string;
                ss >> dl_string;
                ss.clear();

                DeadLockInformation deadlock_information;
                deadlock_information.DL_INFORMATION = dl_string;
                deadlock_information.DETECT_ID = detect_id;
                deadlock_information.PREDECESSOR_ID = cur_trans->trans_id_;
                deadlock_information.PREDECESSOR_LCLV = cur_trans->LCLV_;
                deadlock_information.DETECT_TIME = cur_trans->GetAllTime_ms();
                ss << "<" << cur_trans->Pu_AP_ID_.first << "," << cur_trans->Pu_AP_ID_.second << ">";
                ss >> deadlock_information.PREDECESSOR_PU_AP_ID;
                ss.clear();

                ss << "<" << cur_trans->Pr_AP_ID_.first << "," << cur_trans->Pr_AP_ID_.second << ">";
                ss >> deadlock_information.PREDECESSOR_PR_AP_ID;
                ss.clear();

                deadlock_information.SUCCESSOR_ID = successor_trans->trans_id_;
                deadlock_information.SUCCESSOR_LCLV = successor_trans->LCLV_;

                ss << "<" << successor_trans->Pu_AP_ID_.first << "," << successor_trans->Pu_AP_ID_.second << ">";
                ss >> deadlock_information.SUCCESSOR_PU_AP_ID;
                ss.clear();

                ss << "<" << successor_trans->Pr_AP_ID_.first << "," << successor_trans->Pr_AP_ID_.second << ">";
                ss >> deadlock_information.SUCCESSOR_PR_AP_ID;
                ss.clear();

                deadlock_informations_mu_.lock();
                deadlock_informations_.push_back(deadlock_information);
                deadlock_informations_mu_.unlock();

                DLO(successor_trans, detect_id, LCLV, Pu_AP_ID, visited_set);
            }
        }
    }
}

bool NetWork::DLO2(TransactionProcess *detect_trans)
{
    std::queue<TransactionProcess *> q;
    std::unordered_set<TransactionProcess *> successor_trans_set;
    detect_trans->Get_successor_trans_set(successor_trans_set);
    for (TransactionProcess *successor_trans : successor_trans_set)
    {
        if (successor_trans->LCLV_ == detect_trans->LCLV_ 
            && successor_trans->Pu_AP_ID_.first == detect_trans->Pu_AP_ID_.first 
            && successor_trans->Pu_AP_ID_.second == detect_trans->Pu_AP_ID_.second)
        {
            q.push(successor_trans);
        }
    }
    int step = 1;
    while (!q.empty())
    {
        // std::cout<<"step: "<<step<<std::endl;
        int size = q.size();
        for (int i = 0; i < size; i++)
        {
            TransactionProcess *cur = q.front();
            q.pop();
            if (cur == detect_trans)
            {
                std::cout << "Ready to output deadlock detection node: "
                          << detect_trans->trans_id_ << " Cycle: " << step << std::endl;
                deadlock_loops_mu_.lock();
                deadlock_loops_.emplace_back(detect_trans->trans_id_, step);
                deadlock_loops_mu_.unlock();
                return true;
            }

            std::unordered_set<TransactionProcess *> tmp_successor_trans_set;
            cur->Get_successor_trans_set(tmp_successor_trans_set);

            for (TransactionProcess *successor_trans : tmp_successor_trans_set)
            {
                if (successor_trans->LCLV_ == detect_trans->LCLV_ 
                    && successor_trans->Pu_AP_ID_.first == detect_trans->Pu_AP_ID_.first 
                    && successor_trans->Pu_AP_ID_.second == detect_trans->Pu_AP_ID_.second)
                {
                    q.push(successor_trans);
                }
            }
        }
        step++;
    }

    return false;
}

void NetWork::Thread_Run_LCLP(int deadlock_thread_id, timespec LCLP_deadline)
{
    // std::cout << "------DeadLock_thread is: " << deadlock_thread_id << " ----LCLP_Start-----------"<<std::endl;
    // The LCLP phase begins Initialization
    timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    int trans_number = deadlock_thread_id == DEADLOCK_THREAD_NUMBER - 1 ? ALL_TRANSNUMBER - deadlock_thread_id * TRANSACTION_NUMBER_DEADLOCK : TRANSACTION_NUMBER_DEADLOCK;
    int trans_start = deadlock_thread_id * TRANSACTION_NUMBER_DEADLOCK;
    int trans_end = trans_start + trans_number;
    for (int i = trans_start; i < trans_end; i++)
    {
        if (!trans_pool_[i]->running || trans_pool_[i]->wait_empty())
        {
            // LOG(DEBUG) << "Trans_id: " << i << " trans_id is: " << trans_pool_[i]->trans_id_ << " do_lclp is 0 ";
            trans_pool_[i]->do_lclp = 0;
        }
        else
        {
            trans_pool_[i]->Pu_AP_ID_.first = trans_pool_[i]->Pr_AP_ID_.first.load();
            trans_pool_[i]->Pu_AP_ID_.second = trans_pool_[i]->Pr_AP_ID_.second.load();
        }
    }
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    // std::cout << "------DeadLock_thread is: " << deadlock_thread_id << " -LCLP initialization time- " 
    // << (now.tv_sec * 1000 + now.tv_nsec / 1000000) - (start.tv_sec * 1000 + start.tv_nsec / 1000000) << " ms"<<std::endl;
    while (now < LCLP_deadline)
    {
        // timespec here;
        // clock_gettime(CLOCK_MONOTONIC_RAW, &here);
        lclp_rounds[deadlock_thread_id]++;
        // LOG(INFO) << "------DeadLock_thread is: " << deadlock_thread_id << " Traverse the LCLP once to deadline----- " 
        // << (LCLP_deadline.tv_sec * 1000 + LCLP_deadline.tv_nsec / 1000000) - (now.tv_sec * 1000 + now.tv_nsec / 1000000) << " ms";
        for (int i = trans_start; i < trans_end; i++)
        {
            if (trans_pool_[i]->do_lclp > 0 && trans_pool_[i]->GetRunTime_ms() > DETECTION_THRESHOLD)
            {
                lclp_node[deadlock_thread_id]++;
                LCLP_Trans_new(trans_pool_[i], LCLP_deadline);
            }
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    // LOG(INFO) << "Revert for the next LCLP";
    for (int i = trans_start; i < trans_end; i++)
    {
        trans_pool_[i]->do_lclp = 1;
    }
}

void NetWork::Thread_Run_LCLS(int deadlock_thread_id, timespec LCLS_deadline)
{
    // std::cout << "------DeadLock_thread is: " << deadlock_thread_id << " ----LCLS_Start-----------";
    // The LCLS phase begins: Initialization
    int trans_number = deadlock_thread_id == DEADLOCK_THREAD_NUMBER - 1 ? ALL_TRANSNUMBER - deadlock_thread_id * TRANSACTION_NUMBER_DEADLOCK : TRANSACTION_NUMBER_DEADLOCK;
    int trans_start = deadlock_thread_id * TRANSACTION_NUMBER_DEADLOCK;
    int trans_end = trans_start + trans_number;
    for (int i = trans_start; i < trans_end; i++)
    {
        if (!trans_pool_[i]->running || trans_pool_[i]->wait_empty())
        {
            // LOG(DEBUG) << "Trans_id: " << i << " trans_id is: " << trans_pool_[i]->trans_id_ << " do_lcls is 0 ";
            trans_pool_[i]->do_lcls = 0;
        }
    }
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    // std::cout  << "------DeadLock_thread is: " << deadlock_thread_id << " LCLS has not entered the loop to deadline:[ " 
    // << (int)(LCLS_deadline.tv_sec * 1000 + LCLS_deadline.tv_nsec / 1000000) - (int)(now.tv_sec * 1000 + now.tv_nsec / 1000000) 
    // << " ] ms"<<std::endl;
    // std::cout << "now s: ["<<now.tv_sec<<"] now ns: ["<<now.tv_nsec <<"] LCLS_deadline s: ["
    // <<LCLS_deadline.tv_sec<<"] LCLS_deadline ns: ["<<LCLS_deadline.tv_nsec<<"] now < LCLS_deadline:[ "
    // <<(now< LCLS_deadline)<<"]"<<std::endl;;
    while (now < LCLS_deadline)
    {
        // timespec here;
        // clock_gettime(CLOCK_MONOTONIC_RAW, &here);
        // std::cout  << "------DeadLock_thread is: " << deadlock_thread_id 
        // << " Start to execute LCLS to deadline----- " 
        // << (LCLS_deadline.tv_sec * 1000 + LCLS_deadline.tv_nsec / 1000000) - (now.tv_sec * 1000 + now.tv_nsec / 1000000) 
        // << " ms"<<std::endl;
        lcls_rounds[deadlock_thread_id]++;
        // LOG(INFO) << "------DeadLock_thread is: " << deadlock_thread_id 
        // << " Traverse the LCLS distance once to deadline----- " 
        // << (LCLS_deadline.tv_sec * 1000 + LCLS_deadline.tv_nsec / 1000000) - (now.tv_sec * 1000 + now.tv_nsec / 1000000) 
        // << " ms";
        for (int i = trans_start; i < trans_end; i++)
        {
            if (trans_pool_[i]->do_lcls > 0 && trans_pool_[i]->GetRunTime_ms() > DETECTION_THRESHOLD)
            {
                lcls_node[deadlock_thread_id]++;
                LCLS_Trans_new(trans_pool_[i], LCLS_deadline, true);
            }
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    // LOG(INFO) << "Revert for the next LCLS";
    for (int i = trans_start; i < trans_end; i++)
    {
        trans_pool_[i]->do_lcls = 1;
    }
}

void NetWork::Thread_Run_Detect(int deadlock_thread_id, timespec Detect_deadline)
{
    // The Detect phase begins: Initialization
    int trans_number = deadlock_thread_id == DEADLOCK_THREAD_NUMBER - 1 ? ALL_TRANSNUMBER - deadlock_thread_id * TRANSACTION_NUMBER_DEADLOCK : TRANSACTION_NUMBER_DEADLOCK;
    int trans_start = deadlock_thread_id * TRANSACTION_NUMBER_DEADLOCK;
    int trans_end = trans_start + trans_number;
    for (int i = trans_start; i < trans_end; i++)
    {
        if (!trans_pool_[i]->running || trans_pool_[i]->wait_empty())
        {
            trans_pool_[i]->do_detect = 0;
        }
    }
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    while (now < Detect_deadline)
    {
        detect_rounds[deadlock_thread_id]++;
        for (int i = trans_start; i < trans_end; i++)
        {
            if (trans_pool_[i]->do_detect > 0 && trans_pool_[i]->GetRunTime_ms() > DETECTION_THRESHOLD)
            {
                detect_node[deadlock_thread_id]++;
                Detect_Trans_new(trans_pool_[i], Detect_deadline, true);
            }
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    for (int i = trans_start; i < trans_end; i++)
    {
        trans_pool_[i]->do_detect = 1;
    }
}

void NetWork::RunTransactionProcess_Auto(int i)
{
    if (trans_pool_[i]->running)
    {
        // The transaction is not finished.
        trans_pool_[i]->Run_Transaction_Auto(resource_pool_);
    }
    else
    {
        // The transaction is finished.
        // LOG(INFO) << "The transaction is finished:" << trans_pool_[i]->trans_id_;
        trans_pool_[i]->OutPut();
        // trans_pool_[i]->newTrans(global_trans_id_++);
        trans_pool_[i]->newTrans(global_trans_id_++, AP_pool_[i / MACHINE_TRANSNUMBER]);
        // LOG(INFO) << "Reset trans,trans id is: " << trans_pool_[i]->trans_id_;
    }
}

void NetWork::RunTransactionProcess_Auto_Done(int i)
{
    // LOG(INFO) << "Run TransactionProcess: " << i;
    if (trans_pool_[i]->GetAllTime_ms() > TIME_OUT)
    {
        if (trans_pool_[i]->running)
        {
            // The transaction is not finished.
            trans_pool_[i]->Run_Transaction_NTO(resource_pool_);
        }
        else
        {
            trans_pool_[i]->OutPut();
        }
    }
    else
    {
        if (trans_pool_[i]->running)
        {
            // The transaction is not finished.
            trans_pool_[i]->Run_Transaction_Auto(resource_pool_);
        }
        else
        {
            // The transaction is finished.
            // LOG(INFO) << "The transaction is finished:" << trans_pool_[i]->trans_id_;
            trans_pool_[i]->OutPut();
            // trans_pool_[i]->newTrans(global_trans_id_++);
            trans_pool_[i]->newTrans(global_trans_id_++, AP_pool_[i / MACHINE_TRANSNUMBER]);
            // LOG(INFO) << "Reset trans,trans id is: " << trans_pool_[i]->trans_id_;
        }
    }
}

void NetWork::Thread_Run_NTO(int trans_thread_id)
{
    bool out[TRANSACTION_NUMBER_TRANS_THREAD];
    for (int i = 0; i < TRANSACTION_NUMBER_TRANS_THREAD; i++)
    {
        out[i] = false;
    }
    while (true)
    {
        bool alldone = true;
        for (int i = trans_thread_id * TRANSACTION_NUMBER_TRANS_THREAD; 
            i < (trans_thread_id + 1) * TRANSACTION_NUMBER_TRANS_THREAD; i++)
        {
            if (trans_pool_[i]->running)
            {
                alldone = false;
                trans_pool_[i]->Run_Transaction_NTO(resource_pool_);
            }
            else
            {
                if (!out[i - trans_thread_id * TRANSACTION_NUMBER_TRANS_THREAD])
                {
                    trans_pool_[i]->OutPut();
                    out[i - trans_thread_id * TRANSACTION_NUMBER_TRANS_THREAD] = true;
                }
            }
        }
        if (alldone)
            break;
    }
}

void NetWork::Thread_Run_Auto(int trans_thread_id)
{
    while (GetAllTime_ms() < TIME_OUT)
    {
        // uint32_t begin = GetAllTime_ms();
        for (int i = trans_thread_id * TRANSACTION_NUMBER_TRANS_THREAD; 
            i < (trans_thread_id + 1) * TRANSACTION_NUMBER_TRANS_THREAD; i++)
        {
            RunTransactionProcess_Auto(i);
        }
        // uint32_t end = GetAllTime_ms();
        // LOG(INFO)<<"Trans thread id : "<< trans_thread_id<< " It takes time to traverse the transaction processor once: " 
        // <<(end - begin )<<" ms"<< std::endl;
    }
    std::cout << " timeout " << std::endl;
}

void NetWork::Thread_Run_Auto_Done(int trans_thread_id)
{
    int trans_start = trans_thread_id * TRANSACTION_NUMBER_TRANS_THREAD;
    int trans_number = trans_thread_id == TRANSACTION_THREAD_NUMBER - 1 ? ALL_TRANSNUMBER - trans_thread_id * TRANSACTION_NUMBER_TRANS_THREAD : TRANSACTION_NUMBER_TRANS_THREAD;
    int trans_end = trans_start + trans_number;
    while (GetAllTime_ms() < TIME_OUT)
    {
        // uint32_t begin = GetAllTime_ms();
        for (int i = trans_start; i < trans_end; i++)
        {
            if (trans_pool_[i]->running)
            {
                // The transaction is not finished.
                trans_pool_[i]->Run_Transaction_Auto(resource_pool_);
            }
            else
            {
                // The transaction is finished.
                trans_pool_[i]->OutPut();
                // trans_pool_[i]->newTrans(global_trans_id_++);
                trans_pool_[i]->newTrans(global_trans_id_++, AP_pool_[i / MACHINE_TRANSNUMBER]);
            }
        }
        // uint32_t end = GetAllTime_ms();
        // std::cout <<"Trans thread id : "<< trans_thread_id<< " It takes time to traverse the transaction processor once: " 
        //<<(end - begin )<<" ms"<< std::endl;
    }
    std::cout << "thread id: " << trans_thread_id << " Timeout!!!" << std::endl;
    // ALL_TO_DB();
    time_out_ = true;
    bool out[trans_number];
    for (int i = 0; i < trans_number; i++)
    {
        out[i] = false;
    }
    while (GetAllTime_ms() < FINAL_TIME_OUT)
    {
        bool alldone = true;
        for (int i = trans_start; i < trans_end; i++)
        {
            if (trans_pool_[i]->running)
            {
                alldone = false;
                // The transaction is not finished.
                trans_pool_[i]->Run_Transaction_Auto(resource_pool_);
            }
            else
            {
                // The transaction is finished.
                if (!out[i - trans_start])
                {
                    trans_pool_[i]->OutPut();
                    out[i - trans_start] = true;
                }
            }
        }
        if (alldone)
        {
            break;
            std::cout << "thread id: " << trans_thread_id << " execution completed!!!" << std::endl;
        }
    }
    std::cout << "thread id: " << trans_thread_id << " Timeout execution completed!!!" << std::endl;
}

void NetWork::Run_NTO(bool deadlock_output)
{
    deadlock_output_ = deadlock_output;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time_point_);
    for (uint32_t i = 0; i < TRANSACTION_THREAD_NUMBER; i++)
    {
        std::cout << "build trans_thread: " << i << " NO: " << i << std::endl;
        workers.emplace_back(std::bind(&NetWork::Thread_Run_NTO, this, i));
    }

    std::cout << "workers thread is established" << std::endl;
    int size = workers.size();

    for (std::thread &worker : workers)
    {
        worker.join();

        std::cout << "trans thread join done";
    }

    std::cout << "trans completed" << std::endl;
    done_ = true;
}
void NetWork::Run_NTO_LCL(bool deadlock_output)
{
    deadlock_output_ = deadlock_output;
    for (uint32_t i = 0; i < TRANSACTION_THREAD_NUMBER; i++)
    {
        std::cout << "build trans_thread: " << i << " NO: " << i << std::endl;
        workers.emplace_back(std::bind(&NetWork::Thread_Run_NTO, this, i));
    }

    std::cout << "workers thread is established" << std::endl;
    std::vector<std::thread> DeadLock_workers;
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    timespec LCLP_start = TimeAdd(now, LCLP_start_time);
    for (uint32_t i = 0; i < DEADLOCK_THREAD_NUMBER; i++)
    {
        // LOG(DEBUG) << "build LCL_thread: " << i << " NO: " << i;
        DeadLock_workers.emplace_back(std::bind(&NetWork::Thread_Run_DeadLockDetect_Periodic2, this, i, LCLP_start));
    }
    std::cout << "DeadLock thread is established" << std::endl;

    int size = workers.size();

    for (std::thread &worker : workers)
    {
        worker.join();

        std::cout << "trans thread join done";
    }

    std::cout << "trans completed" << std::endl;
    done_ = true;
    for (std::thread &DeadLock_worker : DeadLock_workers)
    {
        DeadLock_worker.join();
    }
    std::cout << "deadlock completed";
}
void NetWork::Run_Auto_Done(bool deadlock_output)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time_point_);
    deadlock_output_ = deadlock_output;

    for (uint32_t i = 0; i < ALL_TRANSNUMBER; i++)
    {
        trans_pool_[i]->transaction_process_create_time_point_ = start_time_point_;
    }
    for (uint32_t i = 0; i < TRANSACTION_THREAD_NUMBER; i++)
    {
        // LOG(DEBUG) << "build trans_thread : " << i << " NO: " << i;
        workers.emplace_back(std::bind(&NetWork::Thread_Run_Auto_Done, this, i));
    }
    std::cout << "workers thread completed" << std::endl;

    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    timespec LCLP_start = TimeAdd(now, LCLP_start_time);
    for (uint32_t i = 0; i < DEADLOCK_THREAD_NUMBER; i++)
    {
        // LOG(DEBUG) << "build LCL_thread: " << i << " NO: " << i;
        DeadLock_workers.emplace_back(std::bind(&NetWork::Thread_Run_DeadLockDetect_Periodic2, this, i, LCLP_start));
    }
    std::cout << "DeadLock thread completed" << std::endl;

    int size = workers.size();
    for (std::thread &worker : workers)
    {
        // LOG(DEBUG) << "trans thread join";
        worker.join();
        // LOG(DEBUG) << "trans thread join done";
    }
    std::cout << "trans completed" << std::endl;
    done_ = true;

    for (std::thread &DeadLock_worker : DeadLock_workers)
    {
        DeadLock_worker.join();
    }
    std::cout << "deadlock completed";
}
void NetWork::Run_Auto(bool deadlock_output)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time_point_);
    deadlock_output_ = deadlock_output;
    std::vector<std::thread> DeadLock_workers;
    for (uint32_t i = 0; i < TRANSACTION_THREAD_NUMBER; i++)
    {
        // LOG(DEBUG) << "build trans_thread : " << i << " NO: " << i;
        workers.emplace_back(std::bind(&NetWork::Thread_Run_Auto, this, i));
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time_point_);
    std::cout << "worker threads are established" << std::endl;

    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    timespec LCLP_start = TimeAdd(now, LCLP_start_time);
    for (uint32_t i = 0; i < DEADLOCK_THREAD_NUMBER; i++)
    {
        // LOG(DEBUG) << "build LCL_thread: " << i << " NO: " << i;
        DeadLock_workers.emplace_back(std::bind(&NetWork::Thread_Run_DeadLockDetect_Periodic2, this, i, LCLP_start));
    }
    std::cout << "DeadLock thread completed" << std::endl;

    int size = workers.size();
    for (std::thread &worker : workers)
    {

        worker.join();
    }
    std::cout << "trans completed";
    done_ = true;

    for (std::thread &DeadLock_worker : DeadLock_workers)
    {
        DeadLock_worker.join();
    }
    std::cout << "deadlock completed";
}
void NetWork::Run_Auto2(bool deadlock_output)
{
    deadlock_output_ = deadlock_output;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time_point_);
    std::vector<std::thread> DeadLock_workers;
    for (uint32_t i = 0; i < TRANSACTION_THREAD_NUMBER; i++)
    {
        // LOG(DEBUG) << "build trans_thread : " << i << " NO: " << i;
        workers.emplace_back(std::bind(&NetWork::Thread_Run_Auto, this, i));
    }
    std::cout << "worker threads are established" << std::endl;
    // timespec LCLP_start=TimeAdd(start_time_point_,LCLP_start_time);

    int size = workers.size();
    for (std::thread &worker : workers)
    {
        // LOG(DEBUG) << "trans thread join";
        worker.join();
        // LOG(DEBUG) << "trans thread join done";
    }
    std::cout << "trans completed";
    done_ = true;
}

void NetWork::Thread_Run_DeadLockDetect_Once_Doit(int deadlock_thread_id, timespec LCLP_start, int x)
{
    lclp_count[deadlock_thread_id] = 0;
    lclp_node[deadlock_thread_id] = 0;
    lclp_rounds[deadlock_thread_id] = 0;
    lcls_count[deadlock_thread_id] = 0;
    lcls_node[deadlock_thread_id] = 0;
    lcls_rounds[deadlock_thread_id] = 0;
    detect_count[deadlock_thread_id] = 0;
    detect_node[deadlock_thread_id] = 0;
    detect_rounds[deadlock_thread_id] = 0;

    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    std::cout << "deadlock_thread_id : " << deadlock_thread_id << " now s: [" << now.tv_sec << "] now ns: [" 
    << now.tv_nsec << "] " << std::endl;
    
    while (now < LCLP_start)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    timespec LCLP_deadline = TimeAdd(LCLP_start, LCLP_AllTime * x);
    timespec LCLS_Start = TimeAdd(LCLP_start, LCLS_start_time * x);
    timespec LCLS_deadline = TimeAdd(LCLP_start, LCLS_start_time * x + LCLS_AllTime * x);
    timespec Detect_Start = TimeAdd(LCLP_start, Detect_start_time * x);
    timespec Detect_deadline = TimeAdd(LCLP_start, Detect_start_time * x + Detect_AllTime * x);

    Thread_Run_LCLP(deadlock_thread_id, LCLP_deadline);

    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    while (now < LCLS_Start)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    Thread_Run_LCLS(deadlock_thread_id, LCLS_deadline);

    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    while (now < Detect_Start)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    Thread_Run_Detect(deadlock_thread_id, Detect_deadline);

    LCLP_start = TimeAdd(LCLP_start, T * x);

    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lclp one action: " << lclp_count[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lclp one vertex: " << lclp_node[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lclp one round: " << lclp_rounds[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lcls one action: " << lcls_count[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lcls one vertex: " << lcls_node[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lcls one round: " << lcls_rounds[deadlock_thread_id] << std::endl;
}

void NetWork::Thread_Run_DeadLockDetect_Once(int deadlock_thread_id, timespec LCLP_start)
{
    lclp_count[deadlock_thread_id] = 0;
    lclp_node[deadlock_thread_id] = 0;
    lclp_rounds[deadlock_thread_id] = 0;
    lcls_count[deadlock_thread_id] = 0;
    lcls_node[deadlock_thread_id] = 0;
    lcls_rounds[deadlock_thread_id] = 0;
    detect_count[deadlock_thread_id] = 0;
    detect_node[deadlock_thread_id] = 0;
    detect_rounds[deadlock_thread_id] = 0;

    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    std::cout << "deadlock_thread_id : " << deadlock_thread_id << " now s: [" << now.tv_sec << "] now ns: [" 
    << now.tv_nsec << "] " << std::endl;
    
    while (now < LCLP_start)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    timespec LCLP_deadline = TimeAdd(LCLP_start, LCLP_AllTime);
    timespec LCLS_Start = TimeAdd(LCLP_start, LCLS_start_time);
    timespec LCLS_deadline = TimeAdd(LCLP_start, LCLS_start_time + LCLS_AllTime);
    timespec Detect_Start = TimeAdd(LCLP_start, Detect_start_time);
    timespec Detect_deadline = TimeAdd(LCLP_start, Detect_start_time + Detect_AllTime);

    Thread_Run_LCLP(deadlock_thread_id, LCLP_deadline);

    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    while (now < LCLS_Start)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    Thread_Run_LCLS(deadlock_thread_id, LCLS_deadline);

    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    while (now < Detect_Start)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    Thread_Run_Detect(deadlock_thread_id, Detect_deadline);

    LCLP_start = TimeAdd(LCLP_start, T);

    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lclp one action: " << lclp_count[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lclp one vertex: " << lclp_node[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lclp one round: " << lclp_rounds[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lcls one action: " << lcls_count[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lcls one vertex: " << lcls_node[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "lcls one round: " << lcls_rounds[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "detect one action: " << detect_count[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "detect one vertex: " << detect_node[deadlock_thread_id] << std::endl;
    std::cout << "deadlock thread id : " << deadlock_thread_id << " "
              << "detect one round: " << detect_rounds[deadlock_thread_id] << std::endl;
}

void NetWork::Thread_Run_DeadLockDetect_Periodic(int deadlock_thread_id, timespec LCLP_start)
{
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    while (!done_)
    { // timespec start;
        // clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        // std::cout<<"----------LCL once------------"<<std::endl;
        // std::cout << "LCLP_start s: ["<<LCLP_start.tv_sec<<"] LCLP_start ns: ["<<LCLP_start.tv_nsec <<"]"<<std::endl;;
        Thread_Run_DeadLockDetect_Once(deadlock_thread_id, LCLP_start);
        LCLP_start = TimeAdd(LCLP_start, T);
        // std::cout<<"-----------LCL once done------"<<std::endl;
        // timespec end;
        // clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        // std::cout<<"LCL timecost: [" << (end.tv_sec * 1000 + end.tv_nsec / 1000000) - (start.tv_sec * 1000 + start.tv_nsec / 1000000) 
        // << "] ms"<<std::endl;
    }
}
void NetWork::Thread_Run_DeadLockDetect_Periodic2(int deadlock_thread_id, timespec LCLP_start)
{
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    while (!done_)
    {
        // lclp_count[deadlock_thread_id] = 0;
        // lclp_node [deadlock_thread_id]= 0;
        // lclp_rounds[deadlock_thread_id] = 0;
        // lcls_count[deadlock_thread_id] = 0;
        // lcls_node [deadlock_thread_id]= 0;
        // lcls_rounds [deadlock_thread_id]= 0;
        // std::cout << "deadlock_thread_id : "<<deadlock_thread_id<<" now s: ["<<now.tv_sec<<"] now ns: ["
        // <<now.tv_nsec <<"] "<<std::endl;;
        while (now < LCLP_start)
        {
            clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        }
        pthread_rwlock_rdlock(&base_rwlock_);
        timespec LCLP_deadline = TimeAdd(LCLP_start, LCLP_AllTime * base);
        timespec LCLS_Start = TimeAdd(LCLP_start, LCLS_start_time * base);
        timespec LCLS_deadline = TimeAdd(LCLP_start, LCLS_start_time * base + LCLS_AllTime * base);
        timespec Detect_Start = TimeAdd(LCLP_start, Detect_start_time * base);
        timespec Detect_deadline = TimeAdd(LCLP_start, Detect_start_time * base + Detect_AllTime * base);
        pthread_rwlock_unlock(&base_rwlock_);

        Thread_Run_LCLP(deadlock_thread_id, LCLP_deadline);

        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        while (now < LCLS_Start)
        {
            clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        }
        Thread_Run_LCLS(deadlock_thread_id, LCLS_deadline);

        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        while (now < Detect_Start)
        {
            clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        }
        Thread_Run_Detect(deadlock_thread_id, Detect_deadline);
        pthread_rwlock_rdlock(&base_rwlock_);
        LCLP_start = TimeAdd(LCLP_start, T * base);
        pthread_rwlock_unlock(&base_rwlock_);
        // std::cout <<"deadlock thread id : "<< deadlock_thread_id<<" "<< "lclp one action: " << lclp_count[deadlock_thread_id] << std::endl;
        // std::cout <<"deadlock thread id : "<< deadlock_thread_id<<" "<< "lclp one vertex: " << lclp_node[deadlock_thread_id] << std::endl;
        // std::cout <<"deadlock thread id : "<< deadlock_thread_id<<" "<< "lclp one round: " << lclp_rounds[deadlock_thread_id] << std::endl;
        // std::cout <<"deadlock thread id : "<< deadlock_thread_id<<" "<< "lcls one action: " << lcls_count[deadlock_thread_id]<< std::endl;
        // std::cout <<"deadlock thread id : "<< deadlock_thread_id<<" "<< "lcls one vertex: " << lcls_node[deadlock_thread_id] << std::endl;
        // std::cout <<"deadlock thread id : "<< deadlock_thread_id<<" "<< "lcls one round: " << lcls_rounds[deadlock_thread_id] << std::endl;
    }
}
void NetWork::PrintGraph()
{
    std::ofstream graph;
    graph.open("graph_.csv");
    timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    std::vector<GlobalInformation> global_informations;
    GlobalInformation global_information;

    for (TransactionProcess *cur_trans : trans_pool_)
    {
        std::stringstream ss;

        TransactionProcess *successor_trans;
        std::unordered_set<Resource *> wait_res_set;
        cur_trans->Get_wait_res_set(wait_res_set);
        if (!cur_trans->running && !wait_res_set.empty())
        {
            LOG(WARNING) << "transis over, but there is still waiting for res, trans_id is: " << cur_trans->trans_id_;
        }

        LOG(DEBUG) << "trans_id is : " << cur_trans->trans_id_ << " running is: " << cur_trans->running
                   << "wait_res size is:" << wait_res_set.size();
        for (Resource *wait_res : wait_res_set)
        {
            LOG(DEBUG) << "trans_id is : " << cur_trans->trans_id_ << " running is: " << cur_trans->running
                       << "wait_res id is:" << wait_res->res_id_;
            successor_trans = nullptr;
            wait_res->Get_cur_trans(successor_trans);
            if (successor_trans == nullptr)
            {
                continue;
            }
            if (!successor_trans->running)
            {
                LOG(WARNING) << "res using is done";
            }
            ss << successor_trans->trans_id_;
            ss << ",";
        }
        global_information.TRANS_ID = cur_trans->trans_id_;
        global_information.WAIT_IDS = "";
        ss >> global_information.WAIT_IDS;
        ss.clear();
        ss.str("");
        global_informations.push_back(global_information);
        graph << global_information.TRANS_ID << " " << global_information.WAIT_IDS << std::endl;
    }
    graph.close();
    timespec end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    LOG(INFO) << "----Print global graph----- " << (end.tv_sec * 1000 + end.tv_nsec / 1000000) - (start.tv_sec * 1000 + start.tv_nsec / 1000000) << " ms";
    std::cout << "Print Graph done" << std::endl;
}

void NetWork::LCL_Once(bool rollback)
{
    std::cout << "----------------LCL_Once----------------" << std::endl;

    std::vector<std::thread> DeadLock_workers;
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    timespec LCLP_start = TimeAdd(now, LCLP_start_time);
    for (int deadlock_thread_id = 0; deadlock_thread_id < DEADLOCK_THREAD_NUMBER; deadlock_thread_id++)
    {
        DeadLock_workers.emplace_back(std::bind(&NetWork::Thread_Run_DeadLockDetect_Once, this, deadlock_thread_id, LCLP_start));
    }

    std::cout << "DeadLock thread complete!" << std::endl;
    for (std::thread &DeadLock_worker : DeadLock_workers)
    {
        DeadLock_worker.join();
    }
    std::cout << "----------------LCL_Once end----------------" << std::endl;
}
void NetWork::LCL_Once_Doit(int x)
{
    std::cout << "----------------LCL_Once----------------" << std::endl;

    std::vector<std::thread> DeadLock_workers;
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    timespec LCLP_start = TimeAdd(now, LCLP_start_time);
    for (int deadlock_thread_id = 0; deadlock_thread_id < DEADLOCK_THREAD_NUMBER; deadlock_thread_id++)
    {
        DeadLock_workers.emplace_back(std::bind(&NetWork::Thread_Run_DeadLockDetect_Once_Doit, 
            this, deadlock_thread_id, LCLP_start, x));
    }

    std::cout << "DeadLock thread complete!" << std::endl;
    for (std::thread &DeadLock_worker : DeadLock_workers)
    {
        DeadLock_worker.join();
    }

    std::cout << "----------------LCL_Once end----------------" << std::endl;
}
void NetWork::LCL_Periodic()
{
    std::cout << "----------------LCL_Periodic----------------" << std::endl;
    std::vector<std::thread> DeadLock_workers;
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    timespec LCLP_start = TimeAdd(now, LCLP_start_time);
    for (int deadlock_thread_id = 0; deadlock_thread_id < DEADLOCK_THREAD_NUMBER; deadlock_thread_id++)
    {
        DeadLock_workers.emplace_back(std::bind(&NetWork::Thread_Run_DeadLockDetect_Periodic2, 
            this, deadlock_thread_id, LCLP_start));
    }

    for (std::thread &DeadLock_worker : DeadLock_workers)
    {
        DeadLock_worker.join();
    }
    std::cout << "---------------LCL_Periodic end---------------" << std::endl;
}

uint32_t NetWork::GetAllTime_ms()
{
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    return (now.tv_sec - start_time_point_.tv_sec) * 1000 + (now.tv_nsec - start_time_point_.tv_nsec) / 1000000;
}
void NetWork::ALL_TO_DB()
{
    unsigned int max_commit_time = 0;
    std::cout << "Start output" << std::endl;
    int count = 0;
    int detect_count = 0;
    for (int i = 0; i < ALL_TRANSNUMBER; i++)
    {
        count += trans_pool_[i]->commit_times_.size();
        detect_count += trans_pool_[i]->detect_times_.size();
    }
    std::cout << "The total number of committed transactions: " << count << std::endl;
    std::cout << "Total number of deadlocks detected: " << detect_count << std::endl;
    std::cout << "Output file and commit transactions!" << std::endl;
    std::ofstream commit_times_file;
    std::ofstream detect_times_file;
    std::ofstream deadlock_informations_file;
    std::ofstream deadlock_loops_file;

    std::ostringstream ostr;
    ostr << "commit_times_" << RES_NUMBER << "_.csv";
    std::string commit_times_filename = ostr.str();
    commit_times_file.open(ostr.str().c_str());
    ostr.clear();
    ostr.str("");

    ostr << "detect_times_" << RES_NUMBER << "_.csv";
    std::string detect_times_filename = ostr.str();
    detect_times_file.open(ostr.str().c_str());
    ostr.clear();
    ostr.str("");

    ostr << "deadlock_informations_" << RES_NUMBER << "_.csv";
    std::string deadlock_informations_filename = ostr.str();
    deadlock_informations_file.open(ostr.str().c_str());
    ostr.clear();
    ostr.str("");

    ostr << "deadlock_loops_" << RES_NUMBER << "_.csv";

    deadlock_loops_file.open(ostr.str().c_str());
    // Write the data entered by the user to the file
    for (auto data : deadlock_loops_)
    {
        deadlock_loops_file << data.first << " " << data.second << std::endl;
    }
    std::cout << ostr.str().c_str() << " Output complete!" << std::endl;
    std::stringstream commit_times_ss;
    std::stringstream detect_times_ss;
    for (int i = 0; i < ALL_TRANSNUMBER; i++)
    {
        for (auto data : trans_pool_[i]->commit_times_)
        {
            max_commit_time = std::max(max_commit_time, data);
            commit_times_ss << data << ",";
        }
        for (auto data : trans_pool_[i]->detect_times_)
        {
            detect_times_ss << data << ",";
        }
    }
    std::string res;
    commit_times_ss >> res;

    commit_times_file << res;
    std::cout << "Transaction commit time: " << max_commit_time << " ms" << std::endl;
    std::cout << commit_times_filename << " Output complete!" << std::endl;

    std::string detect_res;
    detect_times_ss >> detect_res;
    detect_times_file << detect_res;
    std::cout << detect_times_filename << " Output complete!" << std::endl;

    for (auto data : deadlock_informations_)
    {
        deadlock_informations_file << data.DETECT_ID << " " << data.PREDECESSOR_ID
                                   << " " << data.SUCCESSOR_ID << " " << data.DETECT_TIME << " "
                                   << data.DL_INFORMATION << std::endl;
    }

    // Close files
    commit_times_file.close();
    detect_times_file.close();
    deadlock_informations_file.close();
    deadlock_loops_file.close();
    std::cout << deadlock_informations_file << " Output complete!" << std::endl;
}
void NetWork::ALL_TO_DB_easy()
{
    std::cout << "Start output: " << std::endl;
    int count = 0;
    int detect_count = 0;
    int running_count = 0;
    for (int i = 0; i < ALL_TRANSNUMBER; i++)
    {
        count += trans_pool_[i]->commit_times_.size();
        detect_count += trans_pool_[i]->detect_times_.size();
        if (trans_pool_[i]->running)
            running_count++;
    }
    std::cout << "The total number of committed transactions: " << count << std::endl;
    std::cout << "Total number of deadlocks detected: " << detect_count << std::endl;
    std::cout << "The number of transactions that have not yet been committed: " << running_count << std::endl;
}