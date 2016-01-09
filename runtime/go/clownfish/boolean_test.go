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

func TestBooleanGetValue(t *testing.T) {
	myTrue := NewBoolean(true)
	myFalse := NewBoolean(false)
	if !myTrue.GetValue() {
		t.Errorf("Expected true, got false")
	}
	if myFalse.GetValue() {
		t.Errorf("Expected false, got true")
	}
}

func TestBooleanToString(t *testing.T) {
	myTrue := NewBoolean(true)
	myFalse := NewBoolean(false)
	if got := myTrue.ToString(); got != "true" {
		t.Errorf("Expected \"true\", got %v", got)
	}
	if got := myFalse.ToString(); got != "false" {
		t.Errorf("Expected \"false\", got %v", got)
	}
}

func TestBooleanClone(t *testing.T) {
	myTrue := NewBoolean(true)
	myFalse := NewBoolean(false)
	if myTrue.TOPTR() != myTrue.Clone().TOPTR() {
		t.Errorf("Clone should wrap CFISH_TRUE")
	}
	if myFalse.TOPTR() != myFalse.Clone().TOPTR() {
		t.Errorf("Clone should wrap CFISH_FALSE")
	}
}

func TestBooleanEquals(t *testing.T) {
	myTrue := NewBoolean(true)
	myFalse := NewBoolean(false)
	if !myTrue.Equals(myTrue) {
		t.Error("Equal Clownfish Boolean")
	}
	if !myTrue.Equals(true) {
		t.Error("Equal Go bool (true)")
	}
	if !myFalse.Equals(false) {
		t.Error("Equal Go bool (false)")
	}
	if myTrue.Equals(myFalse) {
		t.Error("Non-equal Clownfish Boolean")
	}
	if myFalse.Equals(0) {
		t.Error("Go 0 should not equal Clownfish Boolean false")
	}
}
