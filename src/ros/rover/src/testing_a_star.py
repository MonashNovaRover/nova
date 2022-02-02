#!/usr/bin/env python3

from a_star import a_star
import numpy as np

grid = np.ones((800, 800))
a_star(grid, (0, 0), (750, 750))
