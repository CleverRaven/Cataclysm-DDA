#!/usr/bin/env python3
import os
import regex
from spellchecker import SpellChecker


Speller = SpellChecker()
Tokenizer = regex.compile(
    r"(?<="
        r"(?:" # a word can be after...
            r"^" # ...the start of text...
        r"|" # ...or...
            r"""[\s\-'‘"“«\*&\(\[\{/>]""" # ...a valid starting delimiter
        r")"
        r"[…]*" # ignore possible punctuations before a word
    r")"
    # a single letter is always considered a valid word and therefore not
    # tokenized
    r"\pL" # a word always starts with an letter...
    r"(?:\pL|['‘’])*" # ...can contain letters and apostrophes...
    r"\pL" # ...and always ends with an letter
    # exclude suffix `'s` and match the apostrophe as a delimiter later
    r"(?<!'[sS])"
    # note: staring and ending apostrophe cannot be distinguished from
    # starting and ending single quote so the former is not considered.
    r"(?="
        r"[,\.\?!:;…]*" # ignore any punctuations after a word
        r"(?:" # a word can be before...
            r"$" # ...the end of text...
        r"|" # ...or...
            r"""[\s\-'’"”»\*\)\]\}/<]""" # ...a valid ending delimiter
        r")"
    r")"
)
KnownWords = set()


def init_known_words():
    dictionary = os.path.join(os.path.dirname(__file__), 'dictionary.txt')
    with open(dictionary, 'r', encoding='utf-8') as fp:
        for line in fp:
            line = line.rstrip('\n').rstrip('\r')
            KnownWords.add(line.split(' ')[0])


if not KnownWords:
    init_known_words()
