# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import unittest
import inspect
import clownfish

class TestHash(unittest.TestCase):

    def testStoreFetch(self):
        h = clownfish.Hash()
        h.store("foo", "bar")
        h.store("foo", "bar")
        self.assertEqual(h.fetch("foo"), "bar")
        h.store("nada", None)
        self.assertEqual(h.fetch("nada"), None)

    def testDelete(self):
        h = clownfish.Hash()
        h.store("foo", "bar")
        got = h.delete("foo")
        self.assertEqual(h.get_size(), 0)
        self.assertEqual(got, "bar")

    def testClear(self):
        h = clownfish.Hash()
        h.store("foo", 1)
        h.clear()
        self.assertEqual(h.get_size(), 0)

    def testHasKey(self):
        h = clownfish.Hash()
        h.store("foo", 1)
        h.store("nada", None)
        self.assertTrue(h.has_key("foo"))
        self.assertFalse(h.has_key("bar"))
        self.assertTrue(h.has_key("nada"))

    def testKeys(self):
        h = clownfish.Hash()
        h.store("a", 1)
        h.store("b", 1)
        keys = sorted(h.keys())
        self.assertEqual(keys, ["a", "b"])

    def testValues(self):
        h = clownfish.Hash()
        h.store("foo", "a")
        h.store("bar", "b")
        got = sorted(h.values())
        self.assertEqual(got, ["a", "b"])

    def testGetCapacity(self):
        h = clownfish.Hash(capacity=1)
        self.assertGreater(h.get_capacity(), 0)

    def testGetSize(self):
        h = clownfish.Hash()
        self.assertEqual(h.get_size(), 0)
        h.store("meep", "moop")
        self.assertEqual(h.get_size(), 1)

    def testEquals(self):
        h = clownfish.Hash()
        other = clownfish.Hash()
        h.store("a", "foo")
        other.store("a", "foo")
        self.assertTrue(h.equals(other))
        other.store("b", "bar")
        self.assertFalse(h.equals(other))
        self.assertTrue(h.equals({"a":"foo"}),
                        "equals() true against a Python dict")
        vec = clownfish.Vector()
        self.assertFalse(h.equals(vec),
                         "equals() false against conflicting Clownfish type")
        self.assertFalse(h.equals(1),
                         "equals() false against conflicting Python type")

    def testIterator(self):
        h = clownfish.Hash()
        h.store("a", "foo")
        i = clownfish.HashIterator(h)
        self.assertTrue(i.next())
        self.assertEqual(i.get_key(), "a")
        self.assertEqual(i.get_value(), "foo")
        self.assertFalse(i.next())

if __name__ == '__main__':
    unittest.main()

