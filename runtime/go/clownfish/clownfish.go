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

*/
import "C"
import "runtime"
import "unsafe"

func init() {
	C.cfish_bootstrap_parcel()
}

type Obj interface {
	ToPtr() unsafe.Pointer
}

type Err struct {
	ref *C.cfish_Err
}

type String struct {
	ref *C.cfish_String
}

type ByteBuf struct {
	ref *C.cfish_ByteBuf
}

type Hash struct {
	ref *C.cfish_Hash
}

type VArray struct {
	ref *C.cfish_VArray
}

type Class struct {
	ref *C.cfish_Class
}

type Method struct {
	ref *C.cfish_Method
}

type LockFreeRegistry struct {
	ref *C.cfish_LockFreeRegistry
}

func NewString(goString string) *String {
	str := C.CString(goString)
	len := C.size_t(len(goString))
	obj := &String{
		C.cfish_Str_new_steal_utf8(str, len),
	}
	runtime.SetFinalizer(obj, (*String).callDecRef)
	return obj
}

func (obj *String) callDecRef() {
	C.cfish_dec_refcount(unsafe.Pointer(obj.ref))
	obj.ref = nil
}

func (obj *String) ToPtr() unsafe.Pointer {
	return unsafe.Pointer(obj.ref)
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
