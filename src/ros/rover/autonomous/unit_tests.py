#!/usr/bin/env python3
import unittest
from controller_math import *
from controller import Controller
import numpy as np

PI = np.pi

class MathTest(unittest.TestCase):
    def test_desired_heading(self):
        start = (0, 0)
        ends = [(1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1)]
        results = np.linspace(0, 1.75, 8) * PI
        backwards = [results[i%8] for i in range(4, 12)]

        for i, end in enumerate(ends):
            self.assertTrue(desired_heading(start, end) == results[i])
            self.assertTrue(desired_heading(end, start) == backwards[i])

    def test_desired_heading_same_points(self):
        starts = [(1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1)]
        ends = starts 

        for i, (end, start) in enumerate(zip(ends, starts)):
            with self.assertRaises(ValueError):
                desired_heading(start, end)


if __name__ == "__main__":
    # running all unit tests
    unittest.main()