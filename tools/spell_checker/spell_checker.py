#!/usr/bin/env python3
import re
from . import Tokenizer, KnownWords


def not_in_known_words(word):
    return word not in KnownWords


def spell_check(message):
    words = Tokenizer.findall(message)
    unknowns = filter(not_in_known_words, words)
    return list(unknowns)
