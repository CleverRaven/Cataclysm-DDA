from ..write_text import write_text


def parse_end_screen(json, origin):
    for line in json.get("added_info", []):
        write_text(line[1], origin,
                   comment="String from the end screen, used in ASCII-art,"
                   " so be aware of the string length.")

    write_text(json.get("last_words_label"), origin,
               comment="String used to label the last word input prompt."
               "ex: 'Last Words:')")
