from ..write_text import write_text


def parse_item_action(json, origin):
    ident = json["id"]
    write_text(json.get("name"), origin,
               comment=f"Item action name of '{ident}'")
