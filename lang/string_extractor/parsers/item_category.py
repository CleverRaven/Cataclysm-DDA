from ..write_text import write_text


def parse_item_category(json, origin):
    write_text(json.get("name_header"), origin,
               comment="Item category name used for headers")
    write_text(json.get("name_noun"), origin,
               comment="Item category name used for descriptive text",
               plural=True)
