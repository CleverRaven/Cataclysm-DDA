#!/usr/bin/env perl

use warnings;
use strict;

use JSON;
use File::Basename;
use File::Spec::Functions qw(catfile);
use Getopt::Std;

# -c check input is in canonical format
# -q quiet with no output to stdout
my %opts;
getopts('cq', \%opts);

my @config;
for( open my $fh, '<', catfile(dirname(__FILE__), 'format.conf'); <$fh>; ) {
    chomp;
    do {} while( s/#.*|^\s+|\s+$//g );
    next unless length;
    my ($rule, $flags) = ( split '=' );
    push @config, [ qr/$rule$/, split(',', ($flags // '')) ];
}

my $json = JSON->new->allow_nonref;

sub match($$) {
    my ($context, $query) = @_;
    $context =~ s/(relative|proportional|extend|delete)(<.*?>)?://g;
    return $context =~ $query || ($context =~ s/<.*?>//gr ) =~ $query;
}

sub find_rule($) {
    for my $i (0 .. $#config) {
        return $i if match($_[0],$config[$i][0]);
    }
    die "ERROR: Unmatched rule '". $_[0] ."'\n";
}

sub has_flag($$) {
    return grep $_[1], $config[find_rule($_[0])][1] // ();
}

sub assemble($@)
{
    my $context = shift;

    return "" unless scalar @_;
    my $str = join(', ', @_);

    if (!has_flag($context, 'NOWRAP')) {
        $str = join(",\n", @_);
    }

    if ($str =~ tr/\n//) {
        $str =~ s/^/  /mg;
        return "\n$str\n";
    } else {
        return " $str ";
    }
}

sub encode(@); # Recursive function needs forward definition

sub encode(@) {
    my ($data, $context) = @_;

    if (ref($data) eq 'ARRAY') {
        my @elems = map { encode($_, "$context:@") } @{$data};
        return '[' . assemble($context, @elems) . ']';
    }

    if (ref($data) eq 'HASH') {
        # Built the context for each member field and determine its sort rank
        my %fields = map {
            my $rule = $context . '<'.($data->{'type'} // '').'>' . ":$_";
            $_ => [ $rule, find_rule($rule) ];
        } keys %{$data};

        # Sort the member fields then recursively encode their data
        my @sorted = (sort { $fields{$a}->[1] <=> $fields{$b}->[1] } keys %fields);
        my @elems = map { qq("$_": ) . encode($data->{$_}, $fields{$_}->[0]) } @sorted;
        return '{' . assemble($context, @elems) . '}';
    }

    return $json->encode($data);
}

my ($original, $result, $dirty);

for( my $obj; <>; ) {
    # Loop continuously until read valid JSON fragment or EOF
    $original .= $_;
    eval { $obj = $json->incr_parse($_) };
    die "ERROR: Syntax error on line $.\n" if $@;

    if (defined($obj)) {
        # Unparseable non-whitespace is later caught as syntax error
        $dirty = length($json->incr_text =~ s/\s+$//r);

        # Process each object with the type field providing root context
        my @output = map { encode($_, $_->{'type'} // '' ) } @{$obj};

        # Indent everything formatted output wrap in an array
        $result = "[\n" . join( ",\n", @output ) =~ s/^/  /mgr . "\n]\n";
    }
}
if ($dirty) {
    die "ERROR: Syntax error at EOF\n";
}

print $result unless $opts{'q'};
exit($opts{'c'} ? ($original eq $result ? 0 : 1) : 0)
