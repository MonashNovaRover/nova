__package__ = "autonomous"
#!/usr/bin/env python3
import unittest
from math_utils.controller_math import *
from controller.controller import Controller
import numpy as np

PI = np.pi

class ControllerTest(unittest.TestCase):
    def test_init(self):
        self.controller = Controller()
        self.assert


if __name__ == "__main__":
    # running all unit tests
    unittest.main()
