#!/usr/bin/env python3
from . import Speller, Tokenizer
from .spell_checker import is_english, sanitize_message
import os
import polib


def gen_dictionary():
    po_path = os.path.join(os.path.dirname(__file__),
                           "../../lang/po/cataclysm-dda.pot")
    pofile = polib.pofile(po_path)
    dictionary = set()
    for entry in pofile:
        words = filter(is_english,
                       Tokenizer.findall(sanitize_message(entry.msgid)))
        for word in Speller.unknown(words):
            dictionary.add(word)
    dict_path = os.path.join(os.path.dirname(__file__), "dictionary.txt")
    with open(dict_path, "w") as fp:
        for word in sorted(dictionary):
            print(word, file=fp)
