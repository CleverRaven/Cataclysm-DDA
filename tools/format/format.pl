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
    my $context = shift =~ s/(relative|proportional|extend|delete)://r;
    return scalar @priority - ( (grep { $context =~ $priority[$_] } (0..$#priority))[0] // scalar @priority );
}

sub get_wrapping($) {
    my $context = shift =~ s/(relative|proportional|extend|delete)://r;
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
        my @elems = map { encode($_, "$context:@") } @{$data};
        return '[' . assemble($context, @elems) . ']';
    }

    if (ref($data) eq 'HASH') {
        my %fields;
        foreach (keys %{$data}) {
            my $rule = $context . '<'.($data->{'type'} // '').'>' . ":$_";
            my $rank = get_priority($rule);
            $fields{$_} = [ $rule, $rank ];
            die "ERROR: Unknown field '$rule'\n" if $rank == 0;
        }

        # Sort the member fields then recursively encode their data
        my @sorted = (sort { $fields{$b}->[1] <=> $fields{$a}->[1] } keys %fields);
        my @elems = map { qq("$_": ) . encode($data->{$_}, $fields{$_}->[0]) } @sorted;
        return '{' . assemble($context, @elems) . '}';
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
