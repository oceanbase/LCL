import time
import matplotlib.ticker as ticker
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import make_interp_spline
from matplotlib.ticker import FuncFormatter


def plot_Trans_Deadlock_cmp(row_number):
    maxtime=350000

    savepath="../../LCL_data/mm_eps_cmp/SQL_normal_res_exp/%d_Transactions_DeadLock_time_cmp.eps"% (row_number)

    #### M&M Path   

    commit_reader = open("/data/DeadLockArchive/mm_data/build/SQL_normal_res_exp/commit_times_%d_.csv" %(row_number))
       
    commit_lines = commit_reader.readlines()

    commitTime = []
    for line in commit_lines:
        numlist=line.split(',')
        for data in numlist:
            if data:
                commitTime.append(int(data))
    end = time.time()
    commit_reader.close()
    print("M&M commit finish")
    
    detect_reader = open("/data/DeadLockArchive/mm_data/build/SQL_normal_res_exp/detect_times_%d_.csv"%(row_number))

    detect_lines = detect_reader.readlines()

    detectTime = []
    for line in detect_lines:
        numlist=line.split(",")
        for data in numlist:
            if data:
                detectTime.append(int(data))
    end = time.time()
    detect_reader.close()
    print("M&M detect finish")



    #### LCL Path

    commit_reader_LCL = open("/data/DeadLockArchive/LCL_code/Doit_LCL_Graph_Commit/build/SQL_normal_res_exp/final/commit_times_%d_.csv" %(row_number))

    commit_lines_LCL = commit_reader_LCL.readlines()

    commitTime_LCL = []
    for line in commit_lines_LCL:
        numlist=line.split(',')
        for data in numlist:
            if data:
                commitTime_LCL.append(int(data))
    end = time.time()
    commit_reader_LCL.close()
    print("LCL commit finish")

    detect_reader_LCL = open("/data/DeadLockArchive/LCL_code/Doit_LCL_Graph_Commit/build/SQL_normal_res_exp/final/detect_times_%d_.csv"%(row_number))

    detect_lines_LCL = detect_reader_LCL.readlines()

    detectTime_LCL = []
    for line in detect_lines_LCL:
        numlist=line.split(",")
        for data in numlist:
            if data:
                detectTime_LCL.append(int(data))
    end = time.time()
    detect_reader_LCL.close()
    print("LCL detect finish")


    detectTime=np.array(detectTime)
    detectTime_LCL=np.array(detectTime_LCL)

    def formatnum(x, pos):
        #return '$%.1f$x$10^{4}$' % (x/1000)
        return format(x,'.1e')
    formatter = FuncFormatter(formatnum)

    histarray = np.arange(0, maxtime+1000, 1000)

    #### M&M Detected deadlocks
    
    hists, bins = np.histogram(detectTime, bins=histarray, density=False)
    cdf=np.insert(hists.cumsum(), 0, 0)

    model = make_interp_spline(bins, cdf)
    xs = np.linspace(np.min(bins), np.max(bins), 1000)

    ys = model(xs)

    fig, ax1 = plt.subplots()
    plt.rcParams.update({'font.size': 12})
    plt.xticks(fontsize=12)
    ax1.plot(histarray/1000,cdf,color="#4F86C6",label="M&M Detected deadlocks",ls='--')

    ax1.set_xlabel("Elapsed time (in seconds)", fontsize=16)
    ax1.yaxis.set_major_locator(ticker.MaxNLocator(integer=True))
    ax1.set_ylim(0)
    ax1.set_ylabel("Number of Detected deadlocks", fontsize=16)

    #plt.axis(ymin=0, ymax=300)  # SQL and resource exp distributions
    #plt.yticks([y for y in range(0, 301, 50)], fontsize=12)

    # plt.axis(ymin=0, ymax=400)  # SQL_exp_res_normal
    # plt.yticks([y for y in range(0, 401, 50)])

    plt.axis(ymin=0, ymax=400)  # SQL normal and resource exp distributions
    plt.yticks([y for y in range(0, 401, 50)])

    # plt.axis(ymin=0, ymax=300)  # SQL and resource normal distributions
    # plt.yticks([y for y in range(0, 301, 50)])


    #### LCL Detected deadlocks
    hists_LCL, bins_LCL = np.histogram(detectTime_LCL, bins=histarray, density=False)
    cdf_LCL=np.insert(hists_LCL.cumsum(), 0, 0)
    model_LCL = make_interp_spline(bins_LCL, cdf_LCL)
    xs_LCL = np.linspace(np.min(bins_LCL), np.max(bins_LCL), 1000)

    ys_LCL = model(xs_LCL)

    ax1.plot(histarray/1000,cdf_LCL,color="#DFC5FE",label="LCL Detected deadlocks",ls=':')


    #### M&M Transactions
    hists2, bins2 = np.histogram(commitTime, bins=histarray, density=False)
    cdf2=np.insert(hists2.cumsum(), 0, 0)
    print(sum(hists2))
    model2 = make_interp_spline(bins2, cdf2)
    xs2 = np.linspace(np.min(bins2), np.max(bins2), 1000)

    ys2 = model2(xs2)


    ax2 = ax1.twinx()

    ax2.plot(histarray/1000,cdf2,color="#F68657",label="M&M Transactions",ls='-')

    ax2.yaxis.set_major_formatter(formatter)
    ax2.set_ylim(0)
    # plt.axis(ymin=0, ymax=900000000)  # SQL_res_exps
    # plt.yticks([y for y in range(0, 900000001, 100000000)], fontsize=12)


    # plt.axis(ymin=0, ymax=900000000)  # SQL_exp_res_normal
    # plt.yticks([y for y in range(0, 900000001, 100000000)])

    plt.axis(ymin=0, ymax=700000000)  # SQL_normal_res_exp
    plt.yticks([y for y in range(0, 700000001, 100000000)])

    # plt.axis(ymin=0, ymax=700000000)  # SQL_res_normals
    # plt.yticks([y for y in range(0, 700000001, 100000000)])

    ax2.set_ylabel("Number of transactions", fontsize=16)


    #### LCL Transactions
    hists_LCL2, bins_LCL2 = np.histogram(commitTime_LCL, bins=histarray, density=False)
    cdf_LCL2=np.insert(hists_LCL2.cumsum(), 0, 0)
    print(sum(hists_LCL2))
    model_LCL2 = make_interp_spline(bins_LCL2, cdf_LCL2)
    xs_LCL2 = np.linspace(np.min(bins_LCL2), np.max(bins_LCL2), 1000)

    ys_LCL2 = model2(xs2)

    ax2.plot(histarray/1000,cdf_LCL2,color="#5D1451",label="LCL Transactions",ls='-.')



    #### set plt
    fig.legend(loc="upper left", bbox_to_anchor=(0, 1), bbox_transform=ax1.transAxes)
    fig.tight_layout()

    plt.grid(axis='x',linestyle='--')

    plt.ylim(0)
    plt.xlim(0,maxtime/1000)

    # plt.grid()
    plt.savefig(savepath, format='eps', dpi=1000, bbox_inches='tight')

    # plt.show()
    print("Ouput finish")



if __name__ == "__main__":
    for row_number in range(400, 1001, 100):
        print('row_number=%d' % (row_number))
        plot_Trans_Deadlock_cmp(row_number)
    print("Game Over!")