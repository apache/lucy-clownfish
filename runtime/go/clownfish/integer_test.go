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

func TestIntGetValue(t *testing.T) {
	fortyTwo := NewInteger(42)
	if got := fortyTwo.GetValue(); got != 42 {
		t.Errorf("Expected 42, got %d", got)
	}
}

func TestIntToF64(t *testing.T) {
	fortyTwo := NewInteger(42)
	if got := fortyTwo.ToF64(); got != 42.0 {
		t.Errorf("Expected 42.0, got %f", got)
	}
}

func TestIntToBool(t *testing.T) {
	fortyTwo := NewInteger(42)
	if got := fortyTwo.ToBool(); !got {
		t.Errorf("Expected true, got %v", got)
	}
	zero := NewInteger(0)
	if got := zero.ToBool(); got {
		t.Errorf("Expected false, got %v", got)
	}
}

func TestIntToString(t *testing.T) {
	fortyTwo := NewInteger(42)
	if got := fortyTwo.ToString(); got != "42" {
		t.Errorf("Expected \"42\", got %s", got)
	}
}

func TestIntClone(t *testing.T) {
	fortyTwo := NewInteger(42)
	dupe := fortyTwo.Clone().(Integer)
	if got := dupe.GetValue(); got != 42 {
		t.Errorf("Expected 42, got %d", got)
	}
}

func TestIntCompareTo(t *testing.T) {
	fortyTwo := NewInteger(42)
	if got := fortyTwo.CompareTo(43); got >= 0 {
		t.Errorf("42 CompareTo 43: %v", got)
	}
	if got := fortyTwo.CompareTo(42); got != 0 {
		t.Errorf("42 CompareTo 42: %v", got)
	}
	if got := fortyTwo.CompareTo(42.1); got >= 0 {
		t.Errorf("42 CompareTo 42.1: %v", got)
	}
	if got := fortyTwo.CompareTo(41.9); got <= 0 {
		t.Errorf("42 CompareTo 41.9: %v", got)
	}
}

func TestIntEquals(t *testing.T) {
	fortyTwo := NewInteger(42)
	fortyThree := NewInteger(43)
	fortyTwoPointZero := NewFloat(42.0)
	dupe := fortyTwo.Clone().(Integer)
	if !fortyTwo.Equals(dupe) {
		t.Error("Equal Clownfish Integer")
	}
	if !fortyTwo.Equals(42) {
		t.Error("Equal Go integer")
	}
	if !fortyTwo.Equals(42.0) {
		t.Error("Equal Go float64")
	}
	if !fortyTwo.Equals(fortyTwoPointZero) {
		t.Error("Equal Clownfish Float")
	}
	if fortyTwo.Equals(fortyThree) {
		t.Error("Non-equal Clownfish Integer")
	}
	if fortyTwo.Equals(43) {
		t.Error("Non-equal Go integer")
	}
	if fortyTwo.Equals(42.1) {
		t.Error("Non-equal Go float64")
	}
	if fortyTwo.Equals("foo") {
		t.Error("Non-equal Go string")
	}
}
