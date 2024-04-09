#!/usr/bin/env python3
from spell_checker.spell_checker import spell_check
import re
import json
from optparse import OptionParser


parser = OptionParser()
parser.add_option("-i", "--input", dest="pot_diff",
                  help="Path to PO diff result in JSON format")

(options, args) = parser.parse_args()
if not options.pot_diff:
    parser.error("Input file not given")


diff = json.load(open(options.pot_diff, "r"))
errors = []
for message in diff["added"]:
    typos = set(spell_check(message))
    if len(typos) == 0:
        continue
    message = message.replace('\n', "\\n")
    for typo in typos:
        bold = "**" + typo + "**"
        message = re.sub(re.escape(typo), lambda _: bold, message)
    errors.append(message)
if errors:
    print("Spell checker encountered unrecognized words in the in-game text"
          " added in this pull request. See below for details.")
    print("<details>")
    print("<summary>Click to expand</summary>")
    print()
    for err in errors:
        print("*", err)
    print("</details>")
    print()
    print("This alert is automatically generated. You can simply disregard if"
          " this is inaccurate, or (optionally) you can also add the new words"
          " to `tools/spell_checker/dictionary.txt` so they will not trigger "
          "an alert next time.")
    print()
    print("<details>")
    print("<summary>Hints for adding a new word to the dictionary</summary>")
    print()
    print("* If the word is normally in all lowercase, such as the noun "
          "`word` or the verb `does`, add it in its lower-case form; if the "
          "word is a proper noun, such as the surname `George`, add it in its "
          "initial-caps form; if the word is an acronym or has special letter "
          "case, such as the acronym `CDDA` or the unit `mW`, add it by "
          "preserving the case of all the letters. A word in the dictionary "
          "will also match its initial-caps form (if the word is in all "
          "lowercase) and all-uppercase form, so a word should be added to "
          "the dictionary in its normal letter case even if used in a "
          "different letter case in a sentence.")
    print("* For a word to be added to the dictionary, it should either be a "
          "real, properly-spelled modern American English word, a foreign "
          "loan word (including romanized foreign names), or a foreign or "
          "made-up word that is used consistently and commonly enough in the "
          "game. Intentional misspelling (including eye dialect) of a word "
          "should not be added unless it has become a common terminology in "
          "the game, because while someone may have a legitimate use for it, "
          "another person may spell it that way accidentally.")
    print("</details>")
