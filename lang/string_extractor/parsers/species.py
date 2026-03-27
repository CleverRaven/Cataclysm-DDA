from ..write_text import write_text


def parse_species(json, origin):
    name = json["id"]
    write_text(json.get("description"), origin,
               comment=f"Description of species '{name}'")
    write_text(json.get("footsteps"), origin,
               comment=f"Footsteps of species '{name}'")
