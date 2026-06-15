from ..helper import get_singular_name
from ..write_text import write_text


def parse_json_flag(json, origin):
    ident = json["id"]
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment=f"Name of JSON flag '{ident}'")

    write_text(json.get("info"), origin, comment=[
        "Please leave anything in <angle_brackets> unchanged.",
        f"Description of JSON flag '{name}'"])

    write_text(json.get("restriction"), origin,
               comment="Description of restriction "
               f"of JSON flag '{name}'")

    write_text(json.get("item_prefix"), origin,
               comment="Custom prefix added to item name")
    write_text(json.get("item_suffix"), origin,
               comment="Custom suffix added to item name")
