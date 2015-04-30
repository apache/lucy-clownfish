package Hello;

use Clownfish;

BEGIN {
    our $VERSION = '0.1.0';
    $VERSION = eval $VERSION;

    require DynaLoader;
    our @ISA = qw(DynaLoader);
    bootstrap Hello;
}

1;

