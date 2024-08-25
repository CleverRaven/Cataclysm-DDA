#!/usr/bin/env python3
from . import DefaultKnownWords, Tokenizer
import os
import polib
import unicodedata


def gen_dictionary():
    po_path = os.path.join(os.path.dirname(__file__),
                           "../../lang/po/cataclysm-dda.pot")
    pofile = polib.pofile(po_path)
    dictionary = set()
    for entry in pofile:
        words = Tokenizer.findall(entry.msgid)
        for word in words:
            norm = unicodedata.normalize('NFC', word)
            if norm not in DefaultKnownWords:
                dictionary.add(norm)
        if entry.msgid_plural:
            words = Tokenizer.findall(entry.msgid_plural)
            for word in words:
                norm = unicodedata.normalize('NFC', word)
                if norm not in DefaultKnownWords:
                    dictionary.add(norm)
    dict_path = os.path.join(os.path.dirname(__file__), "dictionary.txt")
    with open(dict_path, "w", encoding="utf-8") as fp:
        for word in sorted(dictionary, key=lambda v: (v.lower(), v)):
            print(word, file=fp)
