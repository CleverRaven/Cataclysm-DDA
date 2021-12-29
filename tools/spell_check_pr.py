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
    typos = spell_check(message)
    if len(typos) == 0:
        continue
    message = message.replace('\n', "\\n")
    for typo in typos:
        bold = "**" + typo + "**"
        message = re.sub(re.escape(typo), lambda _: bold, message,
                         flags=re.IGNORECASE)
    errors.append(message)
if errors:
    print("**This is an automated message. Please kindly disregard"
          " if you think this is inaccurate.**")
    print()
    print("Automatic spell checking encountered unrecognized words "
          "in the following text:")
    print()
    for err in errors:
        print("*", err)
