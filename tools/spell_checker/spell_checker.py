#!/usr/bin/env python3
import re
from . import Speller, Tokenizer, KnownWords


def is_english(word):
    for w in word:
        if not ('a' <= w and w <= 'z'):
            return False
    return True


def not_in_known_words(word):
    return word not in KnownWords


def sanitize_message(message):
    untagged = re.sub(r'<[0-9a-z_/]+>', '', message)
    unformatted = re.sub(r'%[0-9lz\.\s$]*[scfd]', '', untagged)
    return unformatted


def spell_check(message):
    words = filter(is_english, Tokenizer.findall(sanitize_message(message)))
    unknowns = filter(not_in_known_words, Speller.unknown(words))
    return list(unknowns)
