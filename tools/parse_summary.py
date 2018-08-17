#!/usr/bin/env python

import sys
import json
import re

headers = [
        "Features",
        "Content",
        "Interface",
        "Mods",
        "Balance",
        "Bugfixes",
        "Performance",
        "Infrastructure",
        "Build",
        "I18N",
]

pr = json.load(sys.stdin)
prnum = pr['number']
res = re.search('SUMMARY: (' + '|'.join(headers) + ') "(.*)"', pr['body'])
if res:
    cat = res.group(1)
    msg = res.group(2)
else:
    cat = "Uncatalogued"
    msg = pr['title']
print('{} {}: "{}"'.format(cat, prnum, msg))
