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
import "unsafe"
import "reflect"

func TestBlobNewBlob(t *testing.T) {
	content := []byte("foo")
	blob := NewBlob(content)
	converted := BlobToGo(unsafe.Pointer(blob.TOPTR()))
	if !reflect.DeepEqual(converted, content) {
		t.Errorf("Expected %v, got %v")
	}
}

func TestBlobGetBuf(t *testing.T) {
	content := []byte("foo")
	blob := NewBlob(content)
	if buf := blob.GetBuf(); buf == 0 {
		t.Error("GetBuf() not working as expected")
	}
}

func TestBlobGetSize(t *testing.T) {
	content := []byte("foo")
	blob := NewBlob(content)
	if size := blob.GetSize(); size != 3 {
		t.Error("Size should be 3, not %v", size)
	}
}

func TestBlobClone(t *testing.T) {
	content := []byte("foo")
	blob := NewBlob(content)
	dupe := blob.Clone().(Blob)
	if !blob.Equals(dupe) {
		t.Errorf("Clone() should yield a dupe")
	}
}

func TestBlobCompareTo(t *testing.T) {
	a := NewBlob([]byte("a"))
	b := NewBlob([]byte("b"))
	a2 := NewBlob([]byte("a"))
	if got := a.CompareTo(b); got >= 0 {
		t.Errorf("a CompareTo b: %v", got)
	}
	if got := b.CompareTo(a); got <= 0 {
		t.Errorf("a CompareTo b: %v", got)
	}
	if got := a.CompareTo(a2); got != 0 {
		t.Errorf("a CompareTo a: %v", got)
	}
}

func TestBlobEquals(t *testing.T) {
	a := NewBlob([]byte("a"))
	b := NewBlob([]byte("b"))
	a2 := NewBlob([]byte("a"))
	if a.Equals(b) {
		t.Error("a should not Equal b")
	}
	if b.Equals(a) {
		t.Error("b should not Equal a")
	}
	if !a.Equals(a2) {
		t.Error("a should Equal a")
	}
	if !a.Equals([]byte("a")) {
		t.Error("Comparison to duplicate Go []byte content")
	}
	if a.Equals([]byte("b")) {
		t.Error("Comparison to different Go []byte content")
	}
	if a.Equals("a") {
		t.Error("Comparison against Go string should fail")
	}
	if a.Equals(NewString("a")) {
		t.Error("Comparison against String should fail")
	}
}
