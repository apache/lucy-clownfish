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
import "math"

func deepCheck(t *testing.T, got, expected interface{}) {
	if !reflect.DeepEqual(got, expected) {
		t.Errorf("Expected %v got %v", expected, got)
	}
}

func TestStringToGo(t *testing.T) {
	strings := []string{"foo", "", "z\u0000z"}
	for _, val := range strings {
		got := StringToGo(unsafe.Pointer(NewString(val).TOPTR()))
		deepCheck(t, got, val)
	}
}

func TestBlobToGo(t *testing.T) {
	strings := []string{"foo", "", "z\u0000z"}
	for _, str := range strings {
		expected := []byte(str)
		got := BlobToGo(unsafe.Pointer(NewBlob(expected).TOPTR()))
		deepCheck(t, got, expected)
	}
}

func TestIntegerToGo(t *testing.T) {
	values := []int64{math.MaxInt64, math.MinInt64, 0, 1, -1}
	for _, val := range values {
		got := ToGo(unsafe.Pointer(NewInteger(val).TOPTR()))
		deepCheck(t, got, val)
	}
}

func TestFloatToGo(t *testing.T) {
	values := []float64{math.MaxFloat64, math.SmallestNonzeroFloat64,
		0.0, -0.0, 0.5, -0.5, math.Inf(1), math.Inf(-1)}
	for _, val := range values {
		got := ToGo(unsafe.Pointer(NewFloat(val).TOPTR()))
		deepCheck(t, got, val)
	}
	notNum := ToGo(unsafe.Pointer(NewFloat(math.NaN()).TOPTR()))
	if !math.IsNaN(notNum.(float64)) {
		t.Error("Didn't convert NaN cleanly")
	}
}

func TestBoolToGo(t *testing.T) {
	values := []bool{true, false}
	for _, val := range values {
		got := ToGo(unsafe.Pointer(NewBoolean(val).TOPTR()))
		deepCheck(t, got, val)
	}
}

func TestVectorToGo(t *testing.T) {
	vec := NewVector(3)
	vec.Push(NewString("foo"))
	vec.Push(NewInteger(42))
	//vec.Push(nil)
	inner := NewVector(1)
	inner.Push(NewString("bar"))
	vec.Push(inner)
	expected := []interface{}{
		"foo",
		int64(42),
		//nil,
		[]interface{}{"bar"},
	}
	got := VectorToGo(unsafe.Pointer(vec.TOPTR()))
	deepCheck(t, got, expected)
}

func TestHashToGo(t *testing.T) {
	hash := NewHash(0)
	hash.Store("str", NewString("foo"))
	hash.Store("bool", NewBoolean(false))
	hash.Store("float", NewFloat(-0.5))
	hash.Store("int", NewInteger(42))
	//hash.Store("nil", nil)
	vec := NewVector(1)
	vec.Push(NewString("bar"))
	hash.Store("vector", vec)
	got := HashToGo(unsafe.Pointer(hash.TOPTR()))

	expected := map[string]interface{}{
		"str":   "foo",
		"bool":  false,
		"float": -0.5,
		"int":   int64(42),
		//"nil": nil,
		"vector": []interface{}{"bar"},
	}
	deepCheck(t, got, expected)
}

func TestNilToGo(t *testing.T) {
	got := ToGo(unsafe.Pointer(uintptr(0)))
	if got != nil {
		t.Errorf("Expected nil, got %v", got)
	}
}

func TestGoToNil(t *testing.T) {
	if GoToClownfish(nil, nil, true) != nil {
		t.Error("Convert nullable nil successfully")
	}
}

func TestGoToNilNotNullable(t *testing.T) {
	defer func() { recover() }()
	GoToClownfish(nil, nil, false)                   // should panic
	t.Error("Non-nullable nil should trigger error") // should be unreachable
}

func TestGoToString(t *testing.T) {
	strings := []string{"foo", "", "z\u0000z"}
	for _, val := range strings {
		got := WRAPAny(GoToString(val))
		if _, ok := got.(String); !ok {
			t.Errorf("Not a String, but a %T", got)
		}
		if ToGo(unsafe.Pointer(got.TOPTR())).(string) != val {
			t.Error("Round trip failed")
		}
	}
}

func TestGoToBlob(t *testing.T) {
	strings := []string{"foo", "", "z\u0000z"}
	for _, str := range strings {
		val := []byte(str)
		got := WRAPAny(GoToBlob(val))
		if _, ok := got.(Blob); !ok {
			t.Errorf("Not a Blob, but a %T", got)
		}
		if !reflect.DeepEqual(ToGo(unsafe.Pointer(got.TOPTR())), val) {
			t.Error("Round trip failed")
		}
	}
}

func checkIntConv(t *testing.T, got Obj) {
	if _, ok := got.(Integer); !ok {
		t.Errorf("Not an Integer, but a %T", got)
	}
}

func TestGoToInteger(t *testing.T) {
	checkIntConv(t, WRAPAny(GoToClownfish(int(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(uint(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(int64(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(int32(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(int16(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(int8(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(uint64(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(uint32(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(uint16(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(uint8(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(byte(42), nil, true)))
	checkIntConv(t, WRAPAny(GoToClownfish(rune(42), nil, true)))
}

func TestGoToIntegerRangeError(t *testing.T) {
	defer func() { recover() }()
	GoToClownfish(uint64(math.MaxUint64), nil, true)
	t.Error("Truncation didn't cause error")
}

func TestGoToFloat(t *testing.T) {
	var got Obj

	values := []float64{math.MaxFloat64, math.SmallestNonzeroFloat64,
		0.0, -0.0, 0.5, -0.5, math.Inf(1), math.Inf(-1)}
	for _, val := range values {
		got := WRAPAny(GoToFloat(val))
		if _, ok := got.(Float); !ok {
			t.Errorf("Not a Float, but a %T", got)
		}
		if !reflect.DeepEqual(ToGo(unsafe.Pointer(got.TOPTR())), val) {
			t.Error("Round trip failed")
		}
	}

	// NaN
	got = WRAPAny(GoToFloat(math.NaN()))
	if !math.IsNaN(ToGo(unsafe.Pointer(got.TOPTR())).(float64)) {
		t.Error("Didn't convert NaN cleanly")
	}

	// float32
	expected := float32(0.5)
	got = WRAPAny(GoToClownfish(expected, nil, false))
	deepCheck(t, got.(Float).GetValue(), float64(expected))
}

func TestGoToBoolean(t *testing.T) {
	values := []bool{true, false}
	for _, val := range values {
		got := WRAPAny(GoToBoolean(val))
		if _, ok := got.(Boolean); !ok {
			t.Errorf("Not a Boolean, but a %T", got)
		}
		if !reflect.DeepEqual(ToGo(unsafe.Pointer(got.TOPTR())), val) {
			t.Error("Round trip failed")
		}
	}
}

func TestGoToHash(t *testing.T) {
	expected := map[string]interface{}{
		"foo": int64(1),
		"bar": []interface{}{},
	}
	got := WRAPAny(GoToHash(expected))
	if _, ok := got.(Hash); !ok {
		t.Errorf("Not a Hash, but a %T", got)
	}
	deepCheck(t, ToGo(unsafe.Pointer(got.TOPTR())), expected)
}

func TestGoToVector(t *testing.T) {
	expected := []interface{}{
		"foo",
		"bar",
		[]interface{}{},
		int64(-1),
	}
	got := WRAPAny(GoToVector(expected))
	if _, ok := got.(Vector); !ok {
		t.Errorf("Not a Vector, but a %T", got)
	}
	deepCheck(t, ToGo(unsafe.Pointer(got.TOPTR())), expected)
}
