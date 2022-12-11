#!/usr/bin/env perl

use warnings;
use strict;
use 5.014;

use JSON;
use File::Basename;
use File::Spec::Functions qw(catfile);

my $config;
for( open my $fh, '<', catfile(dirname(__FILE__), 'sections.conf'); <$fh>; ) {
    chomp;
    do {} while( s/#.*|^\s+|\s+$//g );
    next unless length;
    my ($section, $context) = ( split '=' );
    foreach( split ',', $context ) {
        $config->{$_} = $section;
    }
}

my $json = JSON->new->allow_nonref;

sub output($$) {
    my $flags = (shift)->{(shift)};
    foreach (sort keys %{$flags}) {
        print "- `$_` $flags->{$_}\n";
    }
}

my $parsed;
foreach (@{($json->decode(do { local $/; <> }))}) {
    next if $_->{pseudo};

    my $id = $_->{id};
    my $msg = $_->{description} // ($_->{info} =~ s/<.+?>//gr);

    foreach (@{$_->{context}}) {
        my $section = $config->{$_};
        $parsed->{$section} = {} unless exists $parsed->{$section};
        $parsed->{$section}->{$id} = $msg;
    }
}

for( open my $fh, '<', catfile(dirname(__FILE__), 'template.in'); <$fh>; ) {
    print s/@([^@]+)@/output($parsed,$1)/egr;
}
