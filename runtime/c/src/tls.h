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

#ifndef H_CLOWNFISH_TLS
#define H_CLOWNFISH_TLS 1

#include "charmony.h"

#include <setjmp.h>

#include "Clownfish/Err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Err *current_error;
    Err *thrown_error;
    jmp_buf *current_env;
} cfish_ErrContext;

void
cfish_Tls_init(void);

cfish_ErrContext*
cfish_Tls_get_err_context();

#ifdef CFISH_USE_SHORT_NAMES
  #define ErrContext            cfish_ErrContext
  #define Tls_init              cfish_Tls_init
  #define Tls_get_err_context   cfish_Tls_get_err_context
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_CLOWNFISH_TLS */

