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

func TestClassGetName(t *testing.T) {
	className := "Clownfish::Hash"
	class := FetchClass(className)
	if got := class.GetName(); got != className {
		t.Errorf("Expected %s, got %s", className, got)
	}
}

func TestClassGetParent(t *testing.T) {
	hashClass := FetchClass("Clownfish::Hash")
	parent := hashClass.GetParent();
	if parentName := parent.GetName(); parentName != "Clownfish::Obj" {
		t.Errorf("Expected Clownfish::Obj, got %s", parentName)
	}
}

func TestClassGetObjAllocSize(t *testing.T) {
	intClass := FetchClass("Clownfish::Integer")
	objClass := FetchClass("Clownfish::Obj")
	if intClass.GetObjAllocSize() <= objClass.GetObjAllocSize() {
		t.Error("Unexpected result for getObjAllocSize")
	}
}

func TestMakeObj(t *testing.T) {
	intClass := FetchClass("Clownfish::Integer")
	o := intClass.MakeObj()
	if _, ok := o.(Integer); !ok {
		t.Error("MakeObj for Integer class didn't yield an Integer")
	}
}
