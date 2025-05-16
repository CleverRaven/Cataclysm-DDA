from ..write_text import write_text


def parse_item_category(json, origin):
    if "name_header" in json:
        write_text(json["name_header"], origin,
                   comment="Item category name used for headers")
    if "name_noun" in json:
        write_text(json["name_noun"], origin,
                   comment="Item category name used for descriptive text",
                   plural=True)
