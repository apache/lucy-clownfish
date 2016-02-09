import unittest
import cfc

class MyTest(unittest.TestCase):

    def testTrue(self):
        self.assertTrue(True, "True should be true")


if __name__ == '__main__':
    unittest.main()

