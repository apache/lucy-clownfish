/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package clownfish

import "testing"
import "reflect"

func TestVecPushPop(t *testing.T) {
	vec := NewVector(1)
	vec.Push("foo")
	got := vec.Pop()
	expected := "foo"
	if !reflect.DeepEqual(got, expected) {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestVecPushAllSlice(t *testing.T) {
	vec := NewVector(1)
	strings := []interface{}{"foo", "bar", "baz"}
	vec.PushAll(strings)
	got := vec.Slice(0, 3)
	if !reflect.DeepEqual(got, strings) {
		t.Errorf("Expected %v, got %v", strings, got)
	}
}

func TestVecStoreFetch(t *testing.T) {
	vec := NewVector(0)
	vec.Store(2, "foo")
	got := vec.Fetch(2)
	if !reflect.DeepEqual(got, "foo") {
		t.Errorf("Expected \"foo\", got %v", got)
	}
}

func TestVecInsert(t *testing.T) {
	vec := NewVector(0)
	vec.Push("foo")
	vec.Push("bar")
	vec.Insert(1, "between")
	expected := []interface{}{"foo", "between", "bar"}
	got := vec.Slice(0, 10)
	if !reflect.DeepEqual(got, expected) {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestVecInsertAll(t *testing.T) {
	vec := NewVector(0)
	vec.Push("foo")
	vec.Push("bar")
	vec.InsertAll(1, []interface{}{"a", "b"})
	expected := []interface{}{"foo", "a", "b", "bar"}
	got := vec.Slice(0, 10)
	if !reflect.DeepEqual(got, expected) {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestVecDelete(t *testing.T) {
	vec := NewVector(0)
	vec.PushAll([]interface{}{"a", "b", "c"})
	deleted := vec.Delete(1)
	if deleted != "b" {
		t.Errorf("Delete() returned '%v' rather than the expected \"b\"")
	}
	got := vec.Slice(0, 10)
	expected := []interface{}{"a", nil, "c"}
	if !reflect.DeepEqual(got, expected) {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestVecExcise(t *testing.T) {
	vec := NewVector(0)
	vec.PushAll([]interface{}{"a", "b", "c"})
	vec.Excise(1, 1)
	got := vec.Slice(0, 10)
	expected := []interface{}{"a", "c"}
	if !reflect.DeepEqual(got, expected) {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestVecSort(t *testing.T) {
	vec := NewVector(0)
	vec.PushAll([]interface{}{3, 1, 2})
	vec.Sort()
	got := vec.Slice(0, 10)
	expected := []interface{}{int64(1), int64(2), int64(3)}
	if !reflect.DeepEqual(got, expected) {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestVecResize(t *testing.T) {
	vec := NewVector(0)
	vec.PushAll([]interface{}{"foo"})
	vec.Resize(3)
	got := vec.Slice(0, 10)
	expected := []interface{}{"foo", nil, nil}
	if !reflect.DeepEqual(got, expected) {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestVecClone(t *testing.T) {
	vec := NewVector(0)
	charBuf := NewCharBuf(0)
	vec.Push(charBuf)
	clone := vec.Clone().(Vector)
	if clone.TOPTR() == vec.TOPTR() {
		t.Error("Clone returned self")
	}
	fetched := clone.Fetch(0).(CharBuf)
	if fetched.TOPTR() != charBuf.TOPTR() {
		t.Error("Clone should be shallow")
	}
}

func TestVecGetSize(t *testing.T) {
	vec := NewVector(10)
	size := vec.GetSize()
	if size != 0 {
		t.Errorf("Unexpected size: %v", size)
	}
	vec.Resize(5)
	size = vec.GetSize()
	if size != 5 {
		t.Errorf("Unexpected size after resizing: %v", size)
	}
}

func TestVecGetCapacity(t *testing.T) {
	vec := NewVector(10)
	cap := vec.getCapacity()
	if cap != 10 {
		t.Errorf("Unexpected capacity: %v", cap)
	}
}

func TestVecClear(t *testing.T) {
	vec := NewVector(0)
	vec.Push("foo")
	vec.Clear()
	got := vec.Slice(0, 10)
	expected := []interface{}{}
	if !reflect.DeepEqual(got, expected) {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestVecEquals(t *testing.T) {
	vec := NewVector(0)
	other := NewVector(0)
	vec.Push("foo")
	other.Push("foo")
	if !vec.Equals(other) {
		t.Error("Equals should succeed")
	}
	other.Push("bar")
	if vec.Equals(other) {
		t.Error("Equals should fail")
	}
	if !vec.Equals([]interface{}{"foo"}) {
		t.Error("Equals should succeed against a slice")
	}
	hash := NewHash(0)
	if vec.Equals(hash) {
		t.Error("Equals should return false for another Clownfish type")
	}
	if vec.Equals(1) {
		t.Error("Equals should return false for a different Go type.")
	}
}
