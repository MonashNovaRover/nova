#!/usr/bin/env python3
import unittest
from autonomous.math_utils.controller_math import *
import numpy as np

PI = np.pi

class ControllerMathTest(unittest.TestCase):
    def test_yaw_difference(self):
        firsts = np.linspace(0, 2*np.pi, 13)
        seconds = np.linspace(0 ,2*np.pi, 13)

        for a in firsts:
            for b in seconds:
                vec_a = np.array((np.cos(a), np.sin(a), 0))
                vec_b = np.array((np.cos(b), np.sin(b), 0))
                difference_magnitude = np.abs(a - b)
                if a == b:
                    self.assertAlmostEqual(yaw_difference(vec_a, vec_b), 0)
                elif a > b and np.round(difference_magnitude, 10) < np.round(PI, 10):
                    self.assertAlmostEqual(yaw_difference(vec_a, vec_b), -difference_magnitude)
                    self.assertAlmostEqual(yaw_difference(vec_b, vec_a), difference_magnitude)
                elif a > b and np.round(difference_magnitude, 10) > np.round(PI, 10):
                    self.assertAlmostEqual(yaw_difference(vec_a, vec_b), 2 * PI - difference_magnitude)
                    self.assertAlmostEqual(yaw_difference(vec_b, vec_a),-2 * PI + difference_magnitude)
                elif np.round(difference_magnitude, 10) == np.round(PI, 10):
                    self.assertAlmostEqual(yaw_difference(vec_a, vec_b), PI)
                    self.assertAlmostEqual(yaw_difference(vec_b, vec_a), PI)


if __name__ == "__main__":
    # running all unit tests
    unittest.main()
