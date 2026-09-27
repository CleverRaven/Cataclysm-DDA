#!/usr/bin/env python3

# This script is intended to help monitor macOS market share to help us decide
# when it is reasonable to drop support for a particular version.
#
# The expected input file is a CSV-formatted data file downloaded from
# https://gs.statcounter.com/os-version-market-share/macos/desktop/worldwide
#
# The output shows cumulative market share by increasing version number for
# each month in the input data.  The range of versions is limited to make the
# output more reasonable, but the cumulative tally does include smaller
# versions not listed.
#
# See also doc/c++/COMPILER_SUPPORT.md

import csv
import re
import sys
from collections import defaultdict

in_filename, = sys.argv[1:]

with open(in_filename) as in_file:
    reader = csv.reader(in_file)
    rows = list(reader)

headers = rows[0]
rows = rows[1:]

replacement = {
    'Date': 'Date',
    'macOS Tahoe': (27, 7),
    'macOS Sequoia': (15, 8),
    'macOS Sequoia.0': (15, 8),
    'macOS Sonoma': (14, 8),
    'macOS Ventura': (13, 7),
    'macOS Monterey': (12, 7),
    'macOS 11': (11, 7),
    # webkit#216593: Apple are incorrectly reporting all macOS releases
    # since Catalina 10.15 as Catalina 10.15
    'macOS 19': (10, 15),
    'macOS Catalina': (10, 15),
    'macOS Mojave': (10, 14),
    'macOS High Sierra': (10, 13),
    'macOS Sierra': (10, 12),
    'macOS El Capitan': (10, 11),
    'OS X El Capitan': (10, 11),
    'OS X Mavericks': (10, 9),
    'mac OS X Snow Leopard': (10, 6),
    'mac OS X Lion': (10, 7),
    'OS X Mountain Lion': (10, 8),
    'mac OS X Leopard': (10, 6),
    'mac OS X Tiger': (10, 4),
    'mac OS X Jaguar': (10, 2),
    'mac OS X Panther': (10, 3),
    'macOS Cheetah': (10, 0),
    'Other': (0,)
}

version_re = re.compile('([0-9]+)\\.([0-9]+)')


def get_version(name):
    if name in replacement:
        return replacement[name]
    match = version_re.search(name)
    assert match, f'name = {name!r}'
    return tuple(int(x) for x in match.group(1, 2))


headers = [get_version(h) for h in headers]

u_headers = list(dict.fromkeys(headers))
dicts = []
for row in rows:
    summed = defaultdict(float)
    for h, v in zip(headers, row):
        if h == 'Date':
            summed[h] = v
        else:
            summed[h] += float(v)
    row = [summed[h] for h in u_headers]
    dicts.append({field_name: field for field_name,
                  field in zip(u_headers, row)})

headers = u_headers

for dic in dicts:
    date = dic.pop('Date')
    stats = sorted(dic.items())
    tally = []
    total = 0
    for version, fraction in stats:
        total += float(fraction)
        if version >= (10, 11) and version <= (10, 15):
            tally.append((version, total))
    tally_str = ''
    for version, total in tally:
        tally_str += f'{version[0]}.{version[1]}: {total:4.1f}  '
    print(f'{date} :: {tally_str}')
