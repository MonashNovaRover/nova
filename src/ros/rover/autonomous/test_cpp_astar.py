#!/usr/bin/env python3

import time
from a_star import a_star
import numpy as np
import matplotlib.pyplot as plt

def add_obs(array):
    
    for i in range(20, 380):
        array[100, i] = 0.0

    for i in range(100):
        array[140, 399 - i] = 0.0

    for i in range(399):
        array[50, i] = 0.0
    
    for i in range(360):
        array[300, 399 - i] = 0.0

    return array

arr = np.ones((400, 400))
p1, p2 = (0, 0), (399, 399)

arr = add_obs(arr)

t = time.perf_counter()
a_star(arr, p1, p2)
print(time.perf_counter() - t)

plt.imshow(arr)
plt.plot(0, 0, 'ro')
plt.plot(399, 399, 'bo')
plt.savefig("obstacles.png")
