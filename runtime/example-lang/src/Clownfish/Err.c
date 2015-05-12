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

#include "CFBind.h"

void
cfish_Err_init_class() {
}

cfish_Err*
cfish_Err_get_error() {
    THROW(CFISH_ERR, "TODO");
    UNREACHABLE_RETURN(cfish_Err*);
}

void
cfish_Err_set_error(cfish_Err *error) {
    THROW(CFISH_ERR, "TODO");
}

void
cfish_Err_do_throw(cfish_Err *err) {
    THROW(CFISH_ERR, "TODO");
}

void
cfish_Err_throw_mess(cfish_Class *klass, cfish_String *message) {
    THROW(CFISH_ERR, "TODO");
}

void
cfish_Err_warn_mess(cfish_String *message) {
    THROW(CFISH_ERR, "TODO");
}

cfish_Err*
cfish_Err_trap(CFISH_Err_Attempt_t routine, void *context) {
    THROW(CFISH_ERR, "TODO");
    UNREACHABLE_RETURN(cfish_Err*);
}

