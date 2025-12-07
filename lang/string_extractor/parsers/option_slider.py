from ..helper import get_singular_name
from ..write_text import write_text


def parse_option_slider(json, origin):
    name = get_singular_name(json["name"])

    write_text(json["name"], origin,
               comment="Name of an option slider")

    for level in json.get("levels", []):
        write_text(level.get("name"), origin,
                   comment="Name of a slider position in the "
                   f"'{name}' option slider")
        write_text(level.get("description"), origin,
                   comment="Description of a slider position in the "
                   f"'{name}' option slider")
