#!/usr/local/bin/perl

use warnings;
use strict;

use JSON;
use File::Basename;
use File::Spec::Functions qw(catfile);

my $json;
my @priority;
my @wrapping;

sub config($) {
    return map { chomp; $_ } grep { !/^(#.*|\s+)$/ } do { local @ARGV = $_[0]; <> };
}

sub get_priority($) {
    my $context = shift;
    return $#priority - ( (grep { $priority[$_] =~ $context } (0..$#priority))[0] // $#priority + 1 );
}

sub assemble($@)
{
    my $context = shift;

    return "" unless scalar @_;
    my $str = join(', ', @_);

    if (!grep { $context =~ $_ } map { qr{$_} } @wrapping) {
        $str = join(",\n", @_ );
    }

    if( $str =~ tr/\n// ) {
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
        my @sorted = (sort { get_priority($b) <=> get_priority($a) } keys %{$data});
        my @fields = map { qq("$_": ) . encode($data->{$_}, "$context:$_") } @sorted;
        return '{' . assemble($context, @fields) . '}';
    }

    return $json->encode($data);
}

@priority = config(catfile(dirname(__FILE__), 'priority.conf'));
@wrapping = config(catfile(dirname(__FILE__), 'wrap.conf'));

$json = JSON->new->allow_nonref;

my $dirty;
for( my $obj; <>; ) {
    # Loop continuously until read valid JSON fragment or EOF
    eval { $obj = $json->incr_parse($_) };
    die "ERROR: Syntax error on line $.\n" if $@;

    if (defined($obj)) {
        # Unparseable non-whitespace is later caught as syntax error
        $dirty = length($json->incr_text =~ s/\s+$//r);

        # Process each object with the type field providing root context
        my @output = map { encode($_, $_->{'type'} // '' ) } @{$obj};

        # Indent everything formatted output wrap in an array
        print "[\n" . join( ",\n", @output ) =~ s/^/  /mgr . "\n]\n";
    }
}
if ($dirty) {
    die "ERROR: Syntax error at EOF\n";
}
