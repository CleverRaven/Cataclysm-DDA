from ..helper import get_singular_name
from ..write_text import write_text


def parse_help(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin, comment="Help menu category")

    for msg in json.get("messages", []):
        if type(msg) is dict:
            if msg.get("subtitle"):
                write_text(get_singular_name(msg["subtitle"]), origin,
                           comment=f"Message in help menu category '{name}'")
            elif msg.get("force_monospaced"):
                write_text(get_singular_name(msg["force_monospaced"]), origin,
                           comment=f"Message in help menu category '{name}'")
            elif msg.get("paragraph"):
                write_text(get_singular_name(msg["paragraph"]), origin,
                           comment=f"Message in help menu category '{name}'")
        else:
            write_text(msg, origin,
                       comment=f"Message in help menu category '{name}'")
