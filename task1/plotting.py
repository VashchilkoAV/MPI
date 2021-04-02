import csv
import math
import matplotlib.pyplot as plt
import os

csv_path = "ms_bucket.csv"
f_obj = open(csv_path, "r") 
reader = csv.reader(f_obj)

names = None
for row in reader:
    names = row
    break
v = [(int(names[i]), i) for i in range(2, len(names))]
n = len(v)
v.sort()
print(v)
print("\nhello\n\n")
timestamp = []
aver = []
disp = []
diff = []
clients = []
i = 0
val_save = None
clts_save = None
for row in reader:
    val_save = [float(row[v[0][1]])] + [float(row[v[i][1]]) - float(row[v[i-1][1]]) for i in range(1, n)] + [float(row[1]) - float(row[v[n-1][1]])]
    clts_save = float(row[1])
    break
gap = 4
for row in reader:
    i+=1
    if i%gap != 0: continue
    if len(row[1])==0 : break
    #print(row[2:])
    val1 = [float(row[v[0][1]])] + [float(row[v[i][1]]) - float(row[v[i-1][1]]) for i in range(1, n)] + [float(row[1]) - float(row[v[n-1][1]])]
    val = [val1[i]-val_save[i] for i in range(n)]
    val_save = val1
    #print(*val)
    clts1 = float(row[1])
    clts = clts1 - clts_save
    clts_save = float(row[1])
    av = sum([val[i]*v[i][0] for i in range(n)])/clts
    di = sum([((v[i][0]-av)**2) * val[i] for i in range(n)])/(clts-1)
    dif = math.sqrt(di)
    aver.append(av)
    disp.append(di/10**3)
    diff.append(dif)
    clients.append(clts/10**4 * 5)
    timestamp.append(float(row[0])/10**7)
    #print(row[0], av, dif, di, "\n\n")
print(i)

print(disp[5], max(disp))

plt.plot(timestamp, clients, label= "Number of clients, 2*10^3 per gap")
plt.plot(timestamp, diff, label= "Difference, msc")
plt.plot(timestamp, aver, label= "Average, mcs")
#plt.plot(timestamp, disp, label= "Dispercy, ?")
plt.grid(True)
plt.legend()
plt.axis([timestamp[0], timestamp[-1], 0, max([max(aver), max(clients), max(diff)])*1.1])
#plt.ylabel("Request processing time, mcs")
plt.xlabel("Timestamp, 10^7")
plt.title("Server requests")

#plt.savefig("plot.png", fmt= "png")
plt.show()