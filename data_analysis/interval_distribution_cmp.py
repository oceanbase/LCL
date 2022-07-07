#!/usr/bin/python
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt
import numpy as np

def addlabels(x,y, step):
    for i in range(len(x)):
        if y[i] > 100:
            plt.text(x[i]-step*0.02,y[i],y[i], fontsize=8)
        elif y[i] > 10:
            plt.text(x[i]+step*0.05,y[i],y[i], fontsize=8)
        elif y[i] > 0:
            # plt.text(x[i]+step*0.20,y[i],y[i], fontsize=10)
            plt.text(x[i]+step*0.10,y[i],y[i], fontsize=8)


def plot_distribution_cmp(row_number):
    # savePath = "/data/DeadLockArchive/LCL_data/mm_eps_cmp/SQL_res_exps/%d_distribution_cmp.eps" % (row_number)
    # deadlockFile = open("/data/DeadLockArchive/mm_data/build/SQL_res_exps/deadlock_loops_%d_.csv" % (row_number))  

    savePath = "/data/DeadLockArchive/LCL_data/mm_eps_cmp/SQL_exp_res_normal/%d_distribution_cmp.eps" % (row_number)
    deadlockFile = open("/data/DeadLockArchive/mm_data/build/SQL_exp_res_normal/deadlock_loops_%d_.csv" % (row_number)) 

    # savePath = "/data/DeadLockArchive/LCL_data/mm_eps_cmp/SQL_normal_res_exp/%d_distribution_cmp.eps" % (row_number)
    # deadlockFile = open("/data/DeadLockArchive/mm_data/build/SQL_normal_res_exp/deadlock_loops_%d_.csv" % (row_number))  

    # savePath = "/data/DeadLockArchive/LCL_data/mm_eps_cmp/SQL_res_normals/%d_distribution_cmp.eps" % (row_number)
    # deadlockFile = open("/data/DeadLockArchive/mm_data/build/SQL_res_normals/deadlock_loops_%d_.csv" % (row_number))  

 

    deadlocks = []
    for line in deadlockFile:
        numList = line.split()
        if len(numList) > 0:
            deadlocks.append(int(numList[1]))

    
    maxV = 10
    step = 1
    if len(deadlocks) > 0:
        if max(deadlocks) >= 10:
            maxV = (max(deadlocks) // 10 + 1 + 1) * 10
        else:
            maxV = (max(deadlocks) // 10 + 1) * 10
        step = (max(deadlocks)//10+1)
        print("maxV=%d, step=%d" % (maxV, step))
    # maxV = 16
    # step = 2

    # deadlockFile2 = open("/data/DeadLockArchive/LCL_code/Doit_LCL_Graph_Commit/build/SQL_res_exps/final/deadlock_loops_%d_.csv" % (row_number)) 

    deadlockFile2 = open("/data/DeadLockArchive/LCL_code/Doit_LCL_Graph_Commit/build/SQL_exp_res_normal/final/deadlock_loops_%d_.csv" % (row_number))  

    # deadlockFile2 = open("/data/DeadLockArchive/LCL_code/Doit_LCL_Graph_Commit/build/SQL_normal_res_exp/final/deadlock_loops_%d_.csv" % (row_number))  

    # deadlockFile2 = open("/data/DeadLockArchive/LCL_code/Doit_LCL_Graph_Commit/build/SQL_res_normals/final/deadlock_loops_%d_.csv" % (row_number))  
 

    deadlocks2 = []
    for line in deadlockFile2:
        numList = line.split()
        if len(numList) > 0:
            deadlocks2.append(int(numList[1]))

    
    if len(deadlocks2) > 0:
        if max(deadlocks2) >= 10:
            maxV = max((max(deadlocks2) // 10 + 1 + 1) * 10,maxV)
        else:
            maxV = max((max(deadlocks2) // 10 + 1) * 10,maxV)
        step = max((max(deadlocks2)//10+1),step)
        print("maxV=%d, step=%d" % (maxV, step))

    histArray = np.arange(0, maxV, step)
    test = np.arange(0, maxV, step)
    n, bins, patches = plt.hist([deadlocks,deadlocks2], bins=histArray, rwidth=0.8,label=['M&M','LCL'])
    plt.xticks(histArray)  


    hist_labels2 = []
    for i, x in enumerate(histArray):
        #if i == len(histArray)-1: break
        print('i=%d, x=%d' % (i, x))
        tmp = 0
        for d in deadlocks2:
            if d >= histArray[i] and d < histArray[i+1]:
                tmp = tmp + 1
        hist_labels2.append(tmp)

    hist_x2 = []
    for p in patches[1]:
        hist_x2.append(p.get_xy()[0])


    addlabels(hist_x2, hist_labels2, step)
    

    hist_labels = []
    for i, x in enumerate(histArray):
        #if i == len(histArray)-1: break
        print('i=%d, x=%d' % (i, x))
        tmp = 0
        for d in deadlocks:
            if d >= histArray[i] and d < histArray[i+1]:
                tmp = tmp + 1
        hist_labels.append(tmp)

    hist_x = []
    for p in patches[0]:
        #print(p.get_xy())
        hist_x.append(p.get_xy()[0])


    addlabels(hist_x, hist_labels, step)


    plt.xticks(fontsize=16)
    plt.yticks(fontsize=16)
 
    plt.xlabel("DeadLock Cycle Length", fontsize=16)
    plt.ylabel("Number of Deadlocks", fontsize=16)

    # plt.axis(ymin=0, ymax=310)  # SQL and resource exponential distributions
    # plt.yticks([y for y in range(0, 311, 50)])

    plt.axis(ymin=0, ymax=350)  # SQL exponential and resource normal distributions
    plt.yticks([y for y in range(0, 351, 50)])       

    # plt.axis(ymin=0, ymax=400)  # SQL normal and resource exponential distributions
    # plt.yticks([y for y in range(0, 401, 80)])

    # plt.axis(ymin=0, ymax=240) #SQL and resource normal distributions
    # plt.yticks([y for y in range(0, 241, 40)])
    plt.legend(loc='upper right')

    plt.savefig(savePath, format='eps', dpi=1000, bbox_inches='tight')
    #plt.show()
    plt.close('all')

if __name__ == "__main__":
    for row_number in range(400, 1001, 100):
        print('row_number=%d' % (row_number))
        plot_distribution_cmp(row_number)
    print("Game Over!")
