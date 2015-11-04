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
import "sort"

func TestHashStoreFetch(t *testing.T) {
	hash := NewHash(0)
	hash.Store("foo", "bar")
	if got, ok := hash.Fetch("foo").(string); !ok || got != "bar" {
		t.Errorf("Expected 'bar', got '%v'", got)
	}
	hash.Store("nada", nil)
	if got := hash.Fetch("nada"); got != nil {
		t.Errorf("Expected nil, got '%v'", got)
	}
}

func TestHashDelete(t *testing.T) {
	hash := NewHash(0)
	hash.Store("foo", "bar")
	got := hash.Delete("foo")
	if size := hash.GetSize(); size != 0 {
		t.Errorf("Delete failed (size %d)", size)
	}
	if val, ok := got.(string); !ok || val != "bar" {
		t.Errorf("Delete returned unexpected value: '%v'", val)
	}
}

func TestHashClear(t *testing.T) {
	hash := NewHash(0)
	hash.Store("foo", 1)
	hash.Clear()
	if size := hash.GetSize(); size != 0 {
		t.Errorf("Clear failed (size %d)", size)
	}
}

func TestHashHasKey(t *testing.T) {
	hash := NewHash(0)
	hash.Store("foo", 1)
	hash.Store("nada", nil)
	if !hash.HasKey("foo") {
		t.Errorf("HasKey returns true on success")
	}
	if hash.HasKey("bar") {
		t.Errorf("HasKey returns false when key not present")
	}
	if !hash.HasKey("nada") {
		t.Errorf("HasKey returns true for key mapped to nil")
	}
}

func TestHashKeys(t *testing.T) {
	hash := NewHash(0)
	hash.Store("a", 1)
	hash.Store("b", 1)
	keys := hash.Keys()
	sort.Strings(keys)
	expected := []string{"a", "b"}
	if !reflect.DeepEqual(keys, expected) {
		t.Errorf("Expected '%v', got '%v'", expected, keys)
	}
}

func TestHashValues(t *testing.T) {
	hash := NewHash(0)
	hash.Store("foo", "a")
	hash.Store("bar", "b")
	got := hash.Values()
	vals := make([]string, len(got))
	for i, val := range got {
		vals[i] = val.(string)
	}
	sort.Strings(vals)
	expected := []string{"a", "b"}
	if !reflect.DeepEqual(vals, expected) {
		t.Errorf("Expected '%v', got '%v'", expected, vals)
	}
}

func TestGetCapacity(t *testing.T) {
	hash := NewHash(1)
	if cap := hash.getCapacity(); cap <= 1 {
		t.Errorf("Unexpected value for getCapacity: %d", cap)
	}
}

func TestGetSize(t *testing.T) {
	hash := NewHash(0)
	if size := hash.GetSize(); size != 0 {
		t.Errorf("Unexpected value for GetSize: %d", size)
	}
	hash.Store("meep", "moop")
	if size := hash.GetSize(); size != 1 {
		t.Errorf("Unexpected value for GetSize: %d", size)
	}
}

func TestHashEquals(t *testing.T) {
	hash := NewHash(0)
	other := NewHash(0)
	hash.Store("a", "foo")
	other.Store("a", "foo")
	if !hash.Equals(other) {
		t.Error("Equals should succeed")
	}
	other.Store("b", "bar")
	if hash.Equals(other) {
		t.Error("Equals should fail")
	}
	if !hash.Equals(map[string]interface{}{"a":"foo"}) {
		t.Error("Equals should succeed against a Go map")
	}
	vec := NewVector(0)
	if hash.Equals(vec) {
		t.Error("Equals should return false for another Clownfish type")
	}
	if hash.Equals(1) {
		t.Error("Equals should return false for a different Go type.")
	}
}

func TestHashIterator(t *testing.T) {
	hash := NewHash(0)
	hash.Store("a", "foo")
	iter := NewHashIterator(hash)
	if !iter.Next() {
		t.Error("Next() should proceed")
	}
	if key := iter.GetKey(); key != "a" {
		t.Error("Expected 'a', got '%v'", key)
	}
	if val, ok := iter.GetValue().(string); !ok || val != "foo" {
		t.Error("Expected 'a', got '%v'", val)
	}
	if iter.Next() {
		t.Error("Next() should return false when iteration complete")
	}
}
