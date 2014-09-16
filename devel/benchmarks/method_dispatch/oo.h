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

#ifndef OO_H
#define OO_H

#include <stddef.h>
#include <stdint.h>

typedef struct class_t class_t;
typedef struct class_t interface_t;

typedef struct obj_t {
    size_t    refcount;
    class_t  *klass;
    volatile uint64_t value;
} obj_t;

typedef void (*method_t)(obj_t *obj);

struct class_t {
    char         *name;
    size_t        class_size;
    interface_t **itables[8];
    method_t      vtable[1];
};

#define ITABLE_ARRAY_SHIFT  16
#define ITABLE_ARRAY_MASK   0xFFFF0000
#define INTERFACE_ID_SHIFT  8
#define INTERFACE_ID_MASK   0x0000FF00
#define IMETHOD_OFFSET_MASK 0x000000FF

#endif /* OO_H */

