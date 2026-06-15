#!/usr/bin/env python3
import json
from optparse import OptionParser
import os.path
import polib
import shutil
import sys


parser = OptionParser()
parser.add_option("-j", "--json-output", dest="output_file",
                  help="Write result in structured JSON format to file")
(options, args) = parser.parse_args()


class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def print_color(text, color):
    for s in text.split("\n"):
        if not s:
            print()
        else:
            print(f"{color}{s}{bcolors.ENDC}")


def read_all_messages(path):
    if not os.path.isfile(path):
        raise Exception("cannot read {}".format(path))
    pofile = polib.pofile(path)
    messages = set()
    for entry in pofile.untranslated_entries():
        ctxt = entry.msgctxt or ""
        messages.add((entry.msgid, ctxt))
        if entry.msgid_plural:
            messages.add((entry.msgid_plural, ctxt))
    return messages


def compare_po(old_pot_path, new_pot_path):
    print(f"Reading '{old_pot_path}'...")
    old_messages = read_all_messages(old_pot_path)
    print(f"  Read {len(old_messages)} message(s)")
    print()

    print(f"Reading '{new_pot_path}'...")
    new_messages = read_all_messages(new_pot_path)
    print(f"  Read {len(new_messages)} message(s)")
    print()

    print("Computing differences...")
    deleted_messages = list(sorted(old_messages - new_messages))
    added_messages = list(sorted(new_messages - old_messages))
    print()
    return (deleted_messages, added_messages)


def output_to_screen(deleted_messages, added_messages):
    columns = shutil.get_terminal_size((80, 24)).columns
    delim = "-" * columns

    if len(deleted_messages) == 0:
        print("No message deleted.")
    else:
        print(f"{len(deleted_messages)} message(s) deleted:")
        for msg, ctxt in deleted_messages:
            print_color(delim, bcolors.OKCYAN)
            print_color(f"[{ctxt}] {msg}" if ctxt else msg, bcolors.OKCYAN)
        print_color(delim, bcolors.OKCYAN)
    print()

    if len(added_messages) == 0:
        print("No message added.")
    else:
        print(f"{len(added_messages)} message(s) added:")
        for msg, ctxt in added_messages:
            print_color(delim, bcolors.OKGREEN)
            print_color(f"[{ctxt}] {msg}" if ctxt else msg, bcolors.OKGREEN)
        print_color(delim, bcolors.OKGREEN)


def output_to_json(path, deleted_messages, added_messages):
    result = {
        "deleted": list(set([msg for msg, ctxt in deleted_messages])),
        "added": list(set([msg for msg, ctxt in added_messages]))
    }
    with open(path, "w") as fp:
        fp.write(json.dumps(result))
    print(f"Comparison result written to '{path}'\n")


def main():
    if len(sys.argv) < 3:
        print("Usage: python3 ./tools/pot_diff.py <old.pot> <new.pot>")
        return 1
    (deleted_messages, added_messages) = compare_po(sys.argv[1], sys.argv[2])
    if options.output_file:
        output_to_json(options.output_file, deleted_messages, added_messages)
    output_to_screen(deleted_messages, added_messages)
    return 0


exit(main())
