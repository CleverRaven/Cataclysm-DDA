#!/usr/bin/env python3

import argparse
import re
import pathlib

import polib


CHECK_TAGS = False
CHECK_ELLIPSES = False

PATTERN_TAG = re.compile(r"<[a-z/0-9_]+>")
PATTERN_PRINTF = re.compile(r"%[0-9$.]*[diufegxscp]")

PO_PATH = pathlib.Path("lang/po/")
PO_FILES = dict([f.stem, f] for f in sorted(PO_PATH.glob("*.po")))
# PO_FILES = { "ar": Path("lang/po/ar.po").., }


# tags that can be safely omitted in translation
TAGS_OPTIONAL = (
    "<zombie>", "<zombies>",
    "<name_b>", "<name_g>",
    "<freaking>", "<the_cataclysm>",
    "<granny_name_g>",
    "<mypronoun>", "<mypossesivepronoun>",
)

# known false-positive strings. FIXME?
SKIP_STRINGS = (
    "<empty>",
    "Even if you climb down safely, you will fall "
    "<color_yellow>at least %d story</color>.",
    "Pointed in your direction, the %s emits an IFF warning beep.",
    "%s points in your direction and emits an IFF warning beep.",
    "You hear a warning beep.",
)


def _f(input_list):
    """Formats list"""
    """ ['a', 'b'] => `a b` """

    if not input_list:
        return "none"
    if type(input_list) is str:
        return f"`{input_list}`"
    return f"`{' '.join(input_list)}`"


##########################
# ELLIPSES CHECK         #
##########################


def check_ellipses(entry):
    """Checks if translations use three dots instead of the ellipsis symbol"""

    msgstr = entry.msgstr or entry.msgstr_plural[0]
    if ("…" in entry.msgid) and \
       ("..." in msgstr):
        return [" * It's recommended to use the `…` symbol "
                "instead of three dots."]
    return []


##########################
# PRINTF CHECK           #
##########################


def _is_mixed(tags):
    """Checks if the list of tags contains mixed arguments,"""
    """non-positional %s and positional %1$s at the same time."""

    have_positional = 0
    have_nonpositional = 0

    for tag in tags:
        if "$" in tag:
            have_positional += 1
        else:
            have_nonpositional += 1

    return have_positional and have_nonpositional


def _make_pos_arg(tags):
    """Changes arguments to positional ones."""

    pos_tags = []

    for i, tag in enumerate(tags, 1):
        if "$" in tag:
            pos_tags.append(tag)
            continue
        tag = f"%{i}${tag.split('%')[1]}"
        pos_tags.append(tag)

    return sorted(pos_tags)


def check_printf(entry):
    """Checks whether formatting arguments %s/%1$d"""
    """in the source string and in the translation match."""

    # error messages
    msgs = []

    # find all <tags> in the original string
    tags_msgid = PATTERN_PRINTF.findall(entry.msgid)
    # find all <tags> in the translated string
    if entry.msgstr:
        tags_msgstr = PATTERN_PRINTF.findall(entry.msgstr)
    # find all <tags> for the plural translated strings
    elif entry.msgstr_plural:
        tags_msgstr_pl = [PATTERN_PRINTF.findall(v)
                          for v in entry.msgstr_plural.values()]
        tags_msgstr = tags_msgstr_pl[0]

        # assume that all strings should have the same set of arguments
        for tag in tags_msgstr_pl:
            if tag != tags_msgstr:
                msgs.append(" * Plural strings have"
                            " different sets of arguments;")
                break

    # strings must not have %s and %1$s at the same time
    if _is_mixed(tags_msgid) or _is_mixed(tags_msgstr):
        msgs.append(" * Cannot mix positional and non-positional "
                    "arguments in format string;")
    if msgs:
        return msgs

    # if everything else is good
    if tags_msgid == tags_msgstr:
        return msgs
    # and if everything is ok when replacing with positional arguments
    tags_msgid_pos = _make_pos_arg(tags_msgid)
    if tags_msgid_pos == _make_pos_arg(tags_msgstr):
        return msgs

    # check if the number of arguments matches
    if len(tags_msgid) == len(tags_msgstr):
        # if non-positional arguments are swapped
        # then suggest positional syntax
        if sorted(tags_msgid) == sorted(tags_msgstr):
            msgs.append(" * The types of arguments differ;\n"
                        "   Use positional syntax:"
                        f" {_f(tags_msgid)} => {_f(tags_msgid_pos)};")
        # otherwise it's most likely just a typo
        else:
            msgs.append(" * The types of arguments differ:"
                        f" {_f(tags_msgid)} and {_f(tags_msgstr)};")
    else:
        msgs.append(" * The number of arguments differ:"
                    f" {_f(tags_msgid)} and {_f(tags_msgstr)};")

    return msgs


##########################
# TAGS CHECK             #
##########################


def check_tags(entry):
    """Checks if <tags> in the source string match those in the translation."""

    # error messages
    msgs = []

    # find all <tags> in the original string
    tags_msgid = sorted(PATTERN_TAG.findall(entry.msgid))
    # find all <tags> in the translated string
    if entry.msgstr:
        tags_msgstr = sorted(PATTERN_TAG.findall(entry.msgstr))
    # find all <tags> for the plural translated strings
    elif entry.msgstr_plural:
        tags_msgstr_pl = [sorted(PATTERN_TAG.findall(v))
                          for v in entry.msgstr_plural.values()]
        tags_msgstr = tags_msgstr_pl[0]

        # assume that all strings should have the same set of tags
        for tag in tags_msgstr_pl:
            if tag != tags_msgstr:
                msgs.append(" * Plural strings have different sets of tags;")
                break

    # if everything else is good
    if tags_msgid == tags_msgstr:
        return msgs

    # remove common <tags>
    # both lists will contain only tags that are unique to them
    for tag in reversed(tags_msgid):
        if tag in reversed(tags_msgstr):
            tags_msgid.remove(tag)
            tags_msgstr.remove(tag)

    # filter optional <tags>
    for tag in reversed(tags_msgid):
        if tag in TAGS_OPTIONAL:
            if CHECK_TAGS:
                msgs.append(f" * Missing OPTIONAL tag(s): {_f(tag)};")
            tags_msgid.remove(tag)

    # gather error messages to the list and return it
    if tags_msgid:
        msgs.append(f" * Missing tag(s): {_f(tags_msgid)};")
    if tags_msgstr:
        msgs.append(f" * Bad/extra tag(s): {_f(tags_msgstr)};")
    return msgs


##########################
# MAIN FUNCTIONS         #
##########################


def check_po_file(input_path):
    """Loads a file and checks each line."""

    print(f"## Checking {input_path}")
    po = polib.pofile(input_path)

    error_list = []

    for entry in po.translated_entries():

        # skip known false-positive strings
        if entry.msgid in SKIP_STRINGS:
            continue

        msgs = []
        if error := check_tags(entry):
            msgs += error
        if error := check_printf(entry):
            msgs += error
        if CHECK_ELLIPSES:
            if error := check_ellipses(entry):
                msgs += error

        if msgs:
            msgs.append(f"```\n{entry}```")
            error_list.append("\n".join(msgs))

    if error_list:
        print(f" > Total: {len(error_list)} warnings.")
        print("\n".join(error_list), end="")
    else:
        print(" * Everything looks fine.")
    print()


def main(locale_list):
    """Main function."""

    if not pathlib.Path(".").cwd().match("Cataclysm-DDA/"):
        print("You must run the script from"
              " the root directory 'Cataclysm-DDA/'")
        return

    if locale_list:
        for locale in locale_list:
            if f := PO_FILES.get(locale):
                check_po_file(f)
            else:
                print(f"## Can't find locale '{locale}'. Skip.")
    else:
        for f in PO_FILES.values():
            check_po_file(f)


def _parse_args():
    argparser = argparse.ArgumentParser(
        description="Validates translation files.",
        epilog=f"Available locales: {', '.join(PO_FILES.keys())}")
    argparser.add_argument("locale", nargs="*",
                           metavar="locale-name",
                           help="locale or list of locales")
    argparser.add_argument("-c1", "--check-tags", action="store_true",
                           help="also check optional tags")
    argparser.add_argument("-c2", "--check-ellipses", action="store_true",
                           help="also check ellipses")
    return argparser.parse_args()


if __name__ == "__main__":
    args = _parse_args()
    CHECK_TAGS = args.check_tags
    CHECK_ELLIPSES = args.check_ellipses
    main(args.locale)
