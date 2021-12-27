#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import os.path
import sys
import time
import subprocess


#Experiment 1: both exponential distributions
for row_number in range(410, 591, 10): #no need to store data
#for row_number in range(600, 1001, 100):
    print("%d".ljust(100, '-') % row_number)
    cmd = "./LCL_both_exps %d 2>&1 >> SQL_res_exps.log" % row_number
    pid = os.popen(cmd).read()

dstPath = "SQL_res_exps"
if os.path.exists(dstPath) == False:
    os.makedirs(dstPath)

cmd = "mv *.csv %s" % (dstPath)
pid = os.popen(cmd).read()

#Experiment 2: SQL exp distribution and resource normal distribution
for row_number in range(410, 591, 10): #no need to store data
#for row_number in range(600, 1001, 100):
    print("Experiment 2: %d".ljust(100, '-') % row_number)
    cmd = "./LCL_SQL_exp_res_normal %d 2>&1 >> SQL_exp_res_normal.log" % row_number
    pid = os.popen(cmd).read()

dstPath = "SQL_exp_res_normal"
if os.path.exists(dstPath) == False:
    os.makedirs(dstPath)

cmd = "mv *.csv %s" % (dstPath)
pid = os.popen(cmd).read()

#Experiment 3: SQL normal distribution and resource exp distribution
for row_number in range(410, 591, 10): #no need to store data
#for row_number in range(600, 1001, 100):
    print("Experiment 3: %d".ljust(100, '-') % row_number)
    cmd = "./LCL_SQL_normal_res_exp %d 2>&1 >> SQL_normal_res_exp.log" % row_number
    pid = os.popen(cmd).read()

dstPath = "SQL_normal_res_exp"
if os.path.exists(dstPath) == False:
    os.makedirs(dstPath)

cmd = "mv *.csv %s" % (dstPath)
pid = os.popen(cmd).read()

#Experiment 4: SQL normal distribution and resource normal distributions
for row_number in range(490, 591, 10): #no need to store data
#for row_number in range(600, 1001, 100):
    print("Experiment 4: %d".ljust(100, '-') % row_number)
    cmd = "./LCL_both_normals %d 2>&1 >> SQL_res_normals.log" % row_number
    pid = os.popen(cmd).read()

dstPath = "SQL_res_normals"
if os.path.exists(dstPath) == False:
    os.makedirs(dstPath)

cmd = "mv *.csv %s" % (dstPath)
pid = os.popen(cmd).read()

print("Game over!")
