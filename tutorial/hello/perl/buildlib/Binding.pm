package Binding;
use strict;
use warnings;

our $VERSION = '0.01';
$VERSION = eval $VERSION;

sub bind_all {
    my $class = shift;
    my $binding = Clownfish::CFC::Binding::Perl::Class->new(
        parcel     => 'hello',
        class_name => 'Hello',
    );
    Clownfish::CFC::Binding::Perl::Class->register($binding);
}


