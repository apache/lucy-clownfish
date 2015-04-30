use Hello;

package Martian;
BEGIN { our @ISA = qw( Greeter ) }

sub salutation { "We come in peace" }

package main;

my @greeters = ( Person->new, Martian->new, Dog->new );

for my $greeter (@greeters) {
    my $class_name = $greeter->get_class_name;
    my $greeting = $greeter->greet("World");
    print qq|A $class_name says "$greeting"\n|;
}

