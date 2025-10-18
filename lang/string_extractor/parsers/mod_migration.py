from ..write_text import write_text


def parse_mod_migration(json, origin):
    ident = json["id"]

    write_text(json.get("removal_reason"), origin,
               comment=f"Explaination for removal of mod '{ident}'")
