import unittest

from can_str_msg_parser import parse

class TestParser(unittest.TestCase):
    def test_succeeds(self):
        cases = [
            "000#00"
            "0A0#FF"
            "0B0#FF00"
            "0A1#65"
            "1A1#11"
            "0F3#01FF"
            "0F3#01FFFF"
            "0F3#01FFFFFF"
            "0F3#01FFFFFFFF"
            "0F3#01FFFFFFFFFF"
        ]

        for case in cases:
            self.assertEqual(case, str(parse(case)))


if __name__ == '__main__':
    unittest.main()
