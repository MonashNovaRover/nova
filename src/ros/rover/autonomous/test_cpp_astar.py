#!/usr/bin/env python3

import time
from a_star import a_star
import numpy as np
import matplotlib.pyplot as plt

def add_obs(array):
    
    for i in range(100):
        array[140, 399 - i] = 1.0

    for i in range(300):
        array[300, 399 - i] = 1.0

    return array

arr = np.zeros((400, 400))
p1, p2 = (0, 0), (399, 399)

arr = add_obs(arr)

t = time.perf_counter()
path = np.array(a_star(arr, p1, p2, 5.0))
print(time.perf_counter() - t)

plt.imshow(arr)
plt.plot(0, 0, 'ro')
plt.plot(399, 399, 'bo')
plt.plot(path[:,1], path[:,0])
plt.savefig("obstacles.png")

plt.show()
