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

#ifndef H_CFISH_PY_CFBIND
#define H_CFISH_PY_CFBIND 1

#ifdef __cplusplus
extern "C" {
#endif

#include "cfish_platform.h"
#include "Python.h"

struct cfish_Class;
struct cfish_String;

/** Wrap the current state of Python's sys.exc_info in a Clownfish Err and
  * throw it.
  *
  * One refcount of `mess` will be consumed by this function.
  *
  * TODO: Leave the original exception intact.
  */
void
CFBind_reraise_pyerr(struct cfish_Class *err_klass, struct cfish_String *mess);

#ifdef __cplusplus
}
#endif

#endif // H_CFISH_PY_CFBIND

