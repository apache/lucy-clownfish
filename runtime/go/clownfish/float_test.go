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

func TestFloatGetValue(t *testing.T) {
	num := NewFloat(2.5)
	if got := num.GetValue(); got != 2.5 {
		t.Errorf("Expected 2.5, got %f", got)
	}
}

func TestFloatToI64(t *testing.T) {
	num := NewFloat(2.5)
	if got := num.ToI64(); got != 2 {
		t.Errorf("Expected 2, got %d", got)
	}
}

func TestFloatToString(t *testing.T) {
	num := NewFloat(2.5)
	if got := num.ToString(); got != "2.5" {
		t.Errorf("Expected \"2.5\", got %s", got)
	}
}

func TestFloatClone(t *testing.T) {
	num := NewFloat(2.5)
	dupe := num.Clone().(Float)
	if got := dupe.GetValue(); got != 2.5 {
		t.Errorf("Expected 2.5, got %f", got)
	}
}

func TestFloatCompareTo(t *testing.T) {
	num := NewFloat(2.5)
	if got := num.CompareTo(3.5); got >= 0 {
		t.Errorf("2.5 CompareTo 3.5: %v", got)
	}
	if got := num.CompareTo(2.5); got != 0 {
		t.Errorf("2.5 CompareTo 2.5: %v", got)
	}
	if got := num.CompareTo(2); got <= 0 {
		t.Errorf("2.5 CompareTo 2: %v", got)
	}
	if got := num.CompareTo(-1.1); got <= 0 {
		t.Errorf("2.5 CompareTo -1.1: %v", got)
	}
}

func TestFloatEquals(t *testing.T) {
	num := NewFloat(2.5)
	bigger := NewFloat(3.5)
	dupe := num.Clone().(Float)
	if !num.Equals(dupe) {
		t.Error("Equal Clownfish Float")
	}
	if !num.Equals(2.5) {
		t.Error("Equal Go float64")
	}
	if num.Equals(bigger) {
		t.Error("Non-equal Clownfish Integer")
	}
	if num.Equals(43) {
		t.Error("Non-equal Go integer")
	}
	if got := num.Equals(2.6); got {
		t.Error("Non-equal Go float64")
	}
	if got := num.Equals("foo"); got {
		t.Error("Non-equal Go string")
	}
}
