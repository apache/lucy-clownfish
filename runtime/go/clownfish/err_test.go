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
import "errors"

func TestTrapErr(t *testing.T) {
	err := TrapErr(
		func() { panic(NewErr("mistakes were made")) },
	)
	if err == nil {
		t.Error("Failed to trap Err")
	}
}

func TestTrapErr_no_trap_string(t *testing.T) {
	defer func() { recover() }()
	TrapErr(func() { panic("foo") })
	t.Error("Trapped plain string") // shouldn't reach here
}

func TestTrapErr_no_trap_error(t *testing.T) {
	defer func() { recover() }()
	TrapErr(func() { panic(errors.New("foo")) })
	t.Error("Trapped non-clownfish.Error error type") // shouldn't reach here
}

func TestErrGetMess(t *testing.T) {
	err := NewErr("foo")
	expected := "foo"
	if got := err.GetMess(); got != expected {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestErrCatMess(t *testing.T) {
	err := NewErr("foo")
	err.CatMess("bar")
	expected := "foobar"
	if got := err.GetMess(); got != expected {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestErrToString(t *testing.T) {
	err := NewErr("foo")
	expected := "foo"
	if got := err.ToString(); got != expected {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}
