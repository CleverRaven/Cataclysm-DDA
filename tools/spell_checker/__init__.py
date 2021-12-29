#!/usr/bin/env python3
import os
import re
from spellchecker import SpellChecker


Speller = SpellChecker()
Tokenizer = re.compile(r'\w+')
KnownWords = set()


def init_known_words():
    dictionary = os.path.join(os.path.dirname(__file__), 'dictionary.txt')
    with open(dictionary) as fp:
        for line in fp:
            line = line.rstrip('\n').rstrip('\r')
            KnownWords.add(line.split(' ')[0])


if not KnownWords:
    init_known_words()
