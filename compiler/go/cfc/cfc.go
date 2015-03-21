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

import "os"
import "path"
import "path/filepath"
import "runtime"
import "strings"
import "unsafe"

// Return the path of the main include directory holding clownfish parcel and
// header files.
func MainIncludeDir() string {
	return mainIncDir
}

type CFCError struct {
	mess string
}

func (e CFCError) Error() string {
	return e.mess
}

// Given a package name for a Clownfish parcel with Go bindings, return the
// install path for its C static archive.
//
// TODO: It would be better if we could embed the C archive contents within
// the installed Go archive.
func InstalledLibPath(packageName string) (string, error) {
	goPathDirs := filepath.SplitList(os.Getenv("GOPATH"))
	if len(goPathDirs) == 0 {
		return "", CFCError{"GOPATH environment variable not set"}
	}
	packageParts := strings.Split(packageName, "/")
	filename := "lib" + packageParts[len(packageParts)-1] + ".a"
	parts := []string{goPathDirs[0], "pkg"}
	parts = append(parts, runtime.GOOS+"_"+runtime.GOARCH)
	parts = append(parts, packageParts...)
	parts = append(parts, "_lib", filename)
	return path.Join(parts...), nil
}

func DoStuff() {
	hierarchy := NewHierarchy("autogen")
	hierarchy.Build()
}

type Hierarchy struct {
	ref *C.CFCHierarchy
}

type BindCore struct {
	ref *C.CFCBindCore
}

type BindC struct {
	ref *C.CFCC
}

func NewHierarchy(dest string) *Hierarchy {
	destCString := C.CString(dest)
	defer C.free(unsafe.Pointer(destCString))
	obj := &Hierarchy{C.CFCHierarchy_new(destCString)}
	obj.AddIncludeDir(mainIncDir)
	runtime.SetFinalizer(obj, (*Hierarchy).finalize)
	return obj
}

func (obj *Hierarchy) finalize() {
	C.CFCBase_decref((*C.CFCBase)(unsafe.Pointer(obj.ref)))
}

func (obj *Hierarchy) Build() {
	C.CFCHierarchy_build(obj.ref)
}

func (obj *Hierarchy) AddSourceDir(dir string) {
	dirCString := C.CString(dir)
	defer C.free(unsafe.Pointer(dirCString))
	C.CFCHierarchy_add_source_dir(obj.ref, dirCString)
}

func (obj *Hierarchy) AddIncludeDir(dir string) {
	dirCString := C.CString(dir)
	defer C.free(unsafe.Pointer(dirCString))
	C.CFCHierarchy_add_include_dir(obj.ref, dirCString)
}

func (obj *Hierarchy) WriteLog() {
	C.CFCHierarchy_write_log(obj.ref)
}

func NewBindCore(hierarchy *Hierarchy, header string, footer string) *BindCore {
	headerCString := C.CString(header)
	footerCString := C.CString(footer)
	defer C.free(unsafe.Pointer(headerCString))
	defer C.free(unsafe.Pointer(footerCString))
	obj := &BindCore{
		C.CFCBindCore_new(hierarchy.ref, headerCString, footerCString),
	}
	runtime.SetFinalizer(obj, (*BindCore).finalize)
	return obj
}

func (obj *BindCore) finalize() {
	C.CFCBase_decref((*C.CFCBase)(unsafe.Pointer(obj.ref)))
}

func (obj *BindCore) WriteAllModified(modified bool) bool {
	var mod C.int = 0
	if modified {
		mod = 1
	}
	return C.CFCBindCore_write_all_modified(obj.ref, mod) != 0
}

func NewBindC(hierarchy *Hierarchy, header string, footer string) *BindC {
	headerCString := C.CString(header)
	footerCString := C.CString(footer)
	defer C.free(unsafe.Pointer(headerCString))
	defer C.free(unsafe.Pointer(footerCString))
	obj := &BindC{
		C.CFCC_new(hierarchy.ref, headerCString, footerCString),
	}
	runtime.SetFinalizer(obj, (*BindC).finalize)
	return obj
}

func (obj *BindC) finalize() {
	C.CFCBase_decref((*C.CFCBase)(unsafe.Pointer(obj.ref)))
}

func (obj *BindC) WriteCallbacks() {
	// no-op
}

func (obj *BindC) WriteHostDefs() {
	C.CFCC_write_hostdefs(obj.ref)
}
