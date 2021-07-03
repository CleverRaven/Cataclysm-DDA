#!/usr/bin/env python3
import polib
import os
import re
import sys
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


def get_type(arg):
    answer = ""
    for i in range(len(arg) - 1, -1, -1):
        if arg[i].isnumeric() or arg[i] == '.' \
           or arg[i] == '$' or arg[i] == '%':
            break
        answer = arg[i] + answer
    return answer


def check_message(entry):
    msgid = entry.msgid
    msgstr = entry.msgstr
    if not is_formatted_entry(msgid) and not is_formatted_entry(msgstr):
        return (True, [], [], "")
    (non_pos_msgid, pos_msgid, seg_msgid) = decompose_format_strings(msgid)
    if len(non_pos_msgid) > 0 and len(pos_msgid) > 0:
        return (False, seg_msgid, [],
                "Cannot mix positional and non-positional arguments"
                " in format string")
    (non_pos_msgstr, pos_msgstr, seg_msgstr) = decompose_format_strings(msgstr)
    if len(non_pos_msgstr) > 0 and len(pos_msgstr) > 0:
        return (False, [], seg_msgstr,
                "Cannot mix positional and non-positional arguments"
                " in format string")
    if non_pos_msgid == non_pos_msgstr and \
       sorted(pos_msgid) == sorted(pos_msgstr):
        return (True, [], [], "")
    # "%2$d %1$s" is considered equivalent to "%s %d"
    # as the order of types is preserved
    msgid_types = []
    msgstr_types = []
    for arg in non_pos_msgid + sorted(pos_msgid):
        msgid_types.append(get_type(arg))
    for arg in non_pos_msgstr + sorted(pos_msgstr):
        msgstr_types.append(get_type(arg))
    if len(msgid_types) != len(msgstr_types):
        return (False, seg_msgid, seg_msgstr, "The number of arguments differ")
    for i in range(0, len(msgid_types)):
        if msgid_types[i] != msgstr_types[i]:
            return (False, seg_msgid, seg_msgstr,
                    "The types of arguments differ")
    return (True, [], [], "")


def check_po_file(file):
    pofile = polib.pofile(file)
    errors = []
    for entry in pofile.translated_entries():
        if entry.msgid_plural:
            # TODO: implement proper check for plural messages
            continue
        (ok, seg_msgid, seg_msgstr, reason) = check_message(entry)
        if not ok:
            errors.append((entry.msgid, entry.msgstr,
                           seg_msgid, seg_msgstr, reason))
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
    print(bcolors.ENDC)


po_files = []
for file in sorted(os.listdir("lang/po")):
    if file.endswith(".po") and not file.endswith("en.po"):
        po_files.append(file)
files_to_check = []
if len(sys.argv) == 1:
    files_to_check = po_files
else:
    for i in range(1, len(sys.argv)):
        if sys.argv[i] + ".po" in po_files:
            files_to_check.append(sys.argv[i] + ".po")
        else:
            print("Warning: Unknown language", sys.argv[i])
num_errors = 0
for file in sorted(files_to_check):
    print("Checking", file, end="", flush=True)
    errors = check_po_file("lang/po/" + file)
    n = len(errors)
    num_errors += n
    if n > 0:
        print(f" => {bcolors.FAIL}{n} error(s) detected:{bcolors.ENDC}")
        for (msgid, msgstr, seg_msgid, seg_msgstr, reason) in errors:
            print(f"{bcolors.BOLD}problem   :{bcolors.ENDC}", end="")
            print(reason)
            print(f"{bcolors.BOLD}original  :{bcolors.ENDC}", end="")
            print_message(msgid.replace("\n\n", "\n"), seg_msgid)
            print(f"{bcolors.BOLD}translated:{bcolors.ENDC}", end="")
            print_message(msgstr.replace("\n\n", "\n"), seg_msgstr)
            print()
    else:
        print(f" => {bcolors.OKGREEN}No error detected.{bcolors.ENDC}")
    print()
exit(num_errors)
