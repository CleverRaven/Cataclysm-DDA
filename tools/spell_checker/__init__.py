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


def unit_test__tokenizer():
    text_and_results = [
        ("I'm a test", ["I'm", "test"]),
        ("\"This'll do!\"", ["This'll", "do"]),
        ("Someone's test case", ["Someone", "test", "case"]),
        ("SOMEONE'S VERY IMPORTANT TEST", ["SOMEONE", "VERY", "IMPORTANT", "TEST"]),
        ("tic-tac-toe", ["tic", "tac", "toe"]),
        ("Ünicodé", ["Ünicodé"]),
        ("This apostrophe’s weird", ["This", "apostrophe’s", "weird"]),
        ("Foo i18n l10n bar", ["Foo", "bar"]),
        ("Lorem 123 ipsum -- dolor ||| sit", ["Lorem", "ipsum", "dolor", "sit"]),
        ("Lorem", ["Lorem"]),
        ("Lorem ipsum dolor sit amet",
            ["Lorem", "ipsum", "dolor", "sit", "amet"]),
        ("'Lorem ipsum dolor sit amet, consectetur adipiscing elit'",
            ["Lorem", "ipsum", "dolor", "sit", "amet", "consectetur",
                "adipiscing","elit"]),
        ("Lorem, ipsum.  dolor?  sit!  amet.", ["Lorem", "ipsum", "dolor", "sit", "amet"]),
        ("Lorem ipsum, 'dolor?'  sit!?", ["Lorem", "ipsum", "dolor", "sit"]),
        ("Lorem: ipsum; dolor…", ["Lorem", "ipsum", "dolor"]),
        ('"Lorem"', ["Lorem"]),
        ("Lorem-ipsum", ["Lorem", "ipsum"]),
        ("«Lorem ipsum»", ["Lorem", "ipsum"]), # used in some NPC dialogue
        ("*Lorem ipsum*", ["Lorem", "ipsum"]),
        ("&Lorem ipsum", ["Lorem", "ipsum"]), # used in some NPC dialogue
        ("…Lorem", ["Lorem"]),
        ("'…Lorem ipsum…'", ["Lorem", "ipsum"]),
        ("Lorem <ipsum> <color_yellow>dolor</color>", ["Lorem", "dolor"]),
        ("<global_val:<u_val:foobar>>", []),
        ("Lorem (ipsum) [dolor sit] {amet}", ["Lorem", "ipsum", "dolor", "sit", "amet"]),
        ("Lorem ipsum/dolor/sit amet", ["Lorem", "ipsum", "dolor", "sit", "amet"]),
        ("Lorem.ipsum.dolor", []),
        ("Lorem %1$s ipsum %2$d dolor!", ["Lorem", "ipsum", "dolor"])
    ]

    fail = False
    for text, expected in text_and_results:
        result = list(Tokenizer.findall(text))
        if result != expected:
            print(f"test case: {text}\nexpected: {expected}\nresult: {result}\n")
            fail = True

    if fail:
        raise RuntimeError("tokenizer test failed")
