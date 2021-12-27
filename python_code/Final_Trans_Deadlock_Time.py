import time
import matplotlib.ticker as ticker
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import make_interp_spline
from matplotlib.ticker import FuncFormatter

maxtime=350000
savepath="../../LCL_data/eps/SQL_res_exps/1000_Transactions_DeadLock_time.eps"
commit_reader = open("../Doit_LCL_Graph_Commit/build/SQL_res_exps/final/commit_times_1000_.csv")
commit_lines = commit_reader.readlines()

commitTime = []
for line in commit_lines:
    numlist=line.split(',')
    for data in numlist:
        if data:
            commitTime.append(int(data))
end = time.time()
commit_reader.close()
print("commit finish")

detect_reader = open("../Doit_LCL_Graph_Commit/build/SQL_res_exps/final/detect_times_1000_.csv")
detect_lines = detect_reader.readlines()

detectTime = []
for line in detect_lines:
    numlist=line.split(",")
    for data in numlist:
        if data:
            detectTime.append(int(data))
end = time.time()
detect_reader.close()
print("detect finish")

print(len(commitTime))
print("max commitTime is: "+repr(max(commitTime)))
print(len(detectTime))

detectTime=np.array(detectTime)

def formatnum(x, pos):
    return format(x,'.1e')

formatter = FuncFormatter(formatnum)


histarray = np.arange(0, maxtime+1000, 1000)
hists, bins = np.histogram(detectTime, bins=histarray, density=False)
cdf=np.insert(hists.cumsum(), 0, 0)

model = make_interp_spline(bins, cdf)
xs = np.linspace(np.min(bins), np.max(bins), 1000)

ys = model(xs)

fig, ax1 = plt.subplots()

ax1.plot(histarray/1000,cdf,color="#4F86C6",label="Detected deadlocks",ls='--')
ax1.set_xlabel("Elapsed time (in seconds)")
ax1.yaxis.set_major_locator(ticker.MaxNLocator(integer=True))
ax1.set_ylim(0)
ax1.set_ylabel("Number of Detected deadlocks")
#plt.axis(ymin=0, ymax=140)  # SQL normal and resource exp distributions
#plt.yticks([y for y in range(0, 201, 20)])
#plt.axis(ymin=0, ymax=16)  # SQL and resource uniform distributions
#plt.yticks([y for y in range(0, 17, 2)])

plt.axis(ymin=0, ymax=220)  # SQL and resource exp distributions
plt.yticks([y for y in range(0, 221, 20)])

#plt.axis(ymin=0, ymax=300)  # SQL and resource normal distributions
#plt.yticks([y for y in range(0, 301, 30)])

#plt.axis(ymin=0, ymax=380)  # SQL_exp_res_normal
#plt.yticks([y for y in range(0, 381, 40)])

#plt.axis(ymin=0, ymax=30)  # SQL normal and resource uniform distributions
#plt.yticks([y for y in range(0, 31, 5)])


hists2, bins2 = np.histogram(commitTime, bins=histarray, density=False)
cdf2=np.insert(hists2.cumsum(), 0, 0)
print(sum(hists2))
model2 = make_interp_spline(bins2, cdf2)
xs2 = np.linspace(np.min(bins2), np.max(bins2), 1000)

ys2 = model2(xs2)

ax2 = ax1.twinx()

ax2.plot(histarray/1000,cdf2,color="#F68657",label="Transactions",ls='-')
ax2.yaxis.set_major_formatter(formatter)
ax2.set_ylim(0)
plt.axis(ymin=0, ymax=900000000)  # SQL_res_exps
plt.yticks([y for y in range(0, 900000001, 100000000)])

#plt.axis(ymin=0, ymax=900000000)  # SQL_exp_res_normal
#plt.yticks([y for y in range(0, 900000001, 100000000)])

#plt.axis(ymin=0, ymax=700000000)  # SQL_res_normals
#plt.yticks([y for y in range(0, 700000001, 100000000)])

#plt.axis(ymin=0, ymax=600000000)  # SQL_normal_res_exp
#plt.yticks([y for y in range(0, 600000001, 100000000)])
ax2.set_ylabel("Number of transactions")

fig.legend(loc="upper left", bbox_to_anchor=(0, 1), bbox_transform=ax1.transAxes)
fig.tight_layout()
plt.grid(axis='x',linestyle='--')

plt.ylim(0)
plt.xlim(0,maxtime/1000)

plt.savefig(savepath, format='eps', dpi=1000, bbox_inches='tight')

# plt.show()
print("Ouput finish")
