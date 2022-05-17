import numpy as np 

def Zipf(a: np.float64, min: np.uint64, max: np.uint64, size=None):
     """
     Generate Zipf-like random variables,
     but in inclusive [min...max] interval
     """
     if min == 0:
         raise ZeroDivisionError("")
     v = np.arange(min, max+1) # values to sample
     p = 1.0 / np.power(v, a)  # probabilities
     p /= np.sum(p)            # normalized

     return np.random.choice(v, size=size, replace=True, p=p)

np.random.seed(0)
#local_page = 524288
local_page = 100
min = np.uint64(1)
max = np.uint64(local_page)
print (min)
len = 100
#len = 10000000

q = Zipf(1.2, min, max, len)
print(q)
print(type(q))
print(q.__len__())
print(q[0],q[1])
f = open("Zipf.txt", "w")
#f = open("Zipf.txt", "a")
for i in range( q.__len__() ):
  f.write(str(int(q[i] - 1)))
  f.write("\n")
f.close()
print("finish\n")


