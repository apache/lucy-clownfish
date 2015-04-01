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
#include "Clownfish/Hash.h"
#include "Clownfish/VArray.h"
#include "Clownfish/String.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/LockFreeRegistry.h"
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
import "unsafe"

func init() {
	C.GoCfish_glue_exported_symbols()
	C.cfish_bootstrap_parcel()
}

func NewString(goString string) String {
	str := C.CString(goString)
	len := C.size_t(len(goString))
	cfObj := C.cfish_Str_new_steal_utf8(str, len)
	return WRAPString(unsafe.Pointer(cfObj))
}

func CFStringToGo(ptr unsafe.Pointer) string {
	cfString := (*C.cfish_String)(ptr)
	if cfString == nil {
		return ""
	}
	if !C.CFISH_Str_Is_A(cfString, C.CFISH_STRING) {
		cfString := C.CFISH_Str_To_String(cfString)
		defer C.cfish_dec_refcount(unsafe.Pointer(cfString))
	}
	data := C.CFISH_Str_Get_Ptr8(cfString)
	size := C.int(C.CFISH_Str_Get_Size(cfString))
	return C.GoStringN(data, size)
}

func NewErr(mess string) Err {
	str := C.CString(mess)
	len := C.size_t(len(mess))
	messC := C.cfish_Str_new_steal_utf8(str, len)
	cfObj := C.cfish_Err_new(messC)
	return WRAPErr(unsafe.Pointer(cfObj))
}

func (obj *implErr) Error() string {
	return CFStringToGo(unsafe.Pointer(C.CFISH_Err_Get_Mess(obj.ref)))
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
