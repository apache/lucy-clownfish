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

func TestStringCat(t *testing.T) {
	s := NewString("foo")
	got := s.Cat("bar")
	if got != "foobar" {
		t.Error("Expected 'foobar', got", got)
	}
}

func TestStringStartsWithEndsWith(t *testing.T) {
	s := NewString("foobar")
	if !s.StartsWith("foo") {
		t.Error("StartsWith yes")
	}
	if s.StartsWith("bar") {
		t.Error("StartsWith no")
	}
	if !s.EndsWith("bar") {
		t.Error("EndsWith yes")
	}
	if !s.EndsWith("bar") {
		t.Error("EndsWith no")
	}
}

func TestStringBaseXToI64(t *testing.T) {
	s := NewString("100000000")
	var got int64 = s.BaseXToI64(10)
	if got != 100000000 {
		t.Error("positive base 10", got)
	}
	got = s.BaseXToI64(2)
	if got != 256 {
		t.Error("positive base 2", got)
	}
	s = NewString("-100000000")
	got = s.BaseXToI64(10)
	if got != -100000000 {
		t.Error("negative base 10", got)
	}
	got = s.BaseXToI64(2)
	if got != -256 {
		t.Error("negative base 2", got)
	}
}

func TestStringFind(t *testing.T) {
	s := NewString("foobarbaz")
	var got int64 = s.Find("bar")
	if got != 3 {
		t.Error("Find yes", got)
	}
	got = s.Find("banana")
	if got != -1 {
		t.Error("Find no", got)
	}
}

func TestStringEquals(t *testing.T) {
	s := NewString("foo")
	if !s.Equals("foo") {
		t.Error("Equals should succeed")
	}
	if s.Equals("bar") {
		t.Error("Equals should fail")
	}
}

func TestStringCompareTo(t *testing.T) {
	s := NewString("foo")
	if !(s.CompareTo("boo") > 0) {
		t.Error("'foo' > 'boo'")
	}
	if !(s.CompareTo("foo") == 0) {
		t.Error("'foo' == 'foo'")
	}
	if !(s.CompareTo("zoo") < 0) {
		t.Error("'foo' < 'zoo'")
	}
	if !(s.CompareTo("fo") > 0) {
		t.Error("'foo' > 'fo'")
	}
	if !(s.CompareTo("food") < 0) {
		t.Error("'foo' < 'food'")
	}
	if !(s.CompareTo("foo\u0000") < 0) {
		t.Error("'foo' < 'foo\\0'")
	}
	if !(s.CompareTo("") > 0) {
		t.Error("'foo' > ''")
	}
}

func TestStringLenAndGetSize(t *testing.T) {
	s := NewString("\u263a")
	var len uintptr = s.Length()
	if len != 1 {
		t.Error("Length() should return 1, got", len)
	}
	var size uintptr = s.GetSize()
	if size != 3 {
		t.Error("GetSize() should return 3, got", size)
	}
}

func TestStringClone(t *testing.T) {
	t.Skip("Skip Clone() because it shouldn't return an Obj")
	s := NewString("foo")
	got := s.Clone()
	if !s.Equals(got) {
		t.Fail()
	}
}

func TestStringHashSum(t *testing.T) {
	// Test compilation only.
	s := NewString("foo")
	var _ uintptr = s.HashSum()
}

func TestStringToString(t *testing.T) {
	s := NewString("foo")
	if s.ToString() != "foo" {
		t.Fail()
	}
}

func TestStringTrim(t *testing.T) {
	s := NewString(" foo ")
	var got string = s.Trim()
	if got != "foo" {
		t.Error("Trim: '" + got + "'")
	}
	got = s.TrimTop()
	if got != "foo " {
		t.Error("TrimTop: '" + got + "'")
	}
	got = s.TrimTail()
	if got != " foo" {
		t.Error("TrimTail: '" + got + "'")
	}
}

func TestStringCodePointAtFrom(t *testing.T) {
	s := NewString("foobar")
	var got rune = s.CodePointAt(3)
	if got != 'b' {
		t.Error("CodePointAt returned", got)
	}
	got = s.CodePointFrom(2)
	if got != 'a' {
		t.Error("CodePointFrom returned", got)
	}
}

func TestStringSubString(t *testing.T) {
	s := NewString("foobarbaz")
	var got string = s.SubString(3, 3)
	if got != "bar" {
		t.Error("SubString returned", got)
	}
}

func TestStringTopTail(t *testing.T) {
	s := NewString("foo")
	top := s.Top()
	got := top.Next()
	if got != 'f' {
		t.Error("Top iter returned", got)
	}
	tail := s.Tail()
	got = tail.Prev()
	if got != 'o' {
		t.Error("Tail iter returned", got)
	}
}

func TestStrIterNextPrev(t *testing.T) {
	iter := NewStringIterator(NewString("abc"), 1)
	if got := iter.Next(); got != 'b' {
		t.Errorf("Expected 'b', got %d", got)
	}
	if got := iter.Prev(); got != 'b' {
		t.Errorf("Expected 'b', got %d", got)
	}
}

func TestStrIterHasNextHasPrev(t *testing.T) {
	iter := NewStringIterator(NewString("a"), 0)
	if iter.HasPrev() {
		t.Error("HasPrev at top")
	}
	if !iter.HasNext() {
		t.Error("HasNext at top")
	}
	iter.Next()
	if !iter.HasPrev() {
		t.Error("HasPrev at end")
	}
	if iter.HasNext() {
		t.Error("HasNext at end")
	}
}

func TestStrIterClone(t *testing.T) {
	iter := NewStringIterator(NewString("abc"), 0)
	iter.Next()
	clone := iter.Clone().(StringIterator)
	if got := clone.Next(); got != 'b' {
		t.Errorf("Expected 'b', got %d", got)
	}
}

func TestStrIterAssign(t *testing.T) {
	abc := NewString("abc")
	xyz := NewString("xyz")
	iter := NewStringIterator(abc, 1)
	iter.Assign(NewStringIterator(xyz, 1))
	if got := iter.Next(); got != 'y' {
		t.Errorf("Expected 'y', got %d", got)
	}
}

func TestStrIterEquals(t *testing.T) {
	iter := NewStringIterator(NewString("abc"), 0)
	iter.Next()
	clone := iter.Clone().(StringIterator)
	if !iter.Equals(clone) {
		t.Error("Equals should succeed")
	}
	clone.Next()
	if iter.Equals(clone) {
		t.Error("Equals should fail")
	}
}

func TestStrIterCompareTo(t *testing.T) {
	iter := NewStringIterator(NewString("abc"), 0)
	other := iter.Clone().(StringIterator)
	if got := iter.CompareTo(other); got != 0 {
		t.Errorf("Expected 0, got %d", got)
	}
	iter.Next()
	if got := iter.CompareTo(other); got <= 0 {
		t.Errorf("More advanced iterator should be greater than: %d", got)
	}
	if got := other.CompareTo(iter); got >= 0 {
		t.Errorf("Less advanced iterator should be less than: %d", got)
	}
}

func TestStrIterAdvanceRecede(t *testing.T) {
	iter := NewStringIterator(NewString("abcde"), 0)
	if got := iter.Advance(3); got != 3 {
		t.Errorf("Expected Advance to return 3, got %d", got)
	}
	if got := iter.Recede(2); got != 2 {
		t.Errorf("Expected Recede to return 2, got %d", got)
	}
}

func TestStrIterStartsWithEndsWith(t *testing.T) {
	iter := NewStringIterator(NewString("abcd"), 2)
	if !iter.StartsWith("cd") {
		t.Error("StartsWith should succeed")
	}
	if iter.StartsWith("cde") {
		t.Error("StartsWith should fail")
	}
	if !iter.EndsWith("ab") {
		t.Error("EndsWith should succeed")
	}
	if iter.EndsWith("abc") {
		t.Error("EndsWith should fail")
	}
}

func TestStrIterSkipWhite(t *testing.T) {
	iter := NewStringIterator(NewString("foo  bar"), 0)
	if got := iter.SkipNextWhitespace(); got != 0 {
		t.Error("No whitespace to skip")
	}
	iter.Advance(3)
	if got := iter.SkipNextWhitespace(); got != 2 || !iter.StartsWith("bar") {
		t.Error("Skip forward 2 spaces")
	}
	if got := iter.SkipPrevWhitespace(); got != 2 || !iter.EndsWith("foo") {
		t.Error("Skip backwards 2 spaces")
	}
}
