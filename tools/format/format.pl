#!/usr/local/bin/perl

use warnings;
use strict;

use JSON;
use File::Basename;
use File::Spec::Functions qw(catfile);
use Getopt::Std;

my $json;
my @priority;
my @wrapping;

sub config($) {
    return map { chomp; qr/$_$/ } grep { !/^(#.*|\s+)$/ } do { local @ARGV = $_[0]; <> };
}

sub get_priority($) {
    my $context = shift =~ s/(relative|proportional)://r;
    return scalar @priority - ( (grep { $context =~ $priority[$_] } (0..$#priority))[0] // scalar @priority );
}

sub get_wrapping($) {
    my $context = shift =~ s/(relative|proportional)://r;
    return grep { $context =~ $_ } map { $_ } @wrapping;
}

sub assemble($@)
{
    my $context = shift;

    return "" unless scalar @_;
    my $str = join(', ', @_);

    if (!get_wrapping($context)) {
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
        my @fields = map { encode($_, "$context:@") } @{$data};
        return '[' . assemble($context, @fields) . ']';
    }

    if (ref($data) eq 'HASH') {
        foreach( map { "$context:$_" } keys %{$data} ) {
            die "ERROR: Unknown field '$_'\n" if( get_priority($_) == 0 );
        }

        my @sorted = (sort { get_priority("$context:$b") <=> get_priority("$context:$a") } keys %{$data});
        my @fields = map { qq("$_": ) . encode($data->{$_}, "$context:$_") } @sorted;
        return '{' . assemble($context, @fields) . '}';
    }

    return $json->encode($data);
}

# -c check input is in canonical format
# -q quiet with no output to stdout
my %opts;
getopts('cq', \%opts);

@priority = config(catfile(dirname(__FILE__), 'priority.conf'));
@wrapping = config(catfile(dirname(__FILE__), 'nowrap.conf'));

$json = JSON->new->allow_nonref;

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
