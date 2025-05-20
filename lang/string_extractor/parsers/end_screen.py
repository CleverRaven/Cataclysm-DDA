from ..write_text import write_text


def parse_end_screen(json, origin):
    if "added_info" in json:
        for line in json["added_info"]:
            write_text(line[1], origin,
                       comment="String from the end screen, used in ASCII-art,"
                               " so be aware of the string length.")

    if "last_words_label" in json:
        write_text(json["last_words_label"], origin,
                   comment="String used to label the last word input prompt."
                           "ex: \"Last Words:\")")
