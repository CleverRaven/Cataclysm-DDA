from ..write_text import write_text


def parse_end_screen(json, origin):
    if "last_words_label" in json:
        write_text(json["last_words_label"], origin,
                   comment="String used to label the last word input prompt."
                           "ex: \"Last Words:\")")
