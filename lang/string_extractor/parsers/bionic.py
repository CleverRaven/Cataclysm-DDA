from .enchant import parse_enchant
from ..helper import get_singular_name
from ..write_text import write_text


def parse_bionic(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin, comment="Name of a bionic")
    write_text(json.get("description"), origin,
               comment=f"Description of bionic '{name}'")
    write_text(json.get("cant_remove_reason"), origin,
               comment=f"Reason bionic '{name}' cannot be removed")

    for enchantment in json.get("enchantments", []):
        parse_enchant(enchantment, origin)
