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

#include "charmony.h"

#include "Clownfish/Obj.h"
#include "Clownfish/Err.h"
#include "Clownfish/Class.h"
#include "Clownfish/String.h"
#include "Clownfish/Blob.h"
#include "Clownfish/Hash.h"
#include "Clownfish/Vector.h"
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

func init() {
	C.GoCfish_glue_exported_symbols()
	C.cfish_bootstrap_parcel()
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

func CFStringToGo(ptr unsafe.Pointer) string {
	cfString := (*C.cfish_String)(ptr)
	if cfString == nil {
		return ""
	}
	if !C.cfish_Str_is_a(cfString, C.CFISH_STRING) {
		cfString := C.CFISH_Str_To_String(cfString)
		defer C.cfish_dec_refcount(unsafe.Pointer(cfString))
	}
	data := C.CFISH_Str_Get_Ptr8(cfString)
	size := C.int(C.CFISH_Str_Get_Size(cfString))
	return C.GoStringN(data, size)
}

func (e *ErrIMP) Error() string {
	mess := C.CFISH_Err_Get_Mess((*C.cfish_Err)(unsafe.Pointer(e.ref)))
	return CFStringToGo(unsafe.Pointer(mess))
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
