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

#include "charmony.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "tls.h"

#include "Clownfish/Util/Memory.h"

/**************************** No thread support ****************************/
#ifdef CFISH_NOTHREADS

static ErrContext err_context;

void
Tls_init() {
}

ErrContext*
Tls_get_err_context() {
    return &err_context;
}

/********************************** Windows ********************************/
#elif defined(CHY_HAS_WINDOWS_H)

#include <windows.h>

static DWORD err_context_tls_index;

void
Tls_init() {
    DWORD tls_index = TlsAlloc();
    if (tls_index == TLS_OUT_OF_INDEXES) {
        fprintf(stderr, "TlsAlloc failed (TLS_OUT_OF_INDEXES)\n");
        abort();
    }
    LONG old_index = InterlockedCompareExchange((LONG*)&err_context_tls_index,
                                                tls_index, 0);
    if (old_index != 0) {
        TlsFree(tls_index);
    }
}

ErrContext*
Tls_get_err_context() {
    ErrContext *context
        = (ErrContext*)TlsGetValue(err_context_tls_index);

    if (!context) {
        context = (ErrContext*)CALLOCATE(1, sizeof(ErrContext));
        if (!TlsSetValue(err_context_tls_index, context)) {
            fprintf(stderr, "TlsSetValue failed: %d\n", GetLastError());
            abort();
        }
    }

    return context;
}

BOOL WINAPI
DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved) {
    if (reason == DLL_THREAD_DETACH) {
        ErrContext *context
            = (ErrContext*)TlsGetValue(err_context_tls_index);

        if (context) {
            DECREF(context->current_error);
            FREEMEM(context);
        }
    }

    return TRUE;
}

/******************************** pthreads *********************************/
#elif defined(CHY_HAS_PTHREAD_H)

#include <pthread.h>

static pthread_key_t err_context_key;

static void
S_destroy_context(void *context);

void
Tls_init() {
    int error = pthread_key_create(&err_context_key, S_destroy_context);
    if (error) {
        fprintf(stderr, "pthread_key_create failed: %d\n", error);
        abort();
    }
}

ErrContext*
Tls_get_err_context() {
    ErrContext *context
        = (ErrContext*)pthread_getspecific(err_context_key);

    if (!context) {
        context = (ErrContext*)CALLOCATE(1, sizeof(ErrContext));
        int error = pthread_setspecific(err_context_key, context);
        if (error) {
            fprintf(stderr, "pthread_setspecific failed: %d\n", error);
            abort();
        }
    }

    return context;
}

static void
S_destroy_context(void *arg) {
    ErrContext *context = (ErrContext*)arg;
    DECREF(context->current_error);
    FREEMEM(context);
}

/****************** No support for thread-local storage ********************/
#else

#error "No support for thread-local storage."

#endif


