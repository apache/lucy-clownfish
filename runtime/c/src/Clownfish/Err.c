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

#include "Clownfish/Err.h"
#include "Clownfish/String.h"
#include "Clownfish/Util/Memory.h"
#include "Clownfish/Class.h"

typedef struct {
    Err *current_error;
    Err *thrown_error;
    jmp_buf *current_env;
} cfish_ErrGlobals;

/**************************** No thread support ****************************/
#ifdef CFISH_NOTHREADS

static cfish_ErrGlobals err_globals;

void
Err_init_class() {
}

static cfish_ErrGlobals*
S_get_globals() {
    return &err_globals;
}

/********************************** Windows ********************************/
#elif defined(CHY_HAS_WINDOWS_H)

#include <windows.h>

static DWORD err_globals_tls_index;

void
Err_init_class() {
    err_globals_tls_index = TlsAlloc();
    if (err_globals_tls_index == TLS_OUT_OF_INDEXES) {
        fprintf(stderr, "TlsAlloc failed (TLS_OUT_OF_INDEXES)\n");
        abort();
    }
}

static cfish_ErrGlobals*
S_get_globals() {
    cfish_ErrGlobals *globals
        = (cfish_ErrGlobals*)TlsGetValue(err_globals_tls_index);

    if (!globals) {
        globals = (cfish_ErrGlobals*)CALLOCATE(1, sizeof(cfish_ErrGlobals));
        if (!TlsSetValue(err_globals_tls_index, globals)) {
            fprintf(stderr, "TlsSetValue failed: %d\n", GetLastError());
            abort();
        }
    }

    return globals;
}

BOOL WINAPI
DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved) {
    if (reason == DLL_THREAD_DETACH) {
        cfish_ErrGlobals *globals
            = (cfish_ErrGlobals*)TlsGetValue(err_globals_tls_index);

        if (globals) {
            DECREF(globals->current_error);
            FREEMEM(globals);
        }
    }

    return TRUE;
}

/******************************** pthreads *********************************/
#elif defined(CHY_HAS_PTHREAD_H)

#include <pthread.h>

static pthread_key_t err_globals_key;

static void
S_destroy_globals(void *globals);

void
Err_init_class() {
    int error = pthread_key_create(&err_globals_key, S_destroy_globals);
    if (error) {
        fprintf(stderr, "pthread_key_create failed: %d\n", error);
        abort();
    }
}

static cfish_ErrGlobals*
S_get_globals() {
    cfish_ErrGlobals *globals
        = (cfish_ErrGlobals*)pthread_getspecific(err_globals_key);

    if (!globals) {
        globals = (cfish_ErrGlobals*)CALLOCATE(1, sizeof(cfish_ErrGlobals));
        int error = pthread_setspecific(err_globals_key, globals);
        if (error) {
            fprintf(stderr, "pthread_setspecific failed: %d\n", error);
            abort();
        }
    }

    return globals;
}

static void
S_destroy_globals(void *arg) {
    cfish_ErrGlobals *globals = (cfish_ErrGlobals*)arg;
    DECREF(globals->current_error);
    FREEMEM(globals);
}

/****************** No support for thread-local storage ********************/
#else

#error "No support for thread-local storage."

#endif

Err*
Err_get_error() {
    return S_get_globals()->current_error;
}

void
Err_set_error(Err *error) {
    cfish_ErrGlobals *globals = S_get_globals();

    if (globals->current_error) {
        DECREF(globals->current_error);
    }
    globals->current_error = error;
}

void
Err_do_throw(Err *error) {
    cfish_ErrGlobals *globals = S_get_globals();

    if (globals->current_env) {
        globals->thrown_error = error;
        longjmp(*globals->current_env, 1);
    }
    else {
        String *message = Err_Get_Mess(error);
        char *utf8 = Str_To_Utf8(message);
        fprintf(stderr, "%s", utf8);
        FREEMEM(utf8);
        exit(EXIT_FAILURE);
    }
}

void*
Err_To_Host_IMP(Err *self) {
    UNUSED_VAR(self);
    THROW(ERR, "TODO");
    UNREACHABLE_RETURN(void*);
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
Err_trap(Err_Attempt_t routine, void *context) {
    cfish_ErrGlobals *globals = S_get_globals();

    jmp_buf  env;
    jmp_buf *prev_env = globals->current_env;
    globals->current_env = &env;

    if (!setjmp(env)) {
        routine(context);
    }

    globals->current_env = prev_env;

    Err *error = globals->thrown_error;
    globals->thrown_error = NULL;
    return error;
}

