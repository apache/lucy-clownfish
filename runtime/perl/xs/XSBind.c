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

#include <ctype.h>
#include <string.h>

#define C_CFISH_OBJ
#define C_CFISH_CLASS
#define NEED_newRV_noinc
#include "charmony.h"
#include "XSBind.h"
#include "Clownfish/CharBuf.h"
#include "Clownfish/HashIterator.h"
#include "Clownfish/Method.h"
#include "Clownfish/TestHarness/TestUtils.h"
#include "Clownfish/Util/Atomic.h"
#include "Clownfish/Util/StringHelper.h"
#include "Clownfish/Util/Memory.h"

#define XSBIND_REFCOUNT_FLAG   1
#define XSBIND_REFCOUNT_SHIFT  1

// Convert a Perl hash into a Clownfish Hash.  Caller takes responsibility for
// a refcount.
static cfish_Hash*
S_perl_hash_to_cfish_hash(pTHX_ HV *phash);

// Convert a Perl array into a Clownfish Vector.  Caller takes responsibility
// for a refcount.
static cfish_Vector*
S_perl_array_to_cfish_array(pTHX_ AV *parray);

// Convert a Vector to a Perl array.  Caller takes responsibility for a
// refcount.
static SV*
S_cfish_array_to_perl_array(pTHX_ cfish_Vector *varray);

// Convert a Hash to a Perl hash.  Caller takes responsibility for a refcount.
static SV*
S_cfish_hash_to_perl_hash(pTHX_ cfish_Hash *hash);

cfish_Obj*
XSBind_new_blank_obj(pTHX_ SV *either_sv) {
    cfish_Class *klass;

    // Get a Class.
    if (sv_isobject(either_sv)
        && sv_derived_from(either_sv, "Clownfish::Obj")
       ) {
        // Use the supplied object's Class.
        IV iv_ptr = SvIV(SvRV(either_sv));
        cfish_Obj *self = INT2PTR(cfish_Obj*, iv_ptr);
        klass = self->klass;
    }
    else {
        // Use the supplied class name string to find a Class.
        STRLEN len;
        char *ptr = SvPVutf8(either_sv, len);
        cfish_String *class_name = CFISH_SSTR_WRAP_UTF8(ptr, len);
        klass = cfish_Class_singleton(class_name, NULL);
    }

    // Use the Class to allocate a new blank object of the right size.
    return CFISH_Class_Make_Obj(klass);
}

cfish_Obj*
XSBind_sv_to_cfish_obj(pTHX_ SV *sv, cfish_Class *klass, void *allocation) {
    cfish_Obj *retval
        = XSBind_maybe_sv_to_cfish_obj(aTHX_ sv, klass, allocation);
    if (!retval) {
        THROW(CFISH_ERR, "Not a %o", CFISH_Class_Get_Name(klass));
    }
    return retval;
}

cfish_Obj*
XSBind_maybe_sv_to_cfish_obj(pTHX_ SV *sv, cfish_Class *klass,
                             void *allocation) {
    cfish_Obj *retval = NULL;
    if (XSBind_sv_defined(aTHX_ sv)) {
        // Assume that the class name is always NULL-terminated. Somewhat
        // dangerous but should be safe.
        if (sv_isobject(sv)
            && sv_derived_from(sv, CFISH_Str_Get_Ptr8(CFISH_Class_Get_Name(klass)))
           ) {
            // Unwrap a real Clownfish object.
            IV tmp = SvIV(SvRV(sv));
            retval = INT2PTR(cfish_Obj*, tmp);
        }
        else if (allocation &&
                 (klass == CFISH_STRING
                  || klass == CFISH_OBJ)
                ) {
            // Wrap the string from an ordinary Perl scalar inside a
            // stack String.
            STRLEN size;
            char *ptr = SvPVutf8(sv, size);
            retval = (cfish_Obj*)cfish_Str_new_stack_string(
                    allocation, ptr, size);
        }
        else if (SvROK(sv)) {
            // Attempt to convert Perl hashes and arrays into their Clownfish
            // analogues.
            SV *inner = SvRV(sv);
            if (SvTYPE(inner) == SVt_PVAV && klass == CFISH_VECTOR) {
                retval = (cfish_Obj*)
                         S_perl_array_to_cfish_array(aTHX_ (AV*)inner);
            }
            else if (SvTYPE(inner) == SVt_PVHV && klass == CFISH_HASH) {
                retval = (cfish_Obj*)
                         S_perl_hash_to_cfish_hash(aTHX_ (HV*)inner);
            }

            if (retval) {
                // Mortalize the converted object -- which is somewhat
                // dangerous, but is the only way to avoid requiring that the
                // caller take responsibility for a refcount.
                SV *mortal = (SV*)CFISH_Obj_To_Host(retval);
                CFISH_DECREF(retval);
                sv_2mortal(mortal);
            }
        }
    }

    return retval;
}

SV*
XSBind_cfish_to_perl(pTHX_ cfish_Obj *obj) {
    if (obj == NULL) {
        return newSV(0);
    }
    else if (CFISH_Obj_Is_A(obj, CFISH_STRING)) {
        return XSBind_str_to_sv(aTHX_ (cfish_String*)obj);
    }
    else if (CFISH_Obj_Is_A(obj, CFISH_BLOB)) {
        return XSBind_blob_to_sv(aTHX_ (cfish_Blob*)obj);
    }
    else if (CFISH_Obj_Is_A(obj, CFISH_BYTEBUF)) {
        return XSBind_bb_to_sv(aTHX_ (cfish_ByteBuf*)obj);
    }
    else if (CFISH_Obj_Is_A(obj, CFISH_VECTOR)) {
        return S_cfish_array_to_perl_array(aTHX_ (cfish_Vector*)obj);
    }
    else if (CFISH_Obj_Is_A(obj, CFISH_HASH)) {
        return S_cfish_hash_to_perl_hash(aTHX_ (cfish_Hash*)obj);
    }
    else if (CFISH_Obj_Is_A(obj, CFISH_FLOATNUM)) {
        return newSVnv(CFISH_Obj_To_F64(obj));
    }
    else if (obj == (cfish_Obj*)CFISH_TRUE) {
        return newSViv(1);
    }
    else if (obj == (cfish_Obj*)CFISH_FALSE) {
        return newSViv(0);
    }
    else if (sizeof(IV) == 8 && CFISH_Obj_Is_A(obj, CFISH_INTNUM)) {
        int64_t num = CFISH_Obj_To_I64(obj);
        return newSViv((IV)num);
    }
    else if (sizeof(IV) == 4 && CFISH_Obj_Is_A(obj, CFISH_INTEGER32)) {
        int32_t num = (int32_t)CFISH_Obj_To_I64(obj);
        return newSViv((IV)num);
    }
    else if (sizeof(IV) == 4 && CFISH_Obj_Is_A(obj, CFISH_INTEGER64)) {
        int64_t num = CFISH_Obj_To_I64(obj);
        return newSVnv((double)num); // lossy
    }
    else {
        return (SV*)CFISH_Obj_To_Host(obj);
    }
}

cfish_Obj*
XSBind_perl_to_cfish(pTHX_ SV *sv) {
    cfish_Obj *retval = NULL;

    if (XSBind_sv_defined(aTHX_ sv)) {
        bool is_undef = false;

        if (SvROK(sv)) {
            // Deep conversion of references.
            SV *inner = SvRV(sv);
            if (SvTYPE(inner) == SVt_PVAV) {
                retval = (cfish_Obj*)
                         S_perl_array_to_cfish_array(aTHX_ (AV*)inner);
            }
            else if (SvTYPE(inner) == SVt_PVHV) {
                retval = (cfish_Obj*)
                         S_perl_hash_to_cfish_hash(aTHX_ (HV*)inner);
            }
            else if (sv_isobject(sv)
                     && sv_derived_from(sv, "Clownfish::Obj")
                    ) {
                IV tmp = SvIV(inner);
                retval = INT2PTR(cfish_Obj*, tmp);
                (void)CFISH_INCREF(retval);
            }
            else if (!XSBind_sv_defined(aTHX_ inner)) {
                // Reference to undef. After cloning a Perl interpeter,
                // most Clownfish objects look like this after they're
                // CLONE_SKIPped.
                is_undef = true;
            }
        }

        // It's either a plain scalar or a non-Clownfish Perl object, so
        // stringify.
        if (!retval && !is_undef) {
            STRLEN len;
            char *ptr = SvPVutf8(sv, len);
            retval = (cfish_Obj*)cfish_Str_new_from_trusted_utf8(ptr, len);
        }
    }
    else if (sv) {
        // Deep conversion of raw AVs and HVs.
        if (SvTYPE(sv) == SVt_PVAV) {
            retval = (cfish_Obj*)S_perl_array_to_cfish_array(aTHX_ (AV*)sv);
        }
        else if (SvTYPE(sv) == SVt_PVHV) {
            retval = (cfish_Obj*)S_perl_hash_to_cfish_hash(aTHX_ (HV*)sv);
        }
    }

    return retval;
}

SV*
XSBind_blob_to_sv(pTHX_ cfish_Blob *blob) {
    return blob
           ? newSVpvn(CFISH_Blob_Get_Buf(blob), CFISH_Blob_Get_Size(blob))
           : newSV(0);
}

SV*
XSBind_bb_to_sv(pTHX_ cfish_ByteBuf *bb) {
    return bb
           ? newSVpvn(CFISH_BB_Get_Buf(bb), CFISH_BB_Get_Size(bb))
           : newSV(0);
}

SV*
XSBind_str_to_sv(pTHX_ cfish_String *str) {
    if (!str) {
        return newSV(0);
    }
    else {
        SV *sv = newSVpvn(CFISH_Str_Get_Ptr8(str), CFISH_Str_Get_Size(str));
        SvUTF8_on(sv);
        return sv;
    }
}

static cfish_Hash*
S_perl_hash_to_cfish_hash(pTHX_ HV *phash) {
    uint32_t    num_keys = hv_iterinit(phash);
    cfish_Hash *retval   = cfish_Hash_new(num_keys);

    while (num_keys--) {
        HE        *entry    = hv_iternext(phash);
        STRLEN     key_len  = HeKLEN(entry);
        SV        *value_sv = HeVAL(entry);
        cfish_Obj *value    = XSBind_perl_to_cfish(aTHX_ value_sv); // Recurse.

        // Force key to UTF-8 if necessary.
        if (key_len == (STRLEN)HEf_SVKEY) {
            // Key is stored as an SV.  Use its UTF-8 flag?  Not sure about
            // this.
            SV   *key_sv  = HeKEY_sv(entry);
            char *key_str = SvPVutf8(key_sv, key_len);
            CFISH_Hash_Store_Utf8(retval, key_str, key_len, value);
        }
        else if (HeKUTF8(entry)) {
            CFISH_Hash_Store_Utf8(retval, HeKEY(entry), key_len, value);
        }
        else {
            char *key_str = HeKEY(entry);
            bool pure_ascii = true;
            for (STRLEN i = 0; i < key_len; i++) {
                if ((key_str[i] & 0x80) == 0x80) { pure_ascii = false; }
            }
            if (pure_ascii) {
                CFISH_Hash_Store_Utf8(retval, key_str, key_len, value);
            }
            else {
                SV *key_sv = HeSVKEY_force(entry);
                key_str = SvPVutf8(key_sv, key_len);
                CFISH_Hash_Store_Utf8(retval, key_str, key_len, value);
            }
        }
    }

    return retval;
}

static cfish_Vector*
S_perl_array_to_cfish_array(pTHX_ AV *parray) {
    const uint32_t  size   = av_len(parray) + 1;
    cfish_Vector   *retval = cfish_Vec_new(size);

    // Iterate over array elems.
    for (uint32_t i = 0; i < size; i++) {
        SV **elem_sv = av_fetch(parray, i, false);
        if (elem_sv) {
            cfish_Obj *elem = XSBind_perl_to_cfish(aTHX_ *elem_sv);
            if (elem) { CFISH_Vec_Store(retval, i, elem); }
        }
    }
    CFISH_Vec_Resize(retval, size); // needed if last elem is NULL

    return retval;
}

static SV*
S_cfish_array_to_perl_array(pTHX_ cfish_Vector *varray) {
    AV *perl_array = newAV();
    uint32_t num_elems = CFISH_Vec_Get_Size(varray);

    // Iterate over array elems.
    if (num_elems) {
        av_fill(perl_array, num_elems - 1);
        for (uint32_t i = 0; i < num_elems; i++) {
            cfish_Obj *val = CFISH_Vec_Fetch(varray, i);
            if (val == NULL) {
                continue;
            }
            else {
                // Recurse for each value.
                SV *const val_sv = XSBind_cfish_to_perl(aTHX_ val);
                av_store(perl_array, i, val_sv);
            }
        }
    }

    return newRV_noinc((SV*)perl_array);
}

static SV*
S_cfish_hash_to_perl_hash(pTHX_ cfish_Hash *hash) {
    HV *perl_hash = newHV();
    cfish_HashIterator *iter = cfish_HashIter_new(hash);

    // Iterate over key-value pairs.
    while (CFISH_HashIter_Next(iter)) {
        cfish_String *key      = CFISH_HashIter_Get_Key(iter);
        const char   *key_ptr  = CFISH_Str_Get_Ptr8(key);
        I32           key_size = CFISH_Str_Get_Size(key);

        // Recurse for each value.
        cfish_Obj *val    = CFISH_HashIter_Get_Value(iter);
        SV        *val_sv = XSBind_cfish_to_perl(aTHX_ val);

        // Using a negative `klen` argument to signal UTF-8 is undocumented
        // in older Perl versions but works since 5.8.0.
        hv_store(perl_hash, key_ptr, -key_size, val_sv, 0);
    }

    CFISH_DECREF(iter);
    return newRV_noinc((SV*)perl_hash);
}

struct trap_context {
    SV *routine;
    SV *context;
};

static void
S_attempt_perl_call(void *context) {
    struct trap_context *args = (struct trap_context*)context;
    dTHX;
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    XPUSHs(sv_2mortal(newSVsv(args->context)));
    PUTBACK;
    call_sv(args->routine, G_DISCARD);
    FREETMPS;
    LEAVE;
}

cfish_Err*
XSBind_trap(SV *routine, SV *context) {
    struct trap_context args;
    args.routine = routine;
    args.context = context;
    return cfish_Err_trap(S_attempt_perl_call, &args);
}

void
XSBind_enable_overload(pTHX_ void *pobj) {
    SV *perl_obj = (SV*)pobj;
    HV *stash = SvSTASH(SvRV(perl_obj));
#if (PERL_VERSION > 10)
    Gv_AMupdate(stash, false);
#else
    Gv_AMupdate(stash);
#endif
    SvAMAGIC_on(perl_obj);
}

static bool
S_extract_from_sv(pTHX_ SV *value, void *target, const char *label,
                  bool required, int type, cfish_Class *klass,
                  void *allocation) {
    bool valid_assignment = false;

    if (XSBind_sv_defined(aTHX_ value)) {
        switch (type) {
            case XSBIND_WANT_I8:
                *((int8_t*)target) = (int8_t)SvIV(value);
                valid_assignment = true;
                break;
            case XSBIND_WANT_I16:
                *((int16_t*)target) = (int16_t)SvIV(value);
                valid_assignment = true;
                break;
            case XSBIND_WANT_I32:
                *((int32_t*)target) = (int32_t)SvIV(value);
                valid_assignment = true;
                break;
            case XSBIND_WANT_I64:
                if (sizeof(IV) == 8) {
                    *((int64_t*)target) = (int64_t)SvIV(value);
                }
                else { // sizeof(IV) == 4
                    // lossy.
                    *((int64_t*)target) = (int64_t)SvNV(value);
                }
                valid_assignment = true;
                break;
            case XSBIND_WANT_U8:
                *((uint8_t*)target) = (uint8_t)SvUV(value);
                valid_assignment = true;
                break;
            case XSBIND_WANT_U16:
                *((uint16_t*)target) = (uint16_t)SvUV(value);
                valid_assignment = true;
                break;
            case XSBIND_WANT_U32:
                *((uint32_t*)target) = (uint32_t)SvUV(value);
                valid_assignment = true;
                break;
            case XSBIND_WANT_U64:
                if (sizeof(UV) == 8) {
                    *((uint64_t*)target) = (uint64_t)SvUV(value);
                }
                else { // sizeof(UV) == 4
                    // lossy.
                    *((uint64_t*)target) = (uint64_t)SvNV(value);
                }
                valid_assignment = true;
                break;
            case XSBIND_WANT_BOOL:
                *((bool*)target) = !!SvTRUE(value);
                valid_assignment = true;
                break;
            case XSBIND_WANT_F32:
                *((float*)target) = (float)SvNV(value);
                valid_assignment = true;
                break;
            case XSBIND_WANT_F64:
                *((double*)target) = SvNV(value);
                valid_assignment = true;
                break;
            case XSBIND_WANT_OBJ: {
                    cfish_Obj *object
                        = XSBind_maybe_sv_to_cfish_obj(aTHX_ value, klass,
                                                       allocation);
                    if (object) {
                        *((cfish_Obj**)target) = object;
                        valid_assignment = true;
                    }
                    else {
                        cfish_String *mess
                            = CFISH_MAKE_MESS(
                                  "Invalid value for '%s' - not a %o",
                                  label, CFISH_Class_Get_Name(klass));
                        cfish_Err_set_error(cfish_Err_new(mess));
                        return false;
                    }
                }
                break;
            case XSBIND_WANT_SV:
                *((SV**)target) = value;
                valid_assignment = true;
                break;
            default: {
                    cfish_String *mess
                        = CFISH_MAKE_MESS("Unrecognized type: %i32 for param '%s'",
                                          (int32_t)type, label);
                    cfish_Err_set_error(cfish_Err_new(mess));
                    return false;
                }
        }
    }

    // Enforce that required params cannot be undef and must present valid
    // values.
    if (required && !valid_assignment) {
        cfish_String *mess = CFISH_MAKE_MESS("Missing required param %s",
                                             label);
        cfish_Err_set_error(cfish_Err_new(mess));
        return false;
    }

    return true;
}

static CFISH_INLINE bool
S_u1get(const void *array, uint32_t tick) {
    const uint8_t *const u8bits      = (const uint8_t*)array;
    const uint32_t       byte_offset = tick >> 3;
    const uint8_t        mask        = 1 << (tick & 0x7);
    return (u8bits[byte_offset] & mask) != 0;
}

static CFISH_INLINE void
S_u1set(void *array, uint32_t tick) {
    uint8_t *const u8bits      = (uint8_t*)array;
    const uint32_t byte_offset = tick >> 3;
    const uint8_t  mask        = 1 << (tick & 0x7);
    u8bits[byte_offset] |= mask;
}

bool
XSBind_allot_params(pTHX_ SV** stack, int32_t start, int32_t num_stack_elems,
                    ...) {
    va_list args;
    size_t size = sizeof(int64_t) + num_stack_elems / 64;
    void *verified_labels = alloca(size);
    memset(verified_labels, 0, size);

    // Verify that our args come in pairs. Return success if there are no
    // args.
    if ((num_stack_elems - start) % 2 != 0) {
        cfish_String *mess
            = CFISH_MAKE_MESS(
                  "Expecting hash-style params, got odd number of args");
        cfish_Err_set_error(cfish_Err_new(mess));
        return false;
    }

    void *target;
    va_start(args, num_stack_elems);
    while (NULL != (target = va_arg(args, void*))) {
        char *label     = va_arg(args, char*);
        int   label_len = va_arg(args, int);
        int   required  = va_arg(args, int);
        int   type      = va_arg(args, int);
        cfish_Class *klass = va_arg(args, cfish_Class*);
        void *allocation = va_arg(args, void*);

        // Iterate through the stack looking for labels which match this param
        // name.  If the label appears more than once, keep track of where it
        // appears *last*, as the last time a param appears overrides all
        // previous appearances.
        int32_t found_arg = -1;
        for (int32_t tick = start; tick < num_stack_elems; tick += 2) {
            SV *const key_sv = stack[tick];
            if (SvCUR(key_sv) == (STRLEN)label_len) {
                if (memcmp(SvPVX(key_sv), label, label_len) == 0) {
                    found_arg = tick;
                    S_u1set(verified_labels, tick);
                }
            }
        }

        if (found_arg == -1) {
            // Didn't find this parameter. Throw an error if it was required.
            if (required) {
                cfish_String *mess
                    = CFISH_MAKE_MESS("Missing required parameter: '%s'",
                                      label);
                cfish_Err_set_error(cfish_Err_new(mess));
                return false;
            }
        }
        else {
            // Found the arg.  Extract the value.
            SV *value = stack[found_arg + 1];
            bool got_arg = S_extract_from_sv(aTHX_ value, target, label,
                                             required, type, klass,
                                             allocation);
            if (!got_arg) {
                CFISH_ERR_ADD_FRAME(cfish_Err_get_error());
                return false;
            }
        }
    }
    va_end(args);

    // Ensure that all parameter labels were valid.
    for (int32_t tick = start; tick < num_stack_elems; tick += 2) {
        if (!S_u1get(verified_labels, tick)) {
            SV *const key_sv = stack[tick];
            char *key = SvPV_nolen(key_sv);
            cfish_String *mess
                = CFISH_MAKE_MESS("Invalid parameter: '%s'", key);
            cfish_Err_set_error(cfish_Err_new(mess));
            return false;
        }
    }

    return true;
}

/***************************************************************************
 * The routines below are declared within the Clownfish core but left
 * unimplemented and must be defined for each host language.
 ***************************************************************************/

/**************************** Clownfish::Obj *******************************/

static CFISH_INLINE bool
SI_immortal(cfish_Class *klass) {
    if (klass == CFISH_CLASS
        || klass == CFISH_METHOD
        || klass == CFISH_BOOLNUM
       ){
        return true;
    }
    return false;
}

static CFISH_INLINE bool
SI_is_string_type(cfish_Class *klass) {
    if (klass == CFISH_STRING) {
        return true;
    }
    return false;
}

static void
S_lazy_init_host_obj(pTHX_ cfish_Obj *self) {
    SV *inner_obj = newSV(0);
    SvOBJECT_on(inner_obj);
#if (PERL_VERSION <= 16)
    PL_sv_objcount++;
#endif
    (void)SvUPGRADE(inner_obj, SVt_PVMG);
    sv_setiv(inner_obj, PTR2IV(self));

    // Connect class association.
    cfish_String *class_name = CFISH_Class_Get_Name(self->klass);
    HV *stash = gv_stashpvn(CFISH_Str_Get_Ptr8(class_name),
                            CFISH_Str_Get_Size(class_name), TRUE);
    SvSTASH_set(inner_obj, (HV*)SvREFCNT_inc(stash));

    /* Up till now we've been keeping track of the refcount in
     * self->ref.count.  We're replacing ref.count with ref.host_obj, which
     * will assume responsibility for maintaining the refcount.  ref.host_obj
     * starts off with a refcount of 1, so we need to transfer any refcounts
     * in excess of that. */
    cfish_ref_t old_ref = self->ref;
    size_t excess = (old_ref.count >> XSBIND_REFCOUNT_SHIFT) - 1;
    SvREFCNT(inner_obj) += excess;

    // Overwrite refcount with host object.
    cfish_Class *klass = self->klass;
    if (SI_immortal(klass)) {
        SvSHARE(inner_obj);
        if (!cfish_Atomic_cas_ptr((void**)&self->ref, old_ref.host_obj, inner_obj)) {
            // Another thread beat us to it.  Now we have a Perl object to defuse.
            SvSTASH_set(inner_obj, NULL);
            SvREFCNT_dec((SV*)stash);
            SvOBJECT_off(inner_obj);
            SvREFCNT(inner_obj) -= excess;
            SvREFCNT_dec(inner_obj);
#if (PERL_VERSION <= 16)
            PL_sv_objcount--;
#endif
        }
    }
    else {
        self->ref.host_obj = inner_obj;
    }
}

uint32_t
cfish_get_refcount(void *vself) {
    cfish_Obj *self = (cfish_Obj*)vself;
    cfish_ref_t ref = self->ref;
    return ref.count & XSBIND_REFCOUNT_FLAG
           ? ref.count >> XSBIND_REFCOUNT_SHIFT
           : SvREFCNT((SV*)ref.host_obj);
}

cfish_Obj*
cfish_inc_refcount(void *vself) {
    cfish_Obj *self = (cfish_Obj*)vself;

    // Handle special cases.
    cfish_Class *const klass = self->klass;
    if (klass->flags & CFISH_fREFCOUNTSPECIAL) {
        if (SI_is_string_type(klass)) {
            // Only copy-on-incref Strings get special-cased.  Ordinary
            // Strings fall through to the general case.
            if (CFISH_Str_Is_Copy_On_IncRef((cfish_String*)self)) {
                const char *utf8 = CFISH_Str_Get_Ptr8((cfish_String*)self);
                size_t size = CFISH_Str_Get_Size((cfish_String*)self);
                return (cfish_Obj*)cfish_Str_new_from_trusted_utf8(utf8, size);
            }
        }
        else if (SI_immortal(klass)) {
            return self;
        }
    }

    if (self->ref.count & XSBIND_REFCOUNT_FLAG) {
        if (self->ref.count == XSBIND_REFCOUNT_FLAG) {
            CFISH_THROW(CFISH_ERR, "Illegal refcount of 0");
        }
        self->ref.count += 1 << XSBIND_REFCOUNT_SHIFT;
    }
    else {
        SvREFCNT_inc_simple_void_NN((SV*)self->ref.host_obj);
    }
    return self;
}

uint32_t
cfish_dec_refcount(void *vself) {
    cfish_Obj *self = (cfish_Obj*)vself;

    cfish_Class *klass = self->klass;
    if (klass->flags & CFISH_fREFCOUNTSPECIAL) {
        if (SI_immortal(klass)) {
            return 1;
        }
    }

    uint32_t modified_refcount = I32_MAX;
    if (self->ref.count & XSBIND_REFCOUNT_FLAG) {
        if (self->ref.count == XSBIND_REFCOUNT_FLAG) {
            CFISH_THROW(CFISH_ERR, "Illegal refcount of 0");
        }
        if (self->ref.count
            == ((1 << XSBIND_REFCOUNT_SHIFT) | XSBIND_REFCOUNT_FLAG)) {
            modified_refcount = 0;
            CFISH_Obj_Destroy(self);
        }
        else {
            self->ref.count -= 1 << XSBIND_REFCOUNT_SHIFT;
            modified_refcount = self->ref.count >> XSBIND_REFCOUNT_SHIFT;
        }
    }
    else {
        dTHX;
        modified_refcount = SvREFCNT((SV*)self->ref.host_obj) - 1;
        // If the SV's refcount falls to 0, DESTROY will be invoked from
        // Perl-space.
        SvREFCNT_dec((SV*)self->ref.host_obj);
    }
    return modified_refcount;
}

void*
CFISH_Obj_To_Host_IMP(cfish_Obj *self) {
    dTHX;
    if (self->ref.count & XSBIND_REFCOUNT_FLAG) {
        S_lazy_init_host_obj(aTHX_ self);
    }
    return newRV_inc((SV*)self->ref.host_obj);
}

/*************************** Clownfish::Class ******************************/

cfish_Obj*
CFISH_Class_Make_Obj_IMP(cfish_Class *self) {
    cfish_Obj *obj
        = (cfish_Obj*)cfish_Memory_wrapped_calloc(self->obj_alloc_size, 1);
    obj->klass = self;
    obj->ref.count = (1 << XSBIND_REFCOUNT_SHIFT) | XSBIND_REFCOUNT_FLAG;
    return obj;
}

cfish_Obj*
CFISH_Class_Init_Obj_IMP(cfish_Class *self, void *allocation) {
    memset(allocation, 0, self->obj_alloc_size);
    cfish_Obj *obj = (cfish_Obj*)allocation;
    obj->klass = self;
    obj->ref.count = (1 << XSBIND_REFCOUNT_SHIFT) | XSBIND_REFCOUNT_FLAG;
    return obj;
}

cfish_Obj*
CFISH_Class_Foster_Obj_IMP(cfish_Class *self, void *host_obj) {
    dTHX;
    cfish_Obj *obj
        = (cfish_Obj*)cfish_Memory_wrapped_calloc(self->obj_alloc_size, 1);
    SV *inner_obj = SvRV((SV*)host_obj);
    obj->klass = self;
    sv_setiv(inner_obj, PTR2IV(obj));
    obj->ref.host_obj = inner_obj;
    return obj;
}

void
cfish_Class_register_with_host(cfish_Class *singleton, cfish_Class *parent) {
    dTHX;
    dSP;
    ENTER;
    SAVETMPS;
    EXTEND(SP, 2);
    PUSHMARK(SP);
    mPUSHs((SV*)CFISH_Class_To_Host(singleton));
    mPUSHs((SV*)CFISH_Class_To_Host(parent));
    PUTBACK;
    call_pv("Clownfish::Class::_register", G_VOID | G_DISCARD);
    FREETMPS;
    LEAVE;
}

cfish_Vector*
cfish_Class_fresh_host_methods(cfish_String *class_name) {
    dTHX;
    dSP;
    ENTER;
    SAVETMPS;
    EXTEND(SP, 1);
    PUSHMARK(SP);
    mPUSHs(XSBind_str_to_sv(aTHX_ class_name));
    PUTBACK;
    call_pv("Clownfish::Class::_fresh_host_methods", G_SCALAR);
    SPAGAIN;
    cfish_Vector *methods = (cfish_Vector*)XSBind_perl_to_cfish(aTHX_ POPs);
    PUTBACK;
    FREETMPS;
    LEAVE;
    return methods;
}

cfish_String*
cfish_Class_find_parent_class(cfish_String *class_name) {
    dTHX;
    dSP;
    ENTER;
    SAVETMPS;
    EXTEND(SP, 1);
    PUSHMARK(SP);
    mPUSHs(XSBind_str_to_sv(aTHX_ class_name));
    PUTBACK;
    call_pv("Clownfish::Class::_find_parent_class", G_SCALAR);
    SPAGAIN;
    SV *parent_class_sv = POPs;
    PUTBACK;
    cfish_String *parent_class
        = (cfish_String*)XSBind_perl_to_cfish(aTHX_ parent_class_sv);
    FREETMPS;
    LEAVE;
    return parent_class;
}

/*************************** Clownfish::Method ******************************/

cfish_String*
CFISH_Method_Host_Name_IMP(cfish_Method *self) {
    cfish_String *host_alias = CFISH_Method_Get_Host_Alias(self);
    if (host_alias) {
        return (cfish_String*)CFISH_INCREF(host_alias);
    }

    // Convert to lowercase.
    cfish_String *name = CFISH_Method_Get_Name(self);
    cfish_CharBuf *buf = cfish_CB_new(CFISH_Str_Get_Size(name));
    cfish_StringIterator *iter = CFISH_Str_Top(name);
    int32_t code_point;
    while (CFISH_STRITER_DONE != (code_point = CFISH_StrIter_Next(iter))) {
        if (code_point > 127) {
            THROW(CFISH_ERR, "Can't lowercase '%o'", name);
        }
        else {
            CFISH_CB_Cat_Char(buf, tolower(code_point));
        }
    }
    cfish_String *retval = CFISH_CB_Yield_String(buf);
    CFISH_DECREF(iter);
    CFISH_DECREF(buf);

    return retval;
}

/***************************** Clownfish::Err *******************************/

// Anonymous XSUB helper for Err#trap().  It wraps the supplied C function
// so that it can be run inside a Perl eval block.
static SV *attempt_xsub = NULL;

XS(cfish_Err_attempt_via_xs) {
    dXSARGS;
    CFISH_UNUSED_VAR(cv);
    SP -= items;
    if (items != 2) {
        CFISH_THROW(CFISH_ERR, "Usage: $sub->(routine, context)");
    };
    IV routine_iv = SvIV(ST(0));
    IV context_iv = SvIV(ST(1));
    CFISH_Err_Attempt_t routine = INT2PTR(CFISH_Err_Attempt_t, routine_iv);
    void *context               = INT2PTR(void*, context_iv);
    routine(context);
    XSRETURN(0);
}

void
cfish_Err_init_class(void) {
    dTHX;
    char *file = (char*)__FILE__;
    attempt_xsub = (SV*)newXS(NULL, cfish_Err_attempt_via_xs, file);
}

cfish_Err*
cfish_Err_get_error() {
    dTHX;
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    PUTBACK;
    call_pv("Clownfish::Err::get_error", G_SCALAR);
    SPAGAIN;
    cfish_Err *error = (cfish_Err*)XSBind_perl_to_cfish(aTHX_ POPs);
    if (error) { CFISH_CERTIFY(error, CFISH_ERR); }
    PUTBACK;
    FREETMPS;
    LEAVE;
    return error;
}

void
cfish_Err_set_error(cfish_Err *error) {
    dTHX;
    dSP;
    ENTER;
    SAVETMPS;
    EXTEND(SP, 2);
    PUSHMARK(SP);
    PUSHmortal;
    if (error) {
        mPUSHs((SV*)CFISH_Err_To_Host(error));
    }
    else {
        PUSHmortal;
    }
    PUTBACK;
    call_pv("Clownfish::Err::set_error", G_VOID | G_DISCARD);
    FREETMPS;
    LEAVE;
}

void
cfish_Err_do_throw(cfish_Err *err) {
    dTHX;
    dSP;
    SV *error_sv = (SV*)CFISH_Err_To_Host(err);
    CFISH_DECREF(err);
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    XPUSHs(sv_2mortal(error_sv));
    PUTBACK;
    call_pv("Clownfish::Err::do_throw", G_DISCARD);
    FREETMPS;
    LEAVE;
}

void*
CFISH_Err_To_Host_IMP(cfish_Err *self) {
    dTHX;
    CFISH_Err_To_Host_t super_to_host
        = CFISH_SUPER_METHOD_PTR(CFISH_ERR, CFISH_Err_To_Host);
    SV *perl_obj = (SV*)super_to_host(self);
    XSBind_enable_overload(aTHX_ perl_obj);
    return perl_obj;
}

void
cfish_Err_throw_mess(cfish_Class *klass, cfish_String *message) {
    CFISH_UNUSED_VAR(klass);
    cfish_Err *err = cfish_Err_new(message);
    cfish_Err_do_throw(err);
}

void
cfish_Err_warn_mess(cfish_String *message) {
    dTHX;
    SV *error_sv = XSBind_str_to_sv(aTHX_ message);
    CFISH_DECREF(message);
    warn("%s", SvPV_nolen(error_sv));
    SvREFCNT_dec(error_sv);
}

cfish_Err*
cfish_Err_trap(CFISH_Err_Attempt_t routine, void *context) {
    dTHX;
    cfish_Err *error = NULL;
    SV *routine_sv = newSViv(PTR2IV(routine));
    SV *context_sv = newSViv(PTR2IV(context));
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);
    EXTEND(SP, 2);
    PUSHs(sv_2mortal(routine_sv));
    PUSHs(sv_2mortal(context_sv));
    PUTBACK;

    int count = call_sv(attempt_xsub, G_EVAL | G_DISCARD);
    if (count != 0) {
        cfish_String *mess
            = cfish_Str_newf("'attempt' returned too many values: %i32",
                             (int32_t)count);
        error = cfish_Err_new(mess);
    }
    else {
        SV *dollar_at = get_sv("@", FALSE);
        if (SvTRUE(dollar_at)) {
            if (sv_isobject(dollar_at)
                && sv_derived_from(dollar_at,"Clownfish::Err")
               ) {
                IV error_iv = SvIV(SvRV(dollar_at));
                error = INT2PTR(cfish_Err*, error_iv);
                CFISH_INCREF(error);
            }
            else {
                STRLEN len;
                char *ptr = SvPVutf8(dollar_at, len);
                cfish_String *mess = cfish_Str_new_from_trusted_utf8(ptr, len);
                error = cfish_Err_new(mess);
            }
        }
    }
    FREETMPS;
    LEAVE;

    return error;
}

/********************* Clownfish::TestHarness::TestUtils ********************/


#ifndef CFISH_NOTHREADS

void*
cfish_TestUtils_clone_host_runtime() {
    PerlInterpreter *interp = (PerlInterpreter*)PERL_GET_CONTEXT;
    PerlInterpreter *clone  = perl_clone(interp, CLONEf_CLONE_HOST);
    PERL_SET_CONTEXT(interp);
    return clone;
}

void
cfish_TestUtils_set_host_runtime(void *runtime) {
    PERL_SET_CONTEXT(runtime);
}

void
cfish_TestUtils_destroy_host_runtime(void *runtime) {
    PerlInterpreter *interp = (PerlInterpreter*)runtime;
    perl_destruct(interp);
    perl_free(interp);
}

#else /* CFISH_NOTHREADS */

void*
cfish_TestUtils_clone_host_runtime() {
    CFISH_THROW(CFISH_ERR, "No thread support");
    CFISH_UNREACHABLE_RETURN(void*);
}

void
cfish_TestUtils_set_host_runtime(void *runtime) {
    CFISH_THROW(CFISH_ERR, "No thread support");
}

void
cfish_TestUtils_destroy_host_runtime(void *runtime) {
    CFISH_THROW(CFISH_ERR, "No thread support");
}

#endif

