import codecs
from datetime import datetime, timezone
import json
import os
import polib

from .message import messages, occurrences


def process_comments(comments, origins, obsolete_paths):
    result = []

    # remove duplicate comment lines while preserving order
    seen = set()
    for comment in comments:
        if comment not in seen:
            result.append(comment)
            seen.add(comment)

    # add 'obsolete' comment if this string
    # has only obsolete files in the origins
    obsolete_count = 0
    for origin in origins:
        is_obsolete = False

        if "obsolet" in origin:
            # if the file path contains "obsolete"/"obsoletion"
            is_obsolete = True
        else:
            # if the file path matches the obsolete paths
            # explicitly specified in the '-D' arguments
            for o_path in obsolete_paths:
                p = os.path.commonpath([o_path, origin])
                if p in obsolete_paths:
                    is_obsolete = True
                    break
        if is_obsolete:
            obsolete_count += 1

    if obsolete_count == len(origins):
        return [
            "[DEPRECATED] Don't translate this line.",
            "This string is from an obsolete source "
            "and will no longer be used in the game."
        ]
    return result


def sanitize(reference, pkg_name="Cataclysm-DDA"):
    if not os.path.isfile(reference):
        raise Exception(f"Cannot read {reference}")
    pofile = polib.pofile(reference)

    # sanitize plural entries
    # Multiple objects may define slightly different plurals for strings,
    # but without the specified context only one such string can be stored.
    # Adds a plural form to those matching strings that do not have it.
    for entry in pofile.untranslated_entries():
        pair = (entry.msgctxt if entry.msgctxt else "", entry.msgid)
        if pair in messages:
            # first, check the messages against the reference
            for m in messages[pair]:
                if not m.text_plural and entry.msgid_plural:
                    m.text_plural = entry.msgid_plural
                    break
            # then check the reference against the messages
            # prioritize plurals that are explicitly specified in JSON
            temp_plural = ""
            for m in messages[pair]:
                if m.text_plural and not entry.msgid_plural:
                    entry.msgstr_plural = {0: "", 1: ""}
                    if m.explicit_plural:
                        entry.msgid_plural = m.text_plural
                        break
                    temp_plural = m.text_plural
            if temp_plural:
                entry.msgid_plural = temp_plural

    # write the correct header.
    tzinfo = datetime.now(timezone.utc).astimezone().tzinfo
    tztime = datetime.now(tzinfo).strftime('%Y-%m-%d %H:%M%z')
    pofile.metadata = {
        "Project-Id-Version": pkg_name,
        "POT-Creation-Date": f"{tztime}",
        "PO-Revision-Date": f"{tztime}",
        "Last-Translator": "None",
        "Language-Team": "None",
        "Language": "en",
        "MIME-Version": "1.0",
        "Content-Type": "text/plain; charset=UTF-8",
        "Content-Transfer-Encoding": "8bit",
        "Plural-Forms": "nplurals=2; plural=(n > 1);"
    }
    pofile.metadata_is_fuzzy = 0

    pofile.save()


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


def format_msg(text):
    return restore_unicode(json.dumps(text))


def write_to_pot(fp, obsolete_paths=[]):
    entries = []
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
        entry = []

        # translator comments
        for line in process_comments(comments, origins, obsolete_paths):
            entry.append(f"#. ~ {line}")

        # reference
        entry.append(f"#: {origin}")

        # c-format
        if format_tag:
            entry.append(f"#, {format_tag}")

        # context
        if context:
            entry.append(f"msgctxt \"{context}\"")

        # text
        if text_plural:
            entry.append(f"msgid {format_msg(text)}\n"
                         f"msgid_plural {format_msg(text_plural)}\n"
                         "msgstr[0] \"\"\n"
                         "msgstr[1] \"\"")
        else:
            entry.append(f"msgid {format_msg(text)}\n"
                         "msgstr \"\"")

        entries.append("\n".join(entry))
        del messages[(context, text)]

    fp.write("\n\n".join(entries))
