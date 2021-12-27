import time
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import make_interp_spline
from matplotlib.ticker import FuncFormatter

maxtime=350000
savepath="../../LCL_data/eps/1000_Transactions_DeadLock_time.eps"
print("start")
commit_reader = open("../../LCL_data/test_data/1000_commit_times.csv")

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


print(len(commitTime))
print("max commitTime is: "+repr(max(commitTime)))


def formatnum(x, pos):
    return format(x, '.1e')

formatter = FuncFormatter(formatnum)


histarray = np.arange(0, maxtime+1000, 1000)

fig, ax2 = plt.subplots()

ax2.set_xlabel("Elapsed time (in seconds)")

hists2, bins2 = np.histogram(commitTime, bins=histarray, density=False)
cdf2 = np.insert(hists2.cumsum(), 0, 0)
print(sum(hists2))
model2 = make_interp_spline(bins2, cdf2)
xs2 = np.linspace(np.min(bins2), np.max(bins2), 1000)

ys2 = model2(xs2)
ax2.yaxis.set_label_position("right")
ax2.yaxis.tick_right()
ax2.plot(histarray/1000,cdf2,color="#F68657",label="Transactions",ls='-')
ax2.yaxis.set_major_formatter(formatter)
ax2.set_ylim(0)
ax2.set_ylabel("Number of transactions")


fig.legend(loc="upper left", bbox_to_anchor=(0, 1), bbox_transform=ax2.transAxes)
fig.tight_layout()

plt.ylim(0)
plt.xlim(0,maxtime/1000)
plt.savefig(savepath, format='eps', dpi=1000, bbox_inches='tight')

# plt.show()
print("Output Finish")
