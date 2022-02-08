#!/usr/bin/env python3

import matplotlib.pyplot as plt

def show_path(grid, path, start, end):
    plt.imshow(grid)
    plt.plot(100 + 10 * start[1], 100 + 10 * start[0], 'ro')
    plt.plot(100 + 10*end[1], 100 + 10 * end[0], 'bo')
    plt.plot(path[:,1], path[:,0])
    plt.savefig("path.png")


