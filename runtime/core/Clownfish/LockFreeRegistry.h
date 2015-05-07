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

/** Specialized lock free hash table for storing Classes.
 */

struct cfish_Obj;
struct cfish_String;

typedef struct cfish_LockFreeRegistry cfish_LockFreeRegistry;

cfish_LockFreeRegistry*
cfish_LFReg_new(size_t capacity);

cfish_LockFreeRegistry*
cfish_LFReg_init(cfish_LockFreeRegistry *self, size_t capacity);

void
CFISH_LFReg_Destroy(cfish_LockFreeRegistry *self);

bool
CFISH_LFReg_Register(cfish_LockFreeRegistry *self, struct cfish_String *key,
                     struct cfish_Obj *value);

struct cfish_Obj*
CFISH_LFReg_Fetch(cfish_LockFreeRegistry *self, struct cfish_String *key);

#ifdef CFISH_USE_SHORT_NAMES
  #define LockFreeRegistry cfish_LockFreeRegistry
  #define LFReg_new        cfish_LFReg_new
  #define LFReg_init       cfish_LFReg_init
  #define LFReg_Destroy    CFISH_LFReg_Destroy
  #define LFReg_Register   CFISH_LFReg_Register
  #define LFReg_Fetch      CFISH_LFReg_Fetch
#endif

