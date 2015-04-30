use 5.008003;
use strict;
use warnings;
use Clownfish::CFC::Perl::Build;
use File::Spec::Functions qw( catdir );

my @cf_base_path    = Clownfish::CFC::Perl::Build->cf_base_path;
my @cf_linker_flags = Clownfish::CFC::Perl::Build->cf_linker_flags(
    qw( Clownfish )
);

my $builder = Clownfish::CFC::Perl::Build->new(
    module_name        => 'Hello',
    dist_abstract      => 'Hello, world!',
    dist_author        => 'Apache Lucy Project',
    dist_version       => '0.1.0',
    extra_linker_flags => [ @cf_linker_flags ],
    configure_requires => {
        'Clownfish::CFC::Perl::Build' => 0,
    },
    requires => {
        'perl'      => '5.8.3',
        'Clownfish' => '0.4.0',
    },
    clownfish_params => {
        source  => [ catdir( @cf_base_path, 'core' ) ],
    },
);

$builder->create_build_script();

