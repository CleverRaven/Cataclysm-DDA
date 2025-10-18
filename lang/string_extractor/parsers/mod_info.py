from ..write_text import write_text


def parse_mod_info(json, origin):
    name = json["name"]
    write_text(name, origin, comment="MOD name")
    write_text(json["description"], origin,
               comment=f"Description of MOD '{name}'")
