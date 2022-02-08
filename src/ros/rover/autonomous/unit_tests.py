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

    def test_yaw_difference(self):
        firsts = np.linspace(0, 2*np.pi, 13)
        seconds = np.linspace(0 ,2*np.pi, 13)

        for i, a in enumerate(firsts):
            for b in seconds:
                difference_magnitude = np.abs(a - b)
                if a == b:
                    self.assertAlmostEqual(yaw_difference(a, b), 0)
                elif a > b and np.round(difference_magnitude, 10) < np.round(PI, 10):
                    self.assertAlmostEqual(yaw_difference(a, b), -difference_magnitude)
                    self.assertAlmostEqual(yaw_difference(b, a), difference_magnitude)
                elif a > b and np.round(difference_magnitude, 10) > np.round(PI, 10):
                    self.assertAlmostEqual(yaw_difference(a, b), 2 * PI - difference_magnitude)
                    self.assertAlmostEqual(yaw_difference(b, a),-2 * PI + difference_magnitude)
                elif np.round(difference_magnitude, 10) == np.round(PI, 10):
                    self.assertAlmostEqual(yaw_difference(a, b), PI)
                    self.assertAlmostEqual(yaw_difference(b, a), PI)

    def test_yaw_delta_size(self):
        firsts = np.linspace(0, 2*np.pi, 13)
        seconds = np.linspace(0 ,2*np.pi, 13)

        for i, a in enumerate(firsts):
            for b in seconds:
                difference_magnitude = np.abs(a - b)
                if a == b:
                    self.assertAlmostEqual(yaw_delta_size(a, b), 0)
                elif a > b and np.round(difference_magnitude, 10) < np.round(PI, 10):
                    self.assertAlmostEqual(yaw_delta_size(a, b), difference_magnitude)
                    self.assertAlmostEqual(yaw_delta_size(b, a), difference_magnitude)
                elif a > b and np.round(difference_magnitude, 10) > np.round(PI, 10):
                    self.assertAlmostEqual(yaw_delta_size(a, b), 2 * PI - difference_magnitude)
                    self.assertAlmostEqual(yaw_delta_size(b, a), 2 * PI + difference_magnitude)
                elif np.round(difference_magnitude, 10) == np.round(PI, 10):
                    self.assertAlmostEqual(yaw_delta_size(a, b), PI)
                    self.assertAlmostEqual(yaw_delta_size(b, a), PI)


if __name__ == "__main__":
    # running all unit tests
    unittest.main()