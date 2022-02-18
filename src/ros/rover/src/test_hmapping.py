#!/usr/bin/env python3

from height_mapper import get_obstacles
import numpy as np
import matplotlib.pyplot as plt
from time import perf_counter as t

if __name__=="__main__":

    map3d = np.array([[[i, j, i // 40] for i in range(400)] for j in range(400)]).reshape(160000, 3)

    begin = t()
    map2d = get_obstacles(map3d, 400, 400)
    end = t()

    plt.imshow(map2d)
    print("python: took " + str(end - begin) + " s")
    plt.savefig("../debug/python_map.png")
