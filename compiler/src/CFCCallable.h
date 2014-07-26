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

/** Clownfish::CFC::Model::Callable - Base class for functions and methods.
 */

#ifndef H_CFCCALLABLE
#define H_CFCCALLABLE

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CFCCallable CFCCallable;
struct CFCParcel;
struct CFCType;
struct CFCDocuComment;
struct CFCParamList;
struct CFCClass;

#ifdef CFC_NEED_CALLABLE_STRUCT_DEF
#define CFC_NEED_SYMBOL_STRUCT_DEF
#include "CFCSymbol.h"
struct CFCCallable {
    CFCSymbol symbol;
    struct CFCType *return_type;
    struct CFCParamList *param_list;
    struct CFCDocuComment *docucomment;
};
#endif

/**
 * @param parcel A Clownfish::CFC::Model::Parcel.
 * @param exposure The callable's exposure (see
 * L<Clownfish::CFC::Model::Symbol>).
 * @param class_name The full name of the class in whose namespace the
 * function resides.
 * @param class_nickname The C nickname for the class.
 * @param name The name of the callable, without any namespacing prefixes.
 * @param return_type A Clownfish::CFC::Model::Type representing the
 * callable's return type.
 * @param param_list A Clownfish::CFC::Model::ParamList representing the
 * callable's argument list.
 * @param docucomment A Clownfish::CFC::Model::DocuComment describing the
 * callable.
 */
CFCCallable*
CFCCallable_init(CFCCallable *self, struct CFCParcel *parcel,
                 const char *exposure, const char *class_name,
                 const char *class_nickname, const char *name,
                 struct CFCType *return_type, struct CFCParamList *param_list,
                 struct CFCDocuComment *docucomment);

void
CFCCallable_destroy(CFCCallable *self);

/** Find the actual class of all unprefixed object types.
 */
void
CFCCallable_resolve_types(CFCCallable *self);

/** Test whether bindings can be generated for a callable.
  */
int
CFCCallable_can_be_bound(CFCCallable *self);

#ifdef __cplusplus
}
#endif

#endif /* H_CFCCALLABLE */

