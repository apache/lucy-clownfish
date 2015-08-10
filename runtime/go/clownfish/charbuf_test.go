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

func TestCharBufCat(t *testing.T) {
	cb := NewCharBuf(0)
	cb.Cat("foo")
	if got := cb.ToString(); got != "foo" {
		t.Errorf("Expected foo, got %v", got)
	}
}

func TestCharBufMimic(t *testing.T) {
	cb := NewCharBuf(0)
	other := NewCharBuf(0)
	other.Cat("foo")
	cb.Mimic(other)
	if got := cb.ToString(); got != "foo" {
		t.Errorf("Expected foo, got %v", got)
	}
}

func TestCharBufCatChar(t *testing.T) {
	cb := NewCharBuf(0)
	cb.CatChar('x')
	if got := cb.ToString(); got != "x" {
		t.Errorf("Expected x, got %v", got)
	}
}

func TestCharBufSetSizeGetSize(t *testing.T) {
	cb := NewCharBuf(0)
	cb.Cat("abc")
	cb.SetSize(2)
	if got := cb.GetSize(); got != 2 {
		t.Errorf("Size should be 2 but got %d", got)
	}
	if got := cb.ToString(); got != "ab" {
		t.Errorf("Expected ab, got %v", got)
	}
}

func TestCharBufClone(t *testing.T) {
	cb := NewCharBuf(0)
	cb.Cat("foo")
	clone := cb.Clone()
	if got := clone.ToString(); got != "foo" {
		t.Errorf("Expected foo, got %v", got)
	}
}

func TestCharBufYieldString(t *testing.T) {
	cb := NewCharBuf(0)
	cb.Cat("foo")
	if got := cb.YieldString(); got != "foo" {
		t.Errorf("Should yield foo, got %v", got)
	}
	if got := cb.YieldString(); got != "" {
		t.Errorf("Should yield empty string, got %v", got)
	}
}
