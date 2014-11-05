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

/* Package cfc provides a compiler for Apache Clownfish.
 */
package cfc

// #include "CFC.h"
// #include <stdlib.h>
import "C"

import "runtime"
import "unsafe"

func DoStuff() {
	hierarchy := NewHierarchy("autogen")
	hierarchy.Build()
}

type Hierarchy struct {
	ref *C.CFCHierarchy
}

func NewHierarchy(dest string) Hierarchy {
	destCString := C.CString(dest)
	defer C.free(unsafe.Pointer(destCString))
	obj := Hierarchy{C.CFCHierarchy_new(destCString)}
	runtime.SetFinalizer(&obj, (*Hierarchy).RunDecRef)
	return obj
}

func (obj *Hierarchy) RunDecRef() {
	C.CFCBase_decref((*C.CFCBase)(unsafe.Pointer(obj.ref)))
}

func (obj *Hierarchy) Build() {
	C.CFCHierarchy_build(obj.ref)
}
