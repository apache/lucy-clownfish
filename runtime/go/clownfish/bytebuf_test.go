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

func TestByteBufCat(t *testing.T) {
	bb := NewByteBuf(0)
	content := []byte("foo")
	bb.cat(content)
	if got := bb.yieldBlob(); !reflect.DeepEqual(got, content) {
		t.Errorf("Expected %v, got %v", content, got)
	}
}

func TestByteBufSetSizeGetSize(t *testing.T) {
	bb := NewByteBuf(0)
	content := []byte("abc")
	bb.cat(content)
	bb.setSize(2)
	if got := bb.getSize(); got != 2 {
		t.Errorf("Expected size 2, got %d", got)
	}
	expected := []byte("ab")
	if got := bb.yieldBlob(); !reflect.DeepEqual(got, expected) {
		t.Errorf("Expected %v, got %v", expected, got)
	}
}

func TestByteBufGetCapacity(t *testing.T) {
	bb := NewByteBuf(5)
	if cap := bb.getCapacity(); cap < 5 {
		t.Errorf("Expected at least 5, got %d", cap)
	}
}

func TestByteBufEquals(t *testing.T) {
	bb := NewByteBuf(0)
	other := NewByteBuf(0)
	content := []byte("foo")
	bb.cat(content)
	other.cat(content)
	if !bb.Equals(other) {
		t.Errorf("Equals against equal ByteBuf")
	}
	other.setSize(2)
	if bb.Equals(other) {
		t.Errorf("Equals against non-equal ByteBuf")
	}
	if bb.Equals(42) {
		t.Errorf("Equals against arbitrary Go type")
	}
}

func TestByteBufClone(t *testing.T) {
	content := []byte("foo")
	bb := NewByteBuf(0)
	bb.cat(content)
	clone := bb.Clone().(ByteBuf)
	if got := clone.yieldBlob(); !reflect.DeepEqual(got, content) {
		t.Errorf("Expected %v, got %v", content, got)
	}
}

func TestByteBufCompareTo(t *testing.T) {
	bb := NewByteBuf(0)
	other := NewByteBuf(0)
	content := []byte("foo")
	bb.cat(content)
	other.cat(content)
	if got := bb.CompareTo(other); got != 0 {
		t.Errorf("CompareTo equal, got %d", got)
	}
	other.setSize(2)
	if got := bb.CompareTo(other); got <= 0 {
		t.Errorf("CompareTo lesser, got %d", got)
	}
	if got := other.CompareTo(bb); got >= 0 {
		t.Errorf("CompareTo greater, got %d", got)
	}
}
