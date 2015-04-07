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


#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define CFC_NEED_BASE_STRUCT_DEF
#include "CFCBase.h"
#include "CFCGoMethod.h"
#include "CFCUtil.h"
#include "CFCClass.h"
#include "CFCFunction.h"
#include "CFCMethod.h"
#include "CFCSymbol.h"
#include "CFCType.h"
#include "CFCParcel.h"
#include "CFCParamList.h"
#include "CFCGoTypeMap.h"
#include "CFCVariable.h"

#ifndef true
    #define true 1
    #define false 0
#endif

struct CFCGoMethod {
    CFCBase     base;
    CFCMethod  *method;
};

static void
S_CFCGoMethod_destroy(CFCGoMethod *self);

static const CFCMeta CFCGOMETHOD_META = {
    "Clownfish::CFC::Binding::Go::Method",
    sizeof(CFCGoMethod),
    (CFCBase_destroy_t)S_CFCGoMethod_destroy
};

CFCGoMethod*
CFCGoMethod_new(CFCMethod *method) {
    CFCGoMethod *self
        = (CFCGoMethod*)CFCBase_allocate(&CFCGOMETHOD_META);
    self->method = (CFCMethod*)CFCBase_incref((CFCBase*)method);
    return self;
}

static void
S_CFCGoMethod_destroy(CFCGoMethod *self) {
    CFCBase_decref((CFCBase*)self->method);
    CFCBase_destroy((CFCBase*)self);
}

