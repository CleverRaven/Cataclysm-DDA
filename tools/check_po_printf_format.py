#!/usr/bin/env python3
import polib
import os
import re
types = ["d", "i", "u", "f", "e", "g", "x", "s", "c", "p"]


def findall(p, s):
    i = s.find(p)
    while i != -1:
        yield i
        i = s.find(p, i + 1)


def is_formatted_entry(msg):
    if "%" not in msg:
        return False
    indices = list(findall("%", msg))
    index_iter = iter(indices)
    for i in index_iter:
        if i + 1 == len(msg):
            return False
        if msg[i + 1] == "%":  # skip %%
            next(index_iter, None)
            continue
        if not msg[i + 1].isnumeric() and \
           msg[i + 1] not in ["d", "s", "c", "l", "f", "."]:
            return False
        if msg[i + 1] == "." and \
           (i + 2 == len(msg) or not msg[i + 2].isnumeric()):
            return False
    return True


def decompose_format_strings(msg):
    indices = list(findall("%", msg))
    non_positional = []
    positional = []
    segments = []
    index_iter = iter(indices)
    for i in index_iter:
        if i + 1 == len(msg):
            break
        if msg[i + 1] == "%":  # skip %%
            next(index_iter, None)
            continue
        if msg[i + 1].isnumeric() and msg[i + 2] == "$":
            # positional format strings %1$s, %2$d, etc.
            positional.append(msg[i:i + 4])
            segments.append((i, i + 4))
        else:
            # non-positional format strings %s, %3d, etc.
            idx = i + 1
            while msg[idx] not in types and idx + 1 < len(msg):
                idx += 1
            if idx == len(msg):
                break
            # ignore the cases where "%d" is translated to "%2d"
            non_positional.append(re.sub(r'[0-9]+', '', msg[i:idx + 1]))
            segments.append((i, idx + 1))
    return (non_positional, positional, segments)


def check_message(entry):
    msgid = entry.msgid
    msgstr = entry.msgstr
    if not is_formatted_entry(msgid) and not is_formatted_entry(msgstr):
        return (True, [], [])
    (non_pos_msgid, pos_msgid, seg_msgid) = decompose_format_strings(msgid)
    (non_pos_msgstr, pos_msgstr, seg_msgstr) = decompose_format_strings(msgstr)
    if non_pos_msgid == non_pos_msgstr and \
       sorted(pos_msgid) == sorted(pos_msgstr):
        return (True, [], [])
    # Allow "%1$s %2$d" translated to "%s %d"
    # as long as the order of types is preserved
    if len(non_pos_msgid) == 0 and len(pos_msgid) > 0 and \
       len(non_pos_msgstr) == len(pos_msgid) and len(pos_msgstr) == 0:
        for i in range(len(pos_msgid)):
            if pos_msgid[i][-1] != non_pos_msgstr[i][-1]:
                return (False, seg_msgid, seg_msgstr)
        return (True, [], [])
    # otherwise it's an error
    return (False, seg_msgid, seg_msgstr)


def check_po_file(file):
    pofile = polib.pofile(file)
    errors = []
    for entry in pofile.translated_entries():
        if entry.msgid_plural:
            # TODO: implement proper check for plural messages
            continue
        (ok, seg_msgid, seg_msgstr) = check_message(entry)
        if not ok:
            errors.append((entry.msgid, entry.msgstr, seg_msgid, seg_msgstr))
    return errors


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


def print_message(msg, segments):
    beg = [x for (x, y) in segments]
    end = [y for (x, y) in segments]
    for i in range(len(msg)):
        if i in beg:
            print(bcolors.FAIL, end="")
            print(bcolors.UNDERLINE, end="")
        elif i in end:
            print(bcolors.ENDC, end="")
        print(msg[i], end="")
    print()


num_errors = 0
for file in sorted(os.listdir("lang/po")):
    if file.endswith(".po") and not file.endswith("en.po"):
        print("Checking", file, end="", flush=True)
        errors = check_po_file("lang/po/" + file)
        n = len(errors)
        num_errors += n
        if n > 0:
            print(f" => {bcolors.FAIL}{n} error(s) detected:{bcolors.ENDC}")
            for (msgid, msgstr, seg_msgid, seg_msgstr) in errors:
                print(f"{bcolors.BOLD}original  :{bcolors.ENDC}", end="")
                print_message(msgid.replace("\n\n", "\n"), seg_msgid)
                print(f"{bcolors.BOLD}translated:{bcolors.ENDC}", end="")
                print_message(msgstr.replace("\n\n", "\n"), seg_msgstr)
                print()
        else:
            print(f" => {bcolors.OKGREEN}No error detected.{bcolors.ENDC}")
        print()
exit(num_errors)
