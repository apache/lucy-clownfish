# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
package Clownfish::Build::Binding;
use strict;
use warnings;

our $VERSION = '0.004000';
$VERSION = eval $VERSION;

sub bind_all {
    my $class = shift;
    $class->bind_clownfish;
    $class->bind_test;
    $class->bind_test_alias_obj;
    $class->bind_blob;
    $class->bind_bytebuf;
    $class->bind_string;
    $class->bind_err;
    $class->bind_hash;
    $class->bind_float;
    $class->bind_obj;
    $class->bind_varray;
    $class->bind_class;
    $class->bind_stringhelper;
}

sub bind_clownfish {
    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish    PACKAGE = Clownfish

IV
_dummy_function()
CODE:
    RETVAL = 1;
OUTPUT:
    RETVAL

SV*
to_clownfish(sv)
    SV *sv;
CODE:
{
    cfish_Obj *obj = XSBind_perl_to_cfish_nullable(aTHX_ sv, CFISH_OBJ);
    RETVAL = CFISH_OBJ_TO_SV_NOINC(obj);
}
OUTPUT: RETVAL

SV*
to_perl(sv)
    SV *sv;
CODE:
{
    if (sv_isobject(sv) && sv_derived_from(sv, "Clownfish::Obj")) {
        IV tmp = SvIV(SvRV(sv));
        cfish_Obj* obj = INT2PTR(cfish_Obj*, tmp);
        RETVAL = XSBind_cfish_to_perl(aTHX_ obj);
    }
    else {
        RETVAL = newSVsv(sv);
    }
}
OUTPUT: RETVAL
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish",
    );
    $binding->append_xs($xs_code);

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_test {
    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish   PACKAGE = Clownfish::Test

bool
run_tests(package)
    char *package;
CODE:
    cfish_String *class_name = cfish_Str_newf("%s", package);
    cfish_TestFormatter *formatter
        = (cfish_TestFormatter*)cfish_TestFormatterTAP_new();
    cfish_TestSuite *suite = testcfish_Test_create_test_suite();
    bool result = CFISH_TestSuite_Run_Batch(suite, class_name, formatter);
    CFISH_DECREF(class_name);
    CFISH_DECREF(formatter);
    CFISH_DECREF(suite);

    RETVAL = result;
OUTPUT: RETVAL

void
invoke_to_string(sv)
    SV *sv;
PPCODE:
    cfish_Obj *obj = XSBind_perl_to_cfish(aTHX_ sv, CFISH_OBJ);
    cfish_String *str = CFISH_Obj_To_String(obj);
    CFISH_DECREF(str);
    CFISH_DECREF(obj);

int
refcount(obj)
    cfish_Obj *obj;
CODE:
    RETVAL = (int)CFISH_REFCOUNT_NN(obj);
OUTPUT: RETVAL
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "TestClownfish",
        class_name => "Clownfish::Test",
    );
    $binding->append_xs($xs_code);

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_test_alias_obj {
    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "TestClownfish",
        class_name => "Clownfish::Test::AliasTestObj",
    );
    $binding->bind_method(
        alias  => 'perl_alias',
        method => 'Aliased',
    );
    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_blob {
    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish     PACKAGE = Clownfish::Blob

SV*
new(either_sv, sv)
    SV *either_sv;
    SV *sv;
CODE:
{
    STRLEN size;
    char *ptr = SvPV(sv, size);
    cfish_Blob *self
        = (cfish_Blob*)XSBind_new_blank_obj(aTHX_ either_sv);
    cfish_Blob_init(self, ptr, size);
    RETVAL = CFISH_OBJ_TO_SV_NOINC(self);
}
OUTPUT: RETVAL
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish::Blob",
    );
    $binding->append_xs($xs_code);
    $binding->exclude_constructor;

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_bytebuf {
    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish     PACKAGE = Clownfish::ByteBuf

SV*
new(either_sv, sv)
    SV *either_sv;
    SV *sv;
CODE:
{
    STRLEN size;
    char *ptr = SvPV(sv, size);
    cfish_ByteBuf *self
        = (cfish_ByteBuf*)XSBind_new_blank_obj(aTHX_ either_sv);
    cfish_BB_init(self, size);
    CFISH_BB_Mimic_Bytes(self, ptr, size);
    RETVAL = CFISH_OBJ_TO_SV_NOINC(self);
}
OUTPUT: RETVAL
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish::ByteBuf",
    );
    $binding->append_xs($xs_code);
    $binding->exclude_constructor;

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_string {
    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish     PACKAGE = Clownfish::String

SV*
new(either_sv, sv)
    SV *either_sv;
    SV *sv;
CODE:
{
    STRLEN size;
    char *ptr = SvPVutf8(sv, size);
    cfish_String *self = (cfish_String*)XSBind_new_blank_obj(aTHX_ either_sv);
    cfish_Str_init_from_trusted_utf8(self, ptr, size);
    RETVAL = CFISH_OBJ_TO_SV_NOINC(self);
}
OUTPUT: RETVAL
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish::String",
    );
    $binding->append_xs($xs_code);
    $binding->exclude_constructor;

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_err {
    my $pod_spec = Clownfish::CFC::Binding::Perl::Pod->new;
    my $synopsis = <<'END_SYNOPSIS';
    package MyErr;
    use base qw( Clownfish::Err );
    
    ...
    
    package main;
    use Scalar::Util qw( blessed );
    while (1) {
        eval {
            do_stuff() or MyErr->throw("retry");
        };
        if ( blessed($@) and $@->isa("MyErr") ) {
            warn "Retrying...\n";
        }
        else {
            # Re-throw.
            die "do_stuff() died: $@";
        }
    }
END_SYNOPSIS
    $pod_spec->set_synopsis($synopsis);

    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish    PACKAGE = Clownfish::Err

SV*
trap(routine_sv, context_sv)
    SV *routine_sv;
    SV *context_sv;
CODE:
    cfish_Err *error = XSBind_trap(routine_sv, context_sv);
    RETVAL = CFISH_OBJ_TO_SV_NOINC(error);
OUTPUT: RETVAL
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish::Err",
    );
    $binding->bind_constructor( alias => '_new' );
    $binding->set_pod_spec($pod_spec);
    $binding->append_xs($xs_code);

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_hash {
    my @hand_rolled = qw(
        Store
    );

    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish    PACKAGE = Clownfish::Hash
SV*
_fetch(self, key)
    cfish_Hash *self;
    cfish_String *key;
CODE:
    RETVAL = CFISH_OBJ_TO_SV(CFISH_Hash_Fetch_IMP(self, key));
OUTPUT: RETVAL

void
store(self, key, value_sv);
    cfish_Hash         *self;
    cfish_String *key;
    SV           *value_sv;
PPCODE:
{
    cfish_Obj *value
        = (cfish_Obj*)XSBind_perl_to_cfish_nullable(aTHX_ value_sv, CFISH_OBJ);
    CFISH_Hash_Store_IMP(self, key, value);
}
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish::Hash",
    );
    $binding->exclude_method($_) for @hand_rolled;
    $binding->append_xs($xs_code);

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_float {
    my $float_xs_code = <<'END_XS_CODE';
MODULE = Clownfish   PACKAGE = Clownfish::Float

SV*
new(either_sv, value)
    SV     *either_sv;
    double  value;
CODE:
{
    cfish_Float *self
        = (cfish_Float*)XSBind_new_blank_obj(aTHX_ either_sv);
    cfish_Float_init(self, value);
    RETVAL = CFISH_OBJ_TO_SV_NOINC(self);
}
OUTPUT: RETVAL
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish::Float",
    );
    $binding->append_xs($float_xs_code);
    $binding->exclude_constructor;

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_obj {
    my $pod_spec = Clownfish::CFC::Binding::Perl::Pod->new;
    my $synopsis = <<'END_SYNOPSIS';
    package MyObj;
    use base qw( Clownfish::Obj );
    
    # Inside-out member var.
    my %foo;
    
    sub new {
        my ( $class, %args ) = @_;
        my $foo = delete $args{foo};
        my $self = $class->SUPER::new(%args);
        $foo{$$self} = $foo;
        return $self;
    }
    
    sub get_foo {
        my $self = shift;
        return $foo{$$self};
    }
    
    sub DESTROY {
        my $self = shift;
        delete $foo{$$self};
        $self->SUPER::DESTROY;
    }
END_SYNOPSIS
    my $description = <<'END_DESCRIPTION';
Clownfish::Obj is the base class of the Clownfish object hierarchy.

From the standpoint of a Perl programmer, all classes are implemented as
blessed scalar references, with the scalar storing a pointer to a C struct.

=head2 Subclassing

The recommended way to subclass Clownfish::Obj and its descendants is
to use the inside-out design pattern.  (See L<Class::InsideOut> for an
introduction to inside-out techniques.)

Since the blessed scalar stores a C pointer value which is unique per-object,
C<$$self> can be used as an inside-out ID.

    # Accessor for 'foo' member variable.
    sub get_foo {
        my $self = shift;
        return $foo{$$self};
    }


Caveats:

=over

=item *

Inside-out aficionados will have noted that the "cached scalar id" stratagem
recommended above isn't compatible with ithreads.

=item *

Overridden methods must not return undef unless the API specifies that
returning undef is permissible.  (Failure to adhere to this rule currently
results in a segfault rather than an exception.)

=back

=head1 CONSTRUCTOR

=head2 new()

Abstract constructor -- must be invoked via a subclass.  Attempting to
instantiate objects of class "Clownfish::Obj" directly causes an
error.

Takes no arguments; if any are supplied, an error will be reported.
END_DESCRIPTION
    my $destroy_pod = <<'END_POD';
=head2 DESTROY()

All Clownfish classes implement a DESTROY method; if you override it in a
subclass, you must call C<< $self->SUPER::DESTROY >> to avoid leaking memory.
END_POD
    $pod_spec->set_synopsis($synopsis);
    $pod_spec->set_description($description);
    $pod_spec->add_method(
        method => 'Clone',
        alias  => 'clone',
    );
    $pod_spec->add_method(
        method => 'Destroy',
        alias  => 'DESTROY',
        pod    => $destroy_pod,
    );

    my @hand_rolled = qw( Clone );
    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish     PACKAGE = Clownfish::Obj

SV*
get_class(self)
    cfish_Obj *self
CODE:
    cfish_Class *klass = cfish_Obj_get_class(self);
    RETVAL = (SV*)CFISH_Class_To_Host(klass);
OUTPUT: RETVAL

SV*
get_class_name(self)
    cfish_Obj *self
CODE:
    cfish_String *class_name = cfish_Obj_get_class_name(self);
    RETVAL = (SV*)CFISH_Str_To_Host(class_name);
OUTPUT: RETVAL

bool
is_a(self, class_name)
    cfish_Obj *self;
    cfish_String *class_name;
CODE:
{
    cfish_Class *target = cfish_Class_fetch_class(class_name);
    RETVAL = cfish_Obj_is_a(self, target);
}
OUTPUT: RETVAL

SV*
clone(self)
    cfish_Obj *self;
CODE:
    RETVAL = CFISH_OBJ_TO_SV_NOINC(CFISH_Obj_Clone(self));
OUTPUT: RETVAL
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish::Obj",
    );
    $binding->bind_method(
        alias  => 'DESTROY',
        method => 'Destroy',
    );
    $binding->exclude_method($_) for @hand_rolled;
    $binding->append_xs($xs_code);
    $binding->set_pod_spec($pod_spec);

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_varray {
    my @hand_rolled = qw(
        Pop
        Delete
        Store
        Fetch
    );

    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish   PACKAGE = Clownfish::Vector

SV*
pop(self)
    cfish_Vector *self;
CODE:
    RETVAL = CFISH_OBJ_TO_SV_NOINC(CFISH_Vec_Pop(self));
OUTPUT: RETVAL

SV*
delete(self, tick)
    cfish_Vector *self;
    uint32_t    tick;
CODE:
    RETVAL = CFISH_OBJ_TO_SV_NOINC(CFISH_Vec_Delete(self, tick));
OUTPUT: RETVAL

void
store(self, tick, value);
    cfish_Vector *self;
    uint32_t     tick;
    cfish_Obj    *value;
PPCODE:
{
    if (value) { CFISH_INCREF(value); }
    CFISH_Vec_Store_IMP(self, tick, value);
}

SV*
fetch(self, tick)
    cfish_Vector *self;
    uint32_t     tick;
CODE:
    RETVAL = CFISH_OBJ_TO_SV(CFISH_Vec_Fetch(self, tick));
OUTPUT: RETVAL
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish::Vector",
    );
    $binding->exclude_method($_) for @hand_rolled;
    $binding->append_xs($xs_code);

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_class {
    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish   PACKAGE = Clownfish::Class

SV*
fetch_class(unused_sv, class_name_sv)
    SV *unused_sv;
    SV *class_name_sv;
CODE:
{
    STRLEN size;
    char *ptr = SvPVutf8(class_name_sv, size);
    cfish_String *class_name = CFISH_SSTR_WRAP_UTF8(ptr, size);
    cfish_Class *klass = cfish_Class_fetch_class(class_name);
    CFISH_UNUSED_VAR(unused_sv);
    RETVAL = klass ? (SV*)CFISH_Class_To_Host(klass) : &PL_sv_undef;
}
OUTPUT: RETVAL

SV*
singleton(unused_sv, ...)
    SV *unused_sv;
CODE:
{
    cfish_String *class_name = NULL;
    cfish_Class  *parent     = NULL;
    cfish_Class  *singleton  = NULL;
    bool args_ok
        = XSBind_allot_params(aTHX_ &(ST(0)), 1, items,
                              ALLOT_OBJ(&class_name, "class_name", 10, true,
                                        CFISH_STRING, CFISH_ALLOCA_OBJ(CFISH_STRING)),
                              ALLOT_OBJ(&parent, "parent", 6, false,
                                        CFISH_CLASS, NULL),
                              NULL);
    CFISH_UNUSED_VAR(unused_sv);
    if (!args_ok) {
        CFISH_RETHROW(CFISH_INCREF(cfish_Err_get_error()));
    }
    singleton = cfish_Class_singleton(class_name, parent);
    RETVAL = (SV*)CFISH_Class_To_Host(singleton);
}
OUTPUT: RETVAL
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish::Class",
    );
    $binding->append_xs($xs_code);

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

sub bind_stringhelper {
    my $xs_code = <<'END_XS_CODE';
MODULE = Clownfish   PACKAGE = Clownfish::Util::StringHelper

=for comment

Turn an SV's UTF8 flag on.  Equivalent to Encode::_utf8_on, but we don't have
to load Encode.

=cut

void
utf8_flag_on(sv)
    SV *sv;
PPCODE:
    SvUTF8_on(sv);

=for comment

Turn an SV's UTF8 flag off.

=cut

void
utf8_flag_off(sv)
    SV *sv;
PPCODE:
    SvUTF8_off(sv);

SV*
to_base36(num)
    uint64_t num;
CODE:
{
    char base36[cfish_StrHelp_MAX_BASE36_BYTES];
    size_t size = cfish_StrHelp_to_base36(num, &base36);
    RETVAL = newSVpvn(base36, size);
}
OUTPUT: RETVAL

IV
from_base36(str)
    char *str;
CODE:
    RETVAL = strtol(str, NULL, 36);
OUTPUT: RETVAL

=for comment

Upgrade a SV to UTF8, converting Latin1 if necessary. Equivalent to
utf::upgrade().

=cut

void
utf8ify(sv)
    SV *sv;
PPCODE:
    sv_utf8_upgrade(sv);

bool
utf8_valid(sv)
    SV *sv;
CODE:
{
    STRLEN len;
    char *ptr = SvPV(sv, len);
    RETVAL = cfish_StrHelp_utf8_valid(ptr, len);
}
OUTPUT: RETVAL

=for comment

Concatenate one scalar onto the end of the other, ignoring UTF-8 status of the
second scalar.  This is necessary because $not_utf8 . $utf8 results in a
scalar which has been infected by the UTF-8 flag of the second argument.

=cut

void
cat_bytes(sv, catted)
    SV *sv;
    SV *catted;
PPCODE:
{
    STRLEN len;
    char *ptr = SvPV(catted, len);
    if (SvUTF8(sv)) { CFISH_THROW(CFISH_ERR, "Can't cat_bytes onto a UTF-8 SV"); }
    sv_catpvn(sv, ptr, len);
}
END_XS_CODE

    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => "Clownfish",
        class_name => "Clownfish::Util::StringHelper",
    );
    $binding->append_xs($xs_code);

    Clownfish::CFC::Binding::Perl::Class->register($binding);
}

1;
