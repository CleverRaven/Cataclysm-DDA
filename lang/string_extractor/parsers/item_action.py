from ..write_text import write_text


def parse_item_action(json, origin):
    if "name" in json:
        write_text(json["name"], origin,
                   comment="Item action name of \"{}\"".format(json["id"]))
