#!/usr/bin/env python3
from . import Tokenizer, KnownWords
import unicodedata


def not_in_known_words(word):
    return unicodedata.normalize('NFC', word) not in KnownWords


def spell_check(message):
    words = Tokenizer.findall(message)
    unknowns = filter(not_in_known_words, words)
    return list(unknowns)
