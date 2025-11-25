from .enchant import parse_enchant
from ..helper import get_singular_name
from .use_action import parse_use_action
from ..write_text import write_text
from .gun import parse_gun
from .gunmod import parse_gunmod


def parse_subtypes(json, origin):

    item_type = "item"

    if "subtypes" in json:
        item_subtypes = []
        if "GUN" in json["subtypes"]:
            parse_gun(json, origin)
            item_subtypes.append("gun")
        if "GUNMOD" in json["subtypes"]:
            parse_gunmod(json, origin)
            item_subtypes.append("gun mod")
        if "MAGAZINE" in json["subtypes"]:
            item_subtypes.append("magazine")
        if "BIONIC_ITEM" in json["subtypes"]:
            item_subtypes.append("compact bionic module")
        if "ARMOR" in json["subtypes"]:
            item_subtypes.append("armor")
        if "COMESTIBLE" in json["subtypes"]:
            item_subtypes.append("comestible")
        if "TOOL" in json["subtypes"]:
            item_subtypes.append("tool")
        if "TOOLMOD" in json["subtypes"]:
            item_subtypes.append("tool mod")
        if "BOOK" in json["subtypes"]:
            item_subtypes.append("book")
        if "AMMO" in json["subtypes"]:
            item_subtypes.append("ammunition")
        if item_subtypes:
            item_type = "/".join(item_subtypes)

    name = get_singular_name(json)
    isbn = ""

    if "//isbn13" in json:
        isbn = "ISBN {}".format(json["//isbn13"])

    write_text(json.get("name"), origin,
               comment=[isbn, f"{item_type.capitalize()} name"],
               plural=True)
    write_text(json.get("description"), origin,
               comment=[isbn, f"Description of {item_type} '{name}'"])

    for variant in json.get("variants", []):
        variant_name = get_singular_name(variant)
        write_text(variant.get("name"), origin,
                   comment=f"Variant name of {item_type} '{name}'",
                   plural=True)
        write_text(variant.get("description"), origin,
                   comment="Description of variant"
                   f" '{variant_name}' of {item_type} '{name}'")

    return name


def parse_generic(json, origin):

    name = parse_subtypes(json, origin)

    parse_use_action(json.get("use_action"), origin, name)
    parse_use_action(json.get("tick_action"), origin, name)

    for cname in json.get("conditional_names", []):
        write_text(cname["name"], origin,
                   comment="Conditional name for '{}' when '{}' matches '{}'"
                   .format(name, cname["type"], cname["condition"]),
                   plural=True)

    if type(json.get("snippet_category")) is list:
        # snippet_category is either a simple string (the category ident)
        # which is not translated, or an array of snippet texts.
        for entry in json["snippet_category"]:
            if type(entry) is str:
                write_text(entry, origin,
                           comment=f"Snippet of item '{name}'")
            elif type(entry) is dict:
                write_text(entry["text"], origin,
                           comment=f"Snippet of item '{name}'")

    write_text(json.get("plant_name"), origin,
               comment=f"Plant name of seed '{name}'")

    write_text(json.get("revert_msg"), origin,
               comment=f"Breaking/draining message of tool '{name}'")

    for pocket in json.get("pocket_data", []):
        write_text(pocket.get("name"), origin,
                   comment=f"Short name of a pocket in '{name}'")
        write_text(pocket.get("description"), origin,
                   comment=f"Description of a pocket in '{name}'")

    if "passive_effects" in json.get("relic_data", []):
        for enchantment in json["relic_data"]["passive_effects"]:
            parse_enchant(enchantment, origin)

    write_text(json.get("e_port"), origin, comment="E-port name")
    for e_port in json.get("e_ports_banned", []):
        write_text(e_port, origin, comment="E-port name")
