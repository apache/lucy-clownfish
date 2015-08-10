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

func TestMethodGetName(t *testing.T) {
	meth := NewMethod("Do_Stuff", unsafe.Pointer(nil), 32)
	if name := meth.GetName(); name != "Do_Stuff" {
		t.Errorf("Expected \"Do_Stuff\", got %s", name)
	}
}

func TestMethodGetHostAlias(t *testing.T) {
	meth := NewMethod("Do_Stuff", unsafe.Pointer(nil), 32)
	alias := "GetStuffDone"
	meth.SetHostAlias(alias)
	if got := meth.GetHostAlias(); got != alias {
		t.Errorf("Expected %v, got %v", alias, got)
	}
}

func TestMethodHostName(t *testing.T) {
	meth := NewMethod("Do_Stuff", unsafe.Pointer(nil), 32)
	expected := "DoStuff"
	if hostName := meth.HostName(); hostName != expected {
		t.Errorf("Expected %v, got %v", expected, hostName)
	}
}

func TestMethodIsExcludedFromHost(t *testing.T) {
	meth := NewMethod("Do_Stuff", unsafe.Pointer(nil), 32)
	if meth.IsExcludedFromHost() {
		t.Errorf("Meth should not be excluded")
	}
}
