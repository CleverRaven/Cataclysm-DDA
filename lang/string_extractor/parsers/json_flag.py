from ..write_text import write_text


def parse_json_flag(json, origin):
    if "info" in json:
        write_text(json["info"], origin, comment=[
            "Please leave anything in <angle brackets> unchanged.",
            "Description of JSON flag \"{}\"".format(json["id"])
        ])
    if "restriction" in json:
        write_text(json["restriction"], origin,
                   comment="Description of restriction of JSON flag \"{}\""
                   .format(json["id"]))
    if "item_prefix" in json:
        write_text(json["item_prefix"], origin,
                   comment="This is custom prefix added to item name. \"{}\""
                   .format(json["id"]))
    if "item_suffix" in json:
        write_text(json["item_suffix"], origin,
                   comment="This is custom suffix added to item name. \"{}\""
                   .format(json["id"]))
