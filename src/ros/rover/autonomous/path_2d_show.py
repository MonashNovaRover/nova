#!/usr/bin/env python3

import time
from a_star import a_star
import numpy as np
import matplotlib.pyplot as plt

"""def add_obs(array):
    
    for i in range(350):
        array[50][i] = 1.0

    for i in range(100):
        array[140, 399 - i] = 1.0

    for i in range(300):
        array[300, 399 - i] = 1.0

    return array
"""
def show_path(grid, path, start, end):
    plt.imshow(arr)
    plt.plot(start[1], start[0], 'ro')
    plt.plot(end[1], end[0], 'bo')
    plt.plot(path[:,1], path[:,0])
    plt.savefig(r"path.png")

arr = np.zeros((200, 200))
p1, p2 = (0, 10), (149, 199)

#arr = add_obs(arr)

t = time.perf_counter()
path = np.array(a_star(arr, p1, p2, 10.0))
print(time.perf_counter() - t)
show_path(arr, path, p1, p2)


