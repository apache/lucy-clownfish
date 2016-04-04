#!/usr/bin/perl

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

=head1 NAME

html2mdtext.pl - Convert HTML to mdtext for the Apache CMS

=head1 SYNOPSIS

    html2mdtext.pl

=head1 DESCRIPTION

This script creates mdtext files from HTML. It must be run in the C<c>
directory and scans all .html files found in C<autogen/share/doc/clownfish>.
The resulting mdtext files are stored in a directory named C<mdtext>.

=cut

use strict;
use warnings;
use utf8;

use File::Find;
use File::Path qw( make_path );
use File::Slurp;

my $src_dir  = 'autogen/share/doc/clownfish';
my $out_root = 'mdtext';

find( { wanted => \&process_file, no_chdir => 1 }, $src_dir );

sub process_file {
    my $filename = $_;
    my $dir      = $File::Find::topdir;

    return if -d $filename || $filename !~ /\.html\z/;
    $filename =~ s|^$dir/||;

    html2mdtext( $dir, $filename );
};

sub html2mdtext {
    my ( $base_dir, $filename ) = @_;

    my $content = read_file( "$base_dir/$filename", binmode => ':utf8' );

    if ($content !~ m|<title>([^<]+)</title>|) {
        warn("No title found in $filename");
        return;
    }
    my $title = $1;

    if ($content !~ m|<body>\s*(.+?)\s*</body>|s) {
        warn("No body found in $filename");
        return;
    }
    $content = $1;

    # Increase header level.
    $content =~ s{(</?h)(\d)>}{ $1 . ($2 + 1) . '>' }ge;

    $content = <<"EOF";
Title: $title

<div class="c-api">
$content
</div>
EOF

    my @path_comps = split('/', $filename);
    pop(@path_comps);
    my $out_dir = join('/', $out_root, @path_comps);
    make_path($out_dir);

    my $out_filename = "$out_root/$filename";
    $out_filename =~ s|(\.[^/.]*)?\z|.mdtext|;
    write_file( $out_filename, { binmode => ':utf8' }, \$content );
}

