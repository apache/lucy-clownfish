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

type Parcel struct {
	ref *C.CFCParcel
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

type BindGo struct {
	ref *C.CFCGo
}

type BindGoClass struct {
	ref *C.CFCGoClass
}

func FetchParcel(name string) *Parcel {
	nameC := C.CString(name)
	defer C.free(unsafe.Pointer(nameC))
	parcelC := C.CFCParcel_fetch(nameC)
	if parcelC == nil {
		return nil
	}
	obj := &Parcel{parcelC}
	runtime.SetFinalizer(obj, (*Parcel).finalize)
	return obj
}

func (obj *Parcel) finalize() {
	C.CFCBase_decref((*C.CFCBase)(unsafe.Pointer(obj.ref)))
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

func NewBindGo(hierarchy *Hierarchy) *BindGo {
	obj := &BindGo{
		C.CFCGo_new(hierarchy.ref),
	}
	runtime.SetFinalizer(obj, (*BindGo).finalize)
	return obj
}

func (obj *BindGo) finalize() {
	C.CFCBase_decref((*C.CFCBase)(unsafe.Pointer(obj.ref)))
}

func (obj *BindGo) SetHeader(header string) {
	headerCString := C.CString(header)
	defer C.free(unsafe.Pointer(headerCString))
	C.CFCGo_set_header(obj.ref, headerCString)
}

func (obj *BindGo) SetFooter(header string) {
	headerCString := C.CString(header)
	defer C.free(unsafe.Pointer(headerCString))
	C.CFCGo_set_header(obj.ref, headerCString)
}

func (obj *BindGo) SetSuppressInit(suppressInit bool) {
	if suppressInit {
		C.CFCGo_set_suppress_init(obj.ref, C.int(1))
	} else {
		C.CFCGo_set_suppress_init(obj.ref, C.int(0))
	}
}

func (obj *BindGo) WriteBindings(parcel *Parcel, dest string) {
	destC := C.CString(dest)
	defer C.free(unsafe.Pointer(destC))
	C.CFCGo_write_bindings(obj.ref, parcel.ref, destC)
}

func RegisterParcelPackage(parcel, goPackage string) {
	parcelC := C.CString(parcel)
	defer C.free(unsafe.Pointer(parcelC))
	goPackageC := C.CString(goPackage)
	defer C.free(unsafe.Pointer(goPackageC))
	C.CFCGo_register_parcel_package(parcelC, goPackageC)
}

func NewGoClass(parcel *Parcel, className string) *BindGoClass {
	classNameC := C.CString(className)
	defer C.free(unsafe.Pointer(classNameC))
	obj := C.CFCGoClass_new(parcel.ref, classNameC)
	return wrapBindGoClass(unsafe.Pointer(obj))
}

func GoClassSingleton(className string) *BindGoClass {
	classNameC := C.CString(className)
	defer C.free(unsafe.Pointer(classNameC))
	singletonC := C.CFCGoClass_singleton(classNameC)
	if singletonC == nil {
		return nil
	}
	C.CFCBase_incref((*C.CFCBase)(unsafe.Pointer(singletonC)))
	return wrapBindGoClass(unsafe.Pointer(singletonC))
}

func wrapBindGoClass(ptr unsafe.Pointer) *BindGoClass {
	obj := &BindGoClass{(*C.CFCGoClass)(ptr)}
	runtime.SetFinalizer(obj, (*BindGoClass).finalize)
	return obj
}


func (obj *BindGoClass) finalize() {
	C.CFCBase_decref((*C.CFCBase)(unsafe.Pointer(obj.ref)))
}

func (obj *BindGoClass) Register() {
	C.CFCGoClass_register(obj.ref)
}
