#!/usr/bin/env python3
from spell_checker.spell_checker import spell_check
import polib


pofile = polib.pofile('./lang/po/cataclysm-dda.pot')
# occurrences = dict()
for entry in pofile:
    typos = spell_check(entry.msgid)
    # for word in typos:
    #     if word in occurrences:
    #         occurrences[word] += 1
    #     else:
    #         occurrences[word] = 1
    if typos:
        print(typos, "<=", entry.msgid.replace('\n', '\\n'))
    if entry.msgid_plural:
        typos = spell_check(entry.msgid_plural)
        if typos:
            print(typos, "<=", entry.msgid_plural.replace('\n', '\\n'))
