#!/usr/bin/python
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt
import numpy as np

def plot_distribution(row_number):
    #savePath = "../../LCL_data/eps/SQL_normal_res_exp/%d_distribution.eps" % (row_number)
    #deadlockFile = open("../Doit_LCL_Graph_Commit/build/SQL_normal_res_exp/final/deadlock_loops_%d_.csv" % (row_number))  

    savePath = "../../LCL_data/eps/SQL_res_exps/%d_distribution.eps" % (row_number)
    deadlockFile = open("../Doit_LCL_Graph_Commit/build/SQL_res_exps/final/deadlock_loops_%d_.csv" % (row_number))  

    #savePath = "../../LCL_data/eps/SQL_exp_res_normal/%d_distribution.eps" % (row_number)
    #deadlockFile = open("../Doit_LCL_Graph_Commit/build/SQL_exp_res_normal/final/deadlock_loops_%d_.csv" % (row_number)) 

    #savePath = "../../LCL_data/eps/SQL_res_normals/%d_distribution.eps" % (row_number)
    #deadlockFile = open("../Doit_LCL_Graph_Commit/build/SQL_res_normals/final/deadlock_loops_%d_.csv" % (row_number))  
    
    deadlocks = []
    for line in deadlockFile:
        numList = line.split()
        if len(numList) > 0:
            deadlocks.append(int(numList[1]))

    print("Number of deadlocks: "+repr(len(deadlocks)))
    if len(deadlocks) > 0:
        print("Minimum number of deadlock nodes: " + repr(min(deadlocks)))
        print("Maximum number of deadlocked nodes: " + repr(max(deadlocks)))
        print("Average number of deadlocked nodes: " + repr(np.mean(deadlocks)))
        print(deadlocks)
        print(max(deadlocks))
        print((max(deadlocks)//10+1)*10)
    
    maxV = 10
    step = 1
    if len(deadlocks) > 0:
        if max(deadlocks) >= 10:
            maxV = (max(deadlocks) // 10 + 1 + 1) * 10
        else:
            maxV = (max(deadlocks) // 10 + 1) * 10
        step = (max(deadlocks)//10+1)
        print("maxV=%d, step=%d" % (maxV, step))

    histArray = np.arange(0, maxV, step)
    test = np.arange(0, maxV, step)
    n, bins, patches = plt.hist(deadlocks, bins=histArray, rwidth=0.8)
    plt.xticks(histArray)  

    plt.xlabel("DeadLock Cycle Length")
    plt.ylabel("Number of Deadlocks")

    #plt.axis(ymin=0, ymax=120)  # SQL normal and resource exponential distributions
    #plt.yticks([y for y in range(0, 121, 20)])

    plt.axis(ymin=0, ymax=110)  # SQL and resource exponential distributions
    plt.yticks([y for y in range(0, 111, 10)])

    #plt.axis(ymin=0, ymax=320)  # SQL exponential and resource normal distributions
    #plt.yticks([y for y in range(0, 321, 50)])    
    plt.savefig(savePath, format='eps', dpi=1000, bbox_inches='tight')
    #plt.show()
    plt.close('all')

if __name__ == "__main__":
    for row_number in [400, 600, 900, 1000]:#range(400, 1000, 100): #
        print('row_number=%d' % (row_number))
        plot_distribution(row_number)
    print("Game Over!")
