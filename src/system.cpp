#include "system.h"

timespec TimeAdd2(const timespec &time1, uint32_t time2_ms)
{
    timespec result;
    result.tv_nsec = time1.tv_nsec + (time2_ms % 1000) * 1000000;
    result.tv_sec = time1.tv_sec + time2_ms / 1000 + result.tv_nsec/1000000000;
    result.tv_nsec= result.tv_nsec %1000000000;
    return result;
}

bool aminb(const timespec &time1, const timespec &time2)
{
    if (time1.tv_sec < time2.tv_sec)
        return true;
    else if (time1.tv_sec > time2.tv_sec)
        return false;
    else
        return time1.tv_nsec < time2.tv_nsec;
}

Resource::Resource(int res_id) : res_id_(res_id), cur_trans_(nullptr)
{
    pthread_rwlock_init(&res_rwlock_, nullptr);
}

bool Resource::Acquire(TransactionProcess *transactionProcess)
{
    bool result = false;
    pthread_rwlock_wrlock(&res_rwlock_);
    if (cur_trans_ == nullptr)
    {
        cur_trans_ = transactionProcess;
        result = true;
    }
    pthread_rwlock_unlock(&res_rwlock_);
    return result;
}

void Resource::Release()
{
    pthread_rwlock_wrlock(&res_rwlock_);
    cur_trans_ = nullptr;
    pthread_rwlock_unlock(&res_rwlock_);
}


void Resource::Get_cur_trans(TransactionProcess *&transactionProcess)
{
    pthread_rwlock_rdlock(&res_rwlock_);
    if (cur_trans_ == nullptr)
    {
        LOG(WARNING) << "cur_trans==nullptr";
    }else{
        transactionProcess = cur_trans_;
    }

    pthread_rwlock_unlock(&res_rwlock_);
}


TransactionProcess::TransactionProcess(int transprocess_id, int trans_id, std::atomic<int> &global_id) : transprocess_id_(transprocess_id), 
trans_id_(trans_id), running(true), LCLV_(0),do_sql_id(-1), do_lclp(1), do_lcls(1), do_detect(1), rd{},
	gen{ rd() },     
    //sql_number_distribution_(new std::uniform_real_distribution<double>(SQL_MIN , SQL_MAX )), 
    //resource_distribution_(new std::uniform_real_distribution<double>(RESOURCE_MIN, RESOURCE_MAX)) 
    //sql_number_distribution_(new std::normal_distribution<double>(SQL_MEAN , SQL_STDDEV )),                                                                                                                              
    resource_distribution_(new std::normal_distribution<double>(RESOURCE_MEAN, RESOURCE_STDDEV)),                                                                                                                                   
    sql_number_distribution_(new std::exponential_distribution<double>(1/SQL_MEAN)) 
    //resource_distribution_(new std::exponential_distribution<double>(1/RESOURCE_MEAN))                                                                                                                                                                                                                                                                                                            
{   
    Pr_AP_ID_.first = Pu_AP_ID_.first = global_id++;
    Pr_AP_ID_.second = Pu_AP_ID_.second = global_id++;
    pthread_rwlock_init(&trans_rwlock_, NULL);
    //Transaction configuration
    sql_num = GRandom(sql_number_distribution_, SQL_MAX, SQL_MIN );

    trans_disposition.resize(sql_num);
    clock_gettime(CLOCK_MONOTONIC_RAW, &transaction_start_time_point_);
    clock_gettime(CLOCK_MONOTONIC_RAW, &transaction_process_create_time_point_);
    std::unordered_set<int> id_set;
    for (int i = 0; i < sql_num; i++)
    {
        bool same_machine=true;
        trans_disposition[i].res_num = GRandom(resource_distribution_, RESOURCE_MAX, RESOURCE_MIN);
        trans_disposition[i].write=random()%100>SQL_READ_PERCENT;
        trans_disposition[i].acquire_resource_id.resize(trans_disposition[i].res_num);
        for (int j = 0; j < trans_disposition[i].res_num; j++)
        {   
            int machine_id = random() % MACHINE_NUMBER;
            int res_id = machine_id * MACHINE_RESNUMBER + random() % MACHINE_RESNUMBER;

            if(id_set.count(res_id)){
                res_id = machine_id * MACHINE_RESNUMBER + random() % MACHINE_RESNUMBER;
            }else{
                trans_disposition[i].acquire_resource_id[j] = res_id;
                id_set.insert(res_id);
            }       
        }
    }
}

TransactionProcess::~TransactionProcess()
{
    delete sql_number_distribution_;
}

void TransactionProcess::newTrans(int trans_id)
{      
    clock_gettime(CLOCK_MONOTONIC_RAW, &transaction_start_time_point_);

    trans_id_ = trans_id;
    running = true;
    do_sql_id =-1;
}

void TransactionProcess::newTrans(int trans_id,std::atomic<int> &PrAP){
    clock_gettime(CLOCK_MONOTONIC_RAW, &transaction_start_time_point_);
    trans_id_ = trans_id;
    Pr_AP_ID_.first=PrAP++;
    running = true;
    do_sql_id =-1;
    for (int i = 0; i < sql_num; i++)
    {
        trans_disposition[i].write=rd()%100>SQL_READ_PERCENT;
    }
}


void TransactionProcess::Acquire(Resource *resource)
{
    bool success = false;
    //For distributed scenarios, repeated requests may be sent.
    //If the application already exists, it will directly return true
    pthread_rwlock_wrlock(&trans_rwlock_);
    success = resource->Acquire(this);
    if (success)
    {
        hold_res_set_.insert(resource);
        wait_res_set_.erase(resource);
    }
    else
    {   
        wait_res_set_.insert(resource);
    }
    pthread_rwlock_unlock(&trans_rwlock_);
}


void TransactionProcess::Release()
{
    pthread_rwlock_wrlock(&trans_rwlock_);
    for (Resource *resource : hold_res_set_)
    {
        resource->Release();
    }
    hold_res_set_.clear();
    pthread_rwlock_unlock(&trans_rwlock_);
}

void TransactionProcess::StopWait()
{
    pthread_rwlock_wrlock(&trans_rwlock_);
    wait_res_set_.clear();
    pthread_rwlock_unlock(&trans_rwlock_);
}

void TransactionProcess::Rollback()
{
    running = false;
    do_sql_id = sql_num;
    StopWait();
    Release();
}

void TransactionProcess::Get_wait_res_set(std::unordered_set<Resource *> &res_set)
{
    pthread_rwlock_rdlock(&trans_rwlock_);
    res_set = wait_res_set_;
    pthread_rwlock_unlock(&trans_rwlock_);
}

void TransactionProcess::Get_successor_trans_set(std::unordered_set<TransactionProcess *> &successor_trans_set){
    pthread_rwlock_rdlock(&trans_rwlock_);
    TransactionProcess *successor_trans = nullptr;
    for (Resource *wait_res : wait_res_set_)
    {
        successor_trans = nullptr;
        wait_res->Get_cur_trans(successor_trans);
        if (successor_trans == nullptr)
        {
            //It’s not real-time but regular applications
            continue;
        }
        if (successor_trans == this)
        {
            //A!=B
            continue;
        }
        successor_trans_set.insert(successor_trans);
    }
    pthread_rwlock_unlock(&trans_rwlock_);
}

void TransactionProcess::Run_Transaction_NTO(std::vector<Resource *> &resource_pool)
{
    if (do_sql_id == sql_num)
    {
        //Finished
        StopWait();
        Release();
        if (!wait_res_set_.empty())
        {   
            StopWait();
            Release();
        }
        running = false;
    }
    else
    {
        Run_SQL(resource_pool);
    }
}

void TransactionProcess::Run_Transaction_Auto(std::vector<Resource *> &resource_pool)
{
    if (do_sql_id == sql_num)
    {
        //Finished
        StopWait();
        Release();
        if (!wait_res_set_.empty())
        {   
            StopWait();
            Release();
        }
        running = false;
    }
    else
    {
        Run_SQL(resource_pool);
    }
}

// Run_SQL is currently used.
void TransactionProcess::Run_SQL(std::vector<Resource *> &resource_pool)
{
    if (wait_res_set_.empty())
    {      
        do_sql_id++;
        if(do_sql_id>=sql_num){
            return;
        } 
            
        //Not blocked: It means that the last SQL is executed and the next SQL is executed.
        if (trans_disposition[do_sql_id].write)
        {   
            //Write SQL: to apply for resources
            for (int j = 0; j < trans_disposition[do_sql_id].res_num; j++)
            {
                if(do_sql_id>=sql_num){
                    return;
                }
                this->Acquire(resource_pool[trans_disposition[do_sql_id].acquire_resource_id[j]]);
                
            }
        }
    }
    else
    {
        std::unordered_set<Resource *> res_set;
        Get_wait_res_set(res_set);
        for(auto res:res_set){
            this->Acquire(res);
        }
    }
}

//M&M TransactionProcess : Assuming each process waits on one resource at a time, the maximum outdegree of the wait-for graph will be one
TransactionProcess::TransactionProcess(int transprocess_id, int trans_id, std::atomic<int> &global_id,std::atomic<int> &global_ap) : transprocess_id_(transprocess_id),
                                                                                                         trans_id_(trans_id), running(true), ACTIVE(true), LCLV_(0), do_sql_id(-1), do_sql_res_id(0), do_lclp(1), do_lcls(1), do_detect(1), rd{},
                                                                                                         gen{rd()},
                                                                                                         // sql_number_distribution_(new std::uniform_real_distribution<double>(SQL_MIN , SQL_MAX )),
                                                                                                         // resource_distribution_(new std::uniform_real_distribution<double>(RESOURCE_MIN, RESOURCE_MAX))
                                                                                                         // sql_number_distribution_(new std::normal_distribution<double>(SQL_MEAN , SQL_STDDEV )),
                                                                                                         resource_distribution_(new std::normal_distribution<double>(RESOURCE_MEAN, RESOURCE_STDDEV)),
                                                                                                         sql_number_distribution_(new std::exponential_distribution<double>(1 / SQL_MEAN))
// resource_distribution_(new std::exponential_distribution<double>(1/RESOURCE_MEAN))
{
    wait_res_ = nullptr;
    Pr_AP_ID_.first = Pu_AP_ID_.first = global_id++;
    Pr_AP_ID_.second = Pu_AP_ID_.second = global_ap++;
    pthread_rwlock_init(&trans_rwlock_, NULL);
    // Transaction configuration
    sql_num = GRandom(sql_number_distribution_, SQL_MAX, SQL_MIN);

    trans_disposition.resize(sql_num);
    clock_gettime(CLOCK_MONOTONIC_RAW, &transaction_start_time_point_);
    clock_gettime(CLOCK_MONOTONIC_RAW, &transaction_process_create_time_point_);
    std::unordered_set<int> id_set;
    for (int i = 0; i < sql_num; i++)
    {
        bool same_machine = true;
        trans_disposition[i].res_num = GRandom(resource_distribution_, RESOURCE_MAX, RESOURCE_MIN);
        trans_disposition[i].write = random() % 100 > SQL_READ_PERCENT;
        trans_disposition[i].acquire_resource_id.resize(trans_disposition[i].res_num);
        for (int j = 0; j < trans_disposition[i].res_num; j++)
        {
            int machine_id = random() % MACHINE_NUMBER;
            int res_id = machine_id * MACHINE_RESNUMBER + random() % MACHINE_RESNUMBER;

            if (id_set.count(res_id))
            {
                res_id = machine_id * MACHINE_RESNUMBER + random() % MACHINE_RESNUMBER;
            }
            else
            {
                trans_disposition[i].acquire_resource_id[j] = res_id;
                id_set.insert(res_id);
            }
        }
    }
}


void TransactionProcess::Acquire_MM(Resource *resource)
{
    bool success = false;
    pthread_rwlock_wrlock(&trans_rwlock_);
    success = resource->Acquire(this);
    if (success)
    {
        do_sql_res_id++;
        hold_res_set_.insert(resource);
        if (wait_res_ == resource)
        {
            //M&M ACTIVE
            wait_res_ = nullptr;
            ACTIVE = true;
        }
    }
    else
    {
        wait_res_ = resource;
        ACTIVE = false;
    }
    pthread_rwlock_unlock(&trans_rwlock_);
}

void TransactionProcess::StopWait_MM()
{
    pthread_rwlock_wrlock(&trans_rwlock_);
    ACTIVE = true;
    wait_res_ = nullptr;
    pthread_rwlock_unlock(&trans_rwlock_);
}

void TransactionProcess::Rollback_MM()
{
    running = false;
    do_sql_id = sql_num;
    do_sql_res_id = 0;
    StopWait_MM();
    Release();
}

void TransactionProcess::Get_wait_res_MM(Resource *&wait_res)
{
    pthread_rwlock_rdlock(&trans_rwlock_);
    wait_res = wait_res_;
    pthread_rwlock_unlock(&trans_rwlock_);
}

void TransactionProcess::Get_successor_trans_MM(TransactionProcess *&successor_trans){
    pthread_rwlock_rdlock(&trans_rwlock_);
    if(ACTIVE||wait_res_==nullptr){
        //no successor_trans, return;
        pthread_rwlock_unlock(&trans_rwlock_);
        return;
    }
    wait_res_->Get_cur_trans(successor_trans);
    //LOG(DEBUG) <<"successor_trans is: "<<successor_trans<<" cur status is ACTIVE?:"<<ACTIVE  <<" waitres's cur_trans is: "<<wait_res_->cur_trans_;
    

    pthread_rwlock_unlock(&trans_rwlock_);
}

void TransactionProcess::Run_Transaction_Auto_MM(std::vector<Resource *> &resource_pool, std::atomic<int> &global_id)
{
    if (do_sql_id == sql_num)
    {
        // Finished
        StopWait_MM();
        Release();
        if (wait_res_ != nullptr)
        {
            StopWait_MM();
            Release();
        }
        running = false;
    }
    else
    {
        Run_SQL_MM(resource_pool,global_id);
    }
}

void TransactionProcess::Run_SQL_MM(std::vector<Resource *> &resource_pool, std::atomic<int> &global_id)
{
    if (ACTIVE)
    {
        // It means that the last SQL is executed.
        if (do_sql_res_id == 0)
        {
            do_sql_id++;
        }
        if (do_sql_id >= sql_num)
        {
            do_sql_res_id = 0;
            return;
        }

        // Not blocked: It means that the last SQL is executed and the next SQL is executed.
        if (trans_disposition[do_sql_id].write)
        {
            // Write SQL: to apply for resources
            while (do_sql_res_id < trans_disposition[do_sql_id].res_num)
            {
                if (do_sql_id >= sql_num)
                {
                    do_sql_res_id = 0;
                    return;
                }
                this->Acquire_MM(resource_pool[trans_disposition[do_sql_id].acquire_resource_id[do_sql_res_id]]);
                if (!ACTIVE)
                {
                    // acquire failed,  M&M Block
                    this->Pu_AP_ID_.first=this->Pr_AP_ID_.first.load();
                    this->Pu_AP_ID_.second=global_id++;
                    this->Pr_AP_ID_.second=Pu_AP_ID_.second.load();
                    break;
                }
            }
            if (do_sql_res_id == trans_disposition[do_sql_id].res_num)
            {
                do_sql_res_id = 0;
            }
        }
    }
    else
    {
        Resource *wait_res;
        Get_wait_res_MM(wait_res);
        this->Acquire_MM(wait_res);
    }
}

uint32_t TransactionProcess::GRandom(std::normal_distribution<double> *distribution, uint32_t max, uint32_t min)
{
    uint32_t res = std::round((*distribution)(gen));
    if (res < min)
        res = min;
    if (res > max)
        res = max;
    return res;
}


uint32_t TransactionProcess::GRandom(std::exponential_distribution<double> *distribution, uint32_t max, uint32_t min)
{
    uint32_t res = std::round((*distribution)(gen));
    if (res < min)
        res = min;
    if (res > max)
        res = max;
    return res;
}


uint32_t TransactionProcess::GetRunTime_ms()
{
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    return (now.tv_sec - transaction_start_time_point_.tv_sec) * 1000 + (now.tv_nsec - transaction_start_time_point_.tv_nsec) / 1000000;
}

uint32_t TransactionProcess::GetAllTime_ms()
{
    timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    return (now.tv_sec - transaction_process_create_time_point_.tv_sec) * 1000 + (now.tv_nsec - transaction_process_create_time_point_.tv_nsec) / 1000000;
}

void TransactionProcess::OutPut()
{
    commit_times_.push_back(GetAllTime_ms());
    //std::cout<<"Tx done one"<<std::endl;
    // TransInformation trans_information;
    // trans_information.TRANS_ID = trans_id_;
    // trans_information.LONG_OR_SHORT = length_ == LONG ? "long" : "short";
    // trans_information.WRITE_OR_READ = write_or_read_ == WRITE ? "write" : "read";
    // trans_information.SQL_NUM = trans_disposition.size();
    // trans_information.SQL_INFORMATION = Encode(trans_disposition);
    // timespec now;
    // clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    // trans_information.TOTAL_TIME = (now.tv_sec * 1000 + now.tv_nsec / 1000000) - (transaction_start_time_point_.tv_sec * 1000 + transaction_start_time_point_.tv_nsec / 1000000);
    // trans_information.COMMIT_TIME = GetAllTime_ms();
    // trans_information.SUCCESS = trans_success_ ? "true" : "false";
}

void TransactionProcess::record_detect_time(){
    detect_mu.lock();
    detect_times_.push_back(GetAllTime_ms());
    detect_mu.unlock();
}