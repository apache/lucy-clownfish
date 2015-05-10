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

#define CFISH_USE_SHORT_NAMES
#define C_CFISH_ERR

#include "charmony.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "tls.h"
#include "Clownfish/Err.h"
#include "Clownfish/String.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Class.h"

void
Err_init_class() {
    Tls_init();
}

Err*
Err_get_error() {
    return Tls_get_err_context()->current_error;
}

void
Err_set_error(Err *error) {
    ErrContext *context = Tls_get_err_context();

    if (context->current_error) {
        DECREF(context->current_error);
    }
    context->current_error = error;
}

void
Err_do_throw(Err *error) {
    ErrContext *context = Tls_get_err_context();

    if (context->current_env) {
        context->thrown_error = error;
        longjmp(*context->current_env, 1);
    }
    else {
        String *message = Err_Get_Mess(error);
        char *utf8 = Str_To_Utf8(message);
        fprintf(stderr, "%s", utf8);
        FREEMEM(utf8);
        exit(EXIT_FAILURE);
    }
}

void
Err_throw_mess(Class *klass, String *message) {
    UNUSED_VAR(klass);
    Err *err = Err_new(message);
    Err_do_throw(err);
}

void
Err_warn_mess(String *message) {
    char *utf8 = Str_To_Utf8(message);
    fprintf(stderr, "%s", utf8);
    FREEMEM(utf8);
    DECREF(message);
}

Err*
Err_trap(Err_Attempt_t routine, void *routine_context) {
    ErrContext *err_context = Tls_get_err_context();

    jmp_buf  env;
    jmp_buf *prev_env = err_context->current_env;
    err_context->current_env = &env;

    if (!setjmp(env)) {
        routine(routine_context);
    }

    err_context->current_env = prev_env;

    Err *error = err_context->thrown_error;
    err_context->thrown_error = NULL;
    return error;
}

