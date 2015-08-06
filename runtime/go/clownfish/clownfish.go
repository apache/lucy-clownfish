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

/*

#include <limits.h>

#include "charmony.h"

#include "Clownfish/Obj.h"
#include "Clownfish/Err.h"
#include "Clownfish/Class.h"
#include "Clownfish/String.h"
#include "Clownfish/Blob.h"
#include "Clownfish/Hash.h"
#include "Clownfish/HashIterator.h"
#include "Clownfish/Vector.h"
#include "Clownfish/Num.h"
#include "Clownfish/Boolean.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Method.h"

extern void
GoCfish_PanicErr_internal(cfish_Err *error);
typedef void
(*cfish_Err_do_throw_t)(cfish_Err *error);
extern cfish_Err_do_throw_t GoCfish_PanicErr;

extern cfish_Err*
GoCfish_TrapErr_internal(CFISH_Err_Attempt_t routine, void *context);
typedef cfish_Err*
(*cfish_Err_trap_t)(CFISH_Err_Attempt_t routine, void *context);
extern cfish_Err_trap_t GoCfish_TrapErr;

// C symbols linked into a Go-built package archive are not visible to
// external C code -- but internal code *can* see symbols from outside.
// This allows us to fake up symbol export by assigning values only known
// interally to external symbols during Go package initialization.
static CHY_INLINE void
GoCfish_glue_exported_symbols() {
	GoCfish_PanicErr = GoCfish_PanicErr_internal;
	GoCfish_TrapErr  = GoCfish_TrapErr_internal;
}

static CHY_INLINE void
GoCfish_RunRoutine(CFISH_Err_Attempt_t routine, void *context) {
	routine(context);
}

*/
import "C"
import "runtime"
import "unsafe"
import "fmt"
import "math"
import "sync"

const (
	maxUint = ^uint(0)
	minUint = 0
	maxInt  = int(^uint(0) >> 1)
	minInt  = -(maxInt - 1)
)

type WrapFunc func(unsafe.Pointer)Obj
var wrapRegMutex sync.Mutex
var wrapReg *map[unsafe.Pointer]WrapFunc

func init() {
	C.GoCfish_glue_exported_symbols()
	C.cfish_bootstrap_parcel()
	initWRAP()
}

func RegisterWrapFuncs(newEntries map[unsafe.Pointer]WrapFunc) {
	wrapRegMutex.Lock()
	newSize := len(newEntries)
	if wrapReg != nil {
		newSize += len(*wrapReg)
	}
	newReg := make(map[unsafe.Pointer]WrapFunc, newSize)
	if wrapReg != nil {
		for k, v := range *wrapReg {
			newReg[k] = v
		}
	}
	for k, v := range newEntries {
		newReg[k] = v
	}
	wrapReg = &newReg
	wrapRegMutex.Unlock()
}

func WRAPAny(ptr unsafe.Pointer) Obj {
	if ptr == nil {
		return nil
	}
	class := C.cfish_Obj_get_class((*C.cfish_Obj)(ptr))
	wrapFunc := (*wrapReg)[unsafe.Pointer(class)]
	if wrapFunc == nil {
		className := CFStringToGo(unsafe.Pointer(C.CFISH_Class_Get_Name((*C.cfish_Class)(class))))
		panic(fmt.Sprintf("Failed to find WRAP function for %s", className))
	}
	return wrapFunc(ptr)
}

type ObjIMP struct {
	ref uintptr
}

func NewString(goString string) String {
	str := C.CString(goString)
	len := C.size_t(len(goString))
	cfObj := C.cfish_Str_new_steal_utf8(str, len)
	return WRAPString(unsafe.Pointer(cfObj))
}

func NewVector(size int) Vector {
	if (size < 0 || uint64(size) > ^uint64(0)) {
		panic(NewErr(fmt.Sprintf("Param 'size' out of range: %d", size)))
	}
	cfObj := C.cfish_Vec_new(C.size_t(size))
	return WRAPVector(unsafe.Pointer(cfObj))
}

func NewHash(size int) Hash {
	if (size < 0 || uint64(size) > ^uint64(0)) {
		panic(NewErr(fmt.Sprintf("Param 'size' out of range: %d", size)))
	}
	cfObj := C.cfish_Hash_new(C.size_t(size))
	return WRAPHash(unsafe.Pointer(cfObj))
}

func NewHashIterator(hash Hash) HashIterator {
	hashCF := (*C.cfish_Hash)(unsafe.Pointer(hash.TOPTR()))
	cfObj := C.cfish_HashIter_new(hashCF)
	return WRAPHashIterator(unsafe.Pointer(cfObj))
}

func (h *HashIMP) Keys() []string {
	self := (*C.cfish_Hash)(unsafe.Pointer(h.TOPTR()))
	keysCF := C.CFISH_Hash_Keys(self)
	numKeys := C.CFISH_Vec_Get_Size(keysCF)
	keys := make([]string, 0, int(numKeys))
	for i := C.size_t(0); i < numKeys; i++ {
		keys = append(keys, CFStringToGo(unsafe.Pointer(C.CFISH_Vec_Fetch(keysCF, i))))
	}
	C.cfish_decref(unsafe.Pointer(keysCF))
	return keys
}

func (o *ObjIMP) INITOBJ(ptr unsafe.Pointer) {
	o.ref = uintptr(ptr)
	runtime.SetFinalizer(o, ClearRef)
}

func ClearRef (o *ObjIMP) {
	C.cfish_dec_refcount(unsafe.Pointer(o.ref))
	o.ref = 0
}

func (o *ObjIMP) TOPTR() uintptr {
	return o.ref
}

func (o *ObjIMP)Clone() Obj {
	self := (*C.cfish_Obj)(unsafe.Pointer(o.TOPTR()))
	dupe := C.CFISH_Obj_Clone(self)
	return WRAPAny(unsafe.Pointer(dupe)).(Obj)
}

func certifyCF(value interface{}, class *C.cfish_Class) {
	cfObj, ok := value.(Obj)
	if ok {
		if C.cfish_Obj_is_a((*C.cfish_Obj)(unsafe.Pointer(cfObj.TOPTR())), class) {
			return
		}
	}
	className := StringToGo(unsafe.Pointer(C.CFISH_Class_Get_Name(class)))
	panic(NewErr(fmt.Sprintf("Can't convert a %T to %s", value, className)))
}

// Convert a Go type into an incremented Clownfish object.  If the supplied
// object is a Clownfish object wrapped in a Go struct, extract the Clownfish
// object and incref it before returning its address.
func GoToClownfish(value interface{}, class unsafe.Pointer, nullable bool) unsafe.Pointer {
	klass := (*C.cfish_Class)(class)

	// Check for nil values.
	if value == nil {
		if nullable {
			return nil
		} else if class != nil {
			className := StringToGo(unsafe.Pointer(C.CFISH_Class_Get_Name(klass)))
			panic(NewErr("Cannot be nil, must be a valid " + className))
		} else {
			panic(NewErr("Cannot be nil"))
		}
	}

	// Default to accepting any type.
	if klass == nil {
		klass = C.CFISH_OBJ
	}

	// Convert the value according to its type if possible.
	var converted unsafe.Pointer
	switch v := value.(type) {
	case string:
		if klass == C.CFISH_STRING || klass == C.CFISH_OBJ {
			converted = GoToString(value)
		}
	case []byte:
		if klass == C.CFISH_BLOB || klass == C.CFISH_OBJ {
			converted = GoToBlob(value)
		}
	case int:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case uint:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case uintptr:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case int64:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case int32:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case int16:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case int8:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case uint64:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case uint32:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case uint16:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case uint8:
		if klass == C.CFISH_INTEGER || klass == C.CFISH_OBJ {
			converted = GoToInteger(value)
		}
	case float32:
		if klass == C.CFISH_FLOAT || klass == C.CFISH_OBJ {
			converted = GoToFloat(value)
		}
	case float64:
		if klass == C.CFISH_FLOAT || klass == C.CFISH_OBJ {
			converted = GoToFloat(value)
		}
	case []interface{}:
		if klass == C.CFISH_VECTOR || klass == C.CFISH_OBJ {
			converted = GoToVector(value)
		}
	case map[string]interface{}:
		if klass == C.CFISH_HASH || klass == C.CFISH_OBJ {
			converted = GoToHash(value)
		}
	case Obj:
		converted = unsafe.Pointer(C.cfish_incref(unsafe.Pointer(v.TOPTR())))
	}

	// Confirm that we got what we were looking for and return.
	if converted != nil {
		if C.cfish_Obj_is_a((*C.cfish_Obj)(converted), klass) {
			return unsafe.Pointer(C.cfish_incref(converted))
		}
	}

	// Report a conversion error.
	className := StringToGo(unsafe.Pointer(C.CFISH_Class_Get_Name(klass)))
	panic(NewErr(fmt.Sprintf("Can't convert a %T to %s", value, className)))
}

func GoToString(value interface{}) unsafe.Pointer {
	switch v := value.(type) {
	case string:
		size := len(v)
		str := C.CString(v)
		return unsafe.Pointer(C.cfish_Str_new_steal_utf8(str, C.size_t(size)))
	case Obj:
		certifyCF(v, C.CFISH_STRING)
		return unsafe.Pointer(C.cfish_incref(unsafe.Pointer(v.TOPTR())))
	default:
		mess := fmt.Sprintf("Can't convert %T to clownfish.String", v)
		panic(NewErr(mess))
	}
}

func GoToBlob(value interface{}) unsafe.Pointer {
	switch v := value.(type) {
	case []byte:
		size := C.size_t(len(v))
		var buf *C.char = nil
		if size > 0 {
			buf = ((*C.char)(unsafe.Pointer(&v[0])))
		}
		return unsafe.Pointer(C.cfish_Blob_new(buf, size))
	case Obj:
		certifyCF(v, C.CFISH_BLOB)
		return unsafe.Pointer(C.cfish_incref(unsafe.Pointer(v.TOPTR())))
	default:
		mess := fmt.Sprintf("Can't convert %T to clownfish.Blob", v)
		panic(NewErr(mess))
	}
}

func GoToInteger(value interface{}) unsafe.Pointer {
	switch v := value.(type) {
	case int:
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case uint:
		if v > math.MaxInt64 {
			mess := fmt.Sprintf("uint value too large: %v", v)
			panic(NewErr(mess))
		}
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case uintptr:
		if v > math.MaxInt64 {
			mess := fmt.Sprintf("uintptr value too large: %v", v)
			panic(NewErr(mess))
		}
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case uint64:
		if v > math.MaxInt64 {
			mess := fmt.Sprintf("uint64 value too large: %v", v)
			panic(NewErr(mess))
		}
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case uint32:
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case uint16:
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case uint8:
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case int64:
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case int32:
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case int16:
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case int8:
		return unsafe.Pointer(C.cfish_Int_new(C.int64_t(v)))
	case Obj:
		certifyCF(v, C.CFISH_INTEGER)
		return unsafe.Pointer(C.cfish_incref(unsafe.Pointer(v.TOPTR())))
	default:
		mess := fmt.Sprintf("Can't convert %T to clownfish.Integer", v)
		panic(NewErr(mess))
	}
}

func GoToFloat(value interface{}) unsafe.Pointer {
	switch v := value.(type) {
	case float32:
		return unsafe.Pointer(C.cfish_Float_new(C.double(v)))
	case float64:
		return unsafe.Pointer(C.cfish_Float_new(C.double(v)))
	case Obj:
		certifyCF(v, C.CFISH_FLOAT)
		return unsafe.Pointer(C.cfish_incref(unsafe.Pointer(v.TOPTR())))
	default:
		mess := fmt.Sprintf("Can't convert %T to clownfish.Float", v)
		panic(NewErr(mess))
	}
}

func GoToBoolean(value interface{}) unsafe.Pointer {
	switch v := value.(type) {
	case bool:
		if v {
			return unsafe.Pointer(C.cfish_incref(unsafe.Pointer(C.CFISH_TRUE)))
		} else {
			return unsafe.Pointer(C.cfish_incref(unsafe.Pointer(C.CFISH_FALSE)))
		}
	case Obj:
		certifyCF(v, C.CFISH_BOOLEAN)
		return unsafe.Pointer(C.cfish_incref(unsafe.Pointer(v.TOPTR())))
	default:
		mess := fmt.Sprintf("Can't convert %T to clownfish.Boolean", v)
		panic(NewErr(mess))
	}
}

func GoToVector(value interface{}) unsafe.Pointer {
	switch v := value.(type) {
	case []interface{}:
		size := len(v)
		vec := C.cfish_Vec_new(C.size_t(size))
		for i := 0; i < size; i++ {
			elem := GoToClownfish(v[i], nil, true)
			C.CFISH_Vec_Store(vec, C.size_t(i), (*C.cfish_Obj)(elem))
		}
		return unsafe.Pointer(vec)
	case Obj:
		certifyCF(v, C.CFISH_VECTOR)
		return unsafe.Pointer(C.cfish_incref(unsafe.Pointer(v.TOPTR())))
	default:
		mess := fmt.Sprintf("Can't convert %T to clownfish.Vector", v)
		panic(NewErr(mess))
	}
}

func GoToHash(value interface{}) unsafe.Pointer {
	switch v := value.(type) {
	case map[string]interface{}:
		size := len(v)
		hash := C.cfish_Hash_new(C.size_t(size))
		for key, val := range v {
			newVal := GoToClownfish(val, nil, true)
			keySize := len(key)
			keyStr := C.CString(key)
		    cfKey := C.cfish_Str_new_steal_utf8(keyStr, C.size_t(keySize))
			defer C.cfish_dec_refcount(unsafe.Pointer(cfKey))
			C.CFISH_Hash_Store(hash, cfKey, (*C.cfish_Obj)(newVal))
		}
		return unsafe.Pointer(hash)
	case Obj:
		certifyCF(v, C.CFISH_HASH)
		return unsafe.Pointer(C.cfish_incref(unsafe.Pointer(v.TOPTR())))
	default:
		mess := fmt.Sprintf("Can't convert %T to clownfish.Hash", v)
		panic(NewErr(mess))
	}
}

func ToGo(ptr unsafe.Pointer) interface{} {
	if ptr == nil {
		return nil
	}
	class := C.cfish_Obj_get_class((*C.cfish_Obj)(ptr))
	if class == C.CFISH_STRING {
		return CFStringToGo(ptr)
	} else if class == C.CFISH_BLOB {
		return BlobToGo(ptr)
	} else if class == C.CFISH_VECTOR {
		return VectorToGo(ptr)
	} else if class == C.CFISH_HASH {
		return HashToGo(ptr)
	} else if class == C.CFISH_BOOLEAN {
		if ptr == unsafe.Pointer(C.CFISH_TRUE) {
			return true
		} else {
			return false
		}
	} else if class == C.CFISH_INTEGER {
		val := C.CFISH_Int_Get_Value((*C.cfish_Integer)(ptr))
		return int64(val)
	} else if class == C.CFISH_FLOAT {
		val := C.CFISH_Float_Get_Value((*C.cfish_Float)(ptr))
		return float64(val)
	} else {
		// Don't convert to a native Go type, but wrap in a Go struct.
		return WRAPAny(ptr)
	}
}

func CFStringToGo(ptr unsafe.Pointer) string {
	return StringToGo(ptr)
}

func StringToGo(ptr unsafe.Pointer) string {
	cfString := (*C.cfish_String)(ptr)
	if cfString == nil {
		return ""
	}
	if !C.cfish_Str_is_a(cfString, C.CFISH_STRING) {
		cfString := C.CFISH_Str_To_String(cfString)
		defer C.cfish_dec_refcount(unsafe.Pointer(cfString))
	}
	data := C.CFISH_Str_Get_Ptr8(cfString)
	size := C.CFISH_Str_Get_Size(cfString)
	if size > C.size_t(C.INT_MAX) {
		panic(fmt.Sprintf("Overflow: %d > %d", size, C.INT_MAX))
	}
	return C.GoStringN(data, C.int(size))
}

func BlobToGo(ptr unsafe.Pointer) []byte {
	blob := (*C.cfish_Blob)(ptr)
	if blob == nil {
		return nil
	}
	class := C.cfish_Obj_get_class((*C.cfish_Obj)(ptr))
	if class != C.CFISH_BLOB {
		mess := "Not a Blob: " + StringToGo(unsafe.Pointer(C.CFISH_Class_Get_Name(class)))
		panic(NewErr(mess))
	}
	data := C.CFISH_Blob_Get_Buf(blob)
	size := C.CFISH_Blob_Get_Size(blob)
	if size > C.size_t(C.INT_MAX) {
		panic(fmt.Sprintf("Overflow: %d > %d", size, C.INT_MAX))
	}
	return C.GoBytes(unsafe.Pointer(data), C.int(size))
}

func VectorToGo(ptr unsafe.Pointer) []interface{} {
	vec := (*C.cfish_Vector)(ptr)
	if vec == nil {
		return nil
	}
	class := C.cfish_Obj_get_class((*C.cfish_Obj)(ptr))
	if class != C.CFISH_VECTOR {
		mess := "Not a Vector: " + StringToGo(unsafe.Pointer(C.CFISH_Class_Get_Name(class)))
		panic(NewErr(mess))
	}
	size := C.CFISH_Vec_Get_Size(vec)
	if size > C.size_t(maxInt) {
		panic(fmt.Sprintf("Overflow: %d > %d", size, maxInt))
	}
	slice := make([]interface{}, int(size))
	for i := 0; i < int(size); i++ {
		slice[i] = ToGo(unsafe.Pointer(C.CFISH_Vec_Fetch(vec, C.size_t(i))))
	}
	return slice
}

func HashToGo(ptr unsafe.Pointer) map[string]interface{} {
	hash := (*C.cfish_Hash)(ptr)
	if hash == nil {
		return nil
	}
	class := C.cfish_Obj_get_class((*C.cfish_Obj)(ptr))
	if class != C.CFISH_HASH {
		mess := "Not a Hash: " + StringToGo(unsafe.Pointer(C.CFISH_Class_Get_Name(class)))
		panic(NewErr(mess))
	}
	size := C.CFISH_Hash_Get_Size(hash)
	m := make(map[string]interface{}, int(size))
	iter := C.cfish_HashIter_new(hash)
	defer C.cfish_dec_refcount(unsafe.Pointer(iter))
	for C.CFISH_HashIter_Next(iter) {
		key := C.CFISH_HashIter_Get_Key(iter)
		val := C.CFISH_HashIter_Get_Value(iter)
		m[StringToGo(unsafe.Pointer(key))] = ToGo(unsafe.Pointer(val))
	}
	return m
}

func (e *ErrIMP) Error() string {
	mess := C.CFISH_Err_Get_Mess((*C.cfish_Err)(unsafe.Pointer(e.ref)))
	return StringToGo(unsafe.Pointer(mess))
}

//export GoCfish_PanicErr_internal
func GoCfish_PanicErr_internal(cfErr *C.cfish_Err) {
	goErr := WRAPErr(unsafe.Pointer(cfErr))
	panic(goErr)
}

//export GoCfish_TrapErr_internal
func GoCfish_TrapErr_internal(routine C.CFISH_Err_Attempt_t,
	context unsafe.Pointer) *C.cfish_Err {
	err := TrapErr(func() { C.GoCfish_RunRoutine(routine, context) })
	if err != nil {
		ptr := (err.(Err)).TOPTR()
		return ((*C.cfish_Err)(unsafe.Pointer(ptr)))
	}
	return nil
}

// Run the supplied routine, and if it panics with a clownfish.Err, trap and
// return it.
func TrapErr(routine func()) (trapped error) {
	defer func() {
		if r := recover(); r != nil {
			// TODO: pass whitelist of Err types to trap.
			myErr, ok := r.(Err)
			if ok {
				trapped = myErr
			} else {
				// re-panic
				panic(r)
			}
		}
	}()
	routine()
	return trapped
}

func (s *StringIMP) CodePointAt(tick uintptr) rune {
	self := ((*C.cfish_String)(unsafe.Pointer(s.TOPTR())))
	retvalCF := C.CFISH_Str_Code_Point_At(self, C.size_t(tick))
	return rune(retvalCF)
}

func (s *StringIMP) CodePointFrom(tick uintptr) rune {
	self := ((*C.cfish_String)(unsafe.Pointer(s.TOPTR())))
	retvalCF := C.CFISH_Str_Code_Point_From(self, C.size_t(tick))
	return rune(retvalCF)
}

func (s *StringIMP) SwapChars(match, replacement rune) string {
	self := ((*C.cfish_String)(unsafe.Pointer(s.TOPTR())))
	retvalCF := C.CFISH_Str_Swap_Chars(self, C.int32_t(match), C.int32_t(replacement))
	defer C.cfish_dec_refcount(unsafe.Pointer(retvalCF))
	return CFStringToGo(unsafe.Pointer(retvalCF))
}

func NewBoolean(val bool) Boolean {
	if val {
		return WRAPBoolean(unsafe.Pointer(C.cfish_inc_refcount(unsafe.Pointer(C.CFISH_TRUE))))
	} else {
		return WRAPBoolean(unsafe.Pointer(C.cfish_inc_refcount(unsafe.Pointer(C.CFISH_FALSE))))
	}
}

func NewBlob(content []byte) Blob {
	size := C.size_t(len(content))
	var buf *C.char = nil
	if size > 0 {
		buf = ((*C.char)(unsafe.Pointer(&content[0])))
	}
	obj := C.cfish_Blob_new(buf, size)
	return WRAPBlob(unsafe.Pointer(obj))
}
