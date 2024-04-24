import codecs
from datetime import datetime, timezone
import json
import os
import polib

from .message import messages, occurrences


def deduplciate(comments):
    """Remove duplicate comment lines while preserving order."""
    seen = set()
    result = []
    for comment in comments:
        if comment not in seen:
            result.append(comment)
            seen.add(comment)
    return result


def is_unicode(sequence):
    hex = "0123456789abcdef"
    return sequence[0] == "\\" and sequence[1] == "u" and \
        sequence[2] in hex and sequence[3] in hex and \
        sequence[4] in hex and sequence[5] in hex


def restore_unicode(string):
    i = string.find("\\u")
    while i != -1 and i + 5 < len(string):
        slice = string[i:i + 6]
        if is_unicode(slice):
            string = string.replace(slice,
                                    codecs.unicode_escape_decode(slice)[0])
        i = string.find("\\u")
    return string


def format_msg(prefix, text):
    return "{0} {1}".format(prefix, restore_unicode(json.dumps(text)))


def write_pot_header(fp, pkg_name="Cataclysm-DDA"):
    tzinfo = datetime.now(timezone.utc).astimezone().tzinfo
    time = datetime.now(tzinfo).strftime('%Y-%m-%d %H:%M%z')
    print("msgid \"\"", file=fp)
    print("msgstr \"\"", file=fp)
    print("\"Project-Id-Version: {}\\n\"".format(pkg_name), file=fp)
    print("\"POT-Creation-Date: {}\\n\"".format(time), file=fp)
    print("\"PO-Revision-Date: {}\\n\"".format(time), file=fp)
    print("\"Last-Translator: None\\n\"", file=fp)
    print("\"Language-Team: None\\n\"", file=fp)
    print("\"Language: en\\n\"", file=fp)
    print("\"MIME-Version: 1.0\\n\"", file=fp)
    print("\"Content-Type: text/plain; charset=UTF-8\\n\"", file=fp)
    print("\"Content-Transfer-Encoding: 8bit\\n\"", file=fp)
    print("\"Plural-Forms: nplurals=2; plural=(n > 1);\\n\"", file=fp)
    print("", file=fp)


def sanitize_plural_colissions(reference):
    if not os.path.isfile(reference):
        raise Exception("cannot read {}".format(reference))
    pofile = polib.pofile(reference)
    for entry in pofile.untranslated_entries():
        if entry.msgid_plural:
            pair = (entry.msgctxt if entry.msgctxt else "", entry.msgid)
            if pair in messages:
                if len(messages[pair]) == 1:
                    if messages[pair][0].text_plural == "":
                        messages[pair][0].text_plural = entry.msgid_plural


def write_to_pot(fp, with_header=True, pkg_name=None, sanitize=None):
    if sanitize:
        sanitize_plural_colissions(sanitize)
    if with_header:
        write_pot_header(fp, pkg_name)
    for (context, text) in occurrences:
        if (context, text) not in messages:
            continue
        comments = []
        origins = set()
        format_tag = ""
        text_plural = ""
        for message in messages[(context, text)]:
            comments = comments + message.comments
            origins.add(message.origin)
            if message.format_tag:
                format_tag = message.format_tag
            if message.text_plural:
                text_plural = message.text_plural
        origin = " ".join(sorted(origins))

        # translator comments
        for line in deduplciate(comments):
            print("#. ~ {}".format(line), file=fp)

        # reference
        print("#: {}".format(origin), file=fp)

        # c-format
        if format_tag:
            print("#, {}".format(format_tag), file=fp)

        # context
        if context:
            print("msgctxt \"{}\"".format(context), file=fp)

        # text
        if text_plural:
            print(format_msg("msgid", text), file=fp)
            print(format_msg("msgid_plural", text_plural), file=fp)
            print("msgstr[0] \"\"", file=fp)
            print("msgstr[1] \"\"", file=fp)
        else:
            print(format_msg("msgid", text), file=fp)
            print("msgstr \"\"", file=fp)

        print("", file=fp)

        del messages[(context, text)]
