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


def read_all_messages(path):
    if not os.path.isfile(path):
        raise Exception("cannot read {}".format(path))
    pofile = polib.pofile(path)
    messages = set()
    for entry in pofile.untranslated_entries():
        messages.add(entry.msgid)
        if entry.msgid_plural:
            messages.add(entry.msgid_plural)
    return messages


def compare_po(old_pot_path, new_pot_path):
    print(f"Reading {old_pot_path}")
    old_messages = read_all_messages(old_pot_path)
    print(f"{len(old_messages)} message(s) read from {old_pot_path}")
    print()

    print(f"Reading {new_pot_path}")
    new_messages = read_all_messages(new_pot_path)
    print(f"{len(new_messages)} message(s) read from {new_pot_path}")
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
        for msg in deleted_messages:
            print(f"{bcolors.OKCYAN}{delim}{bcolors.ENDC}")
            print(f"{bcolors.OKCYAN}{msg}{bcolors.ENDC}")
        print(f"{bcolors.OKCYAN}{delim}{bcolors.ENDC}")
    print()

    if len(added_messages) == 0:
        print("No message added.")
    else:
        print(f"{len(added_messages)} message(s) added:")
        for msg in added_messages:
            print(f"{bcolors.OKGREEN}{delim}{bcolors.ENDC}")
            print(f"{bcolors.OKGREEN}{msg}{bcolors.ENDC}")
        print(f"{bcolors.OKGREEN}{delim}{bcolors.ENDC}")


def output_to_json(path, deleted_messages, added_messages):
    result = {"deleted": deleted_messages, "added": added_messages}
    with open(path, "w") as fp:
        fp.write(json.dumps(result))


def main():
    if len(sys.argv) < 3:
        print("Usage: python3 ./tools/pot_diff.py <old.pot> <new.pot>")
        return 1
    (deleted_messages, added_messages) = compare_po(sys.argv[1], sys.argv[2])
    if options.output_file:
        output_to_json(options.output_file, deleted_messages, added_messages)
        print("Comparison result written to {}".format(options.output_file))
    else:
        output_to_screen(deleted_messages, added_messages)
    return 0


exit(main())
