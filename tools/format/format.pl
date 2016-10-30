#!/usr/bin/env perl

use warnings;
use strict;
use 5.010;

use JSON;
use File::Basename;
use File::Spec::Functions qw(catfile);
use Getopt::Std;

# -c check input is in canonical format
# -q quiet with no output to stdout
# -v verbose error messages (includes hints at canonical format)
my %opts;
getopts('cqv', \%opts);

my @config;
for( open my $fh, '<', catfile(dirname(__FILE__), 'format.conf'); <$fh>; ) {
    chomp;
    do {} while( s/#.*|^\s+|\s+$//g );
    next unless length;
    my ($rule, $flags) = ( split '=' );
    push @config, [ qr/$rule$/, [ split(',', ($flags // '')) ] ];
}

my $json = JSON->new->allow_nonref;
my $fail = sub { die "ERROR: $_[0]\n"; };

sub match($$) {
    my ($context, $query) = @_;
    $context =~ s/(relative|proportional|extend|delete)(<.*?>)?://g;
    return $context =~ $query || ($context =~ s/<.*?>//gr ) =~ $query;
}

sub find_rule($) {
    state %cache;
    return $cache{$_[0]} if exists $cache{$_[0]};
    for my $i (0 .. $#config) {
        return $cache{$_[0]} = $i if match($_[0],$config[$i][0]);
    }
    return $cache{$_[0]} = -1; # no rule matching this context
}

sub has_flag($$) {
    my ($rule, $flag) = (find_rule($_[0]), $_[1]);
    return 0 if $rule < 0;
    return grep { $_ eq $flag } @{$config[$rule][1] // []};
}

sub assemble($@)
{
    my $context = shift;

    return "" unless scalar @_;
    my $str = join(', ', @_);

    if (!has_flag($context, 'NOWRAP')) {
        $str = join(",\n", @_);
    }

    if ($str =~ tr/\n// or has_flag($context, 'WRAP')) {
        $str =~ s/^/  /mg;
        return "\n$str\n";
    } else {
        return " $str ";
    }
}

sub encode(@); # Recursive function needs forward definition

sub encode(@) {
    my ($data, $context) = @_;

    $fail->("Unmatched context '$context'") if ref($data) and find_rule($context) < 0;

    if (ref($data) eq 'ARRAY') {
        my @elems = map { encode($_, "$context:@") } @{$data};
        return '[' . assemble($context, @elems) . ']';
    }

    return encode([$data],$context) if has_flag($context,'ARRAY');

    if (ref($data) eq 'HASH') {
        # Built the context for each member field and determine its sort rank
        $context .= '<' . ($data->{'type'} // '') . '>';
        my %fields = map {
            my $rule = "$context:$_";
            my $rank = find_rule($rule);
            $fail->("Unmatched contex '$rule'") if $rank < 0;
            $_ => [ $rule, $rank ];
        } keys %{$data};

        # Sort the member fields then recursively encode their data
        # Where two fields match the same context sort them alphabetically to determine order
        my @sorted = (sort { $fields{$a}->[1] <=> $fields{$b}->[1] or $a cmp $b } keys %fields);
        my @elems = map { qq("$_": ) . encode($data->{$_}, $fields{$_}->[0]) } @sorted;
        return '{' . assemble($context, @elems) . '}';
    }

    return $json->encode($data);
}

my ($original, $dirty);
my @parsed;

my $src;
if (exists $ENV{'GATEWAY_INTERFACE'}) {
    require CGI;
    CGI->import(':cgi');
    open($src, '<', \(param('data') // ''));

    $fail = sub {
        print header('text/plain', '400 Bad request');
        print "$_[0]\n";
        exit 0;
    };

} elsif (scalar @ARGV) {
    open($src, '<', $ARGV[0]);

} else {
    $src = \*STDIN;
}

while(<$src>) {
    $original .= $_;
    $dirty .= $_;
    eval {
        $json->incr_parse($_);
        for (my $obj; $obj = $json->incr_parse;) {
            $dirty = $json->incr_text;

            # Also supports single objects as users may paste them in to the web linter
            push @parsed, ref($obj) eq 'ARRAY' ? @{$obj} : $obj;
        }
    };
    $fail->("Syntax error on line $.") if $@;
}

# If we have unparsed content fail unless is insignificant whitespace
$fail->("Syntax error at EOF") if $dirty =~ /[^\s]/;

my @output;
foreach (@parsed) {
    # Process each object with the type field providing root context
    $fail->("Invalid JSON structure") unless ref($_) eq 'HASH' and exists $_->{type};
    push @output, encode($_, $_->{'type'} // '' );
}

# Indent each entry and output wrapped as a JSON array
my $result = scalar @output ? "[\n" . join( ",\n", @output ) =~ s/^/  /mgr . "\n]\n" : '';

if (exists $ENV{'GATEWAY_INTERFACE'}) {
    if (($original // '') eq $result) {
        print header('application/json', '304 Not Modified');
    } else {
        print header('application/json', '200 OK');
        print $result;
    }
    exit 0;
}

print $result unless $opts{'q'};
exit 0 unless $opts{'c'};

# If checking for canonical formatting get offset of first mismatch (if any)
exit 0 if ($original // '') eq $result;

($original ^ $result) =~ /^\0*/;
my $line = scalar split '\n', substr($result,0,$+[0]);
print STDERR "ERROR: Format error at line $line\n";
print STDERR "< " . (split '\n', $original)[$line-1] . "\n";
print STDERR "> " . (split '\n', $result  )[$line-1] . "\n";

if ($opts{'v'}) {
    print STDERR "\nHINT: Canonical output for this block was:\n";
    my @diff = split '\n', $result;
    while ($line-- > 0 and $diff[$line] ne '  {') {}

    for (my $i = $line + 1; $i < @diff; $i++) {
        last if $diff[$i] =~ /^  },?$/;
        print STDERR $diff[$i] . "\n";
    }
}
exit 1;
