from ..write_text import write_text


def parse_overmap_special(json, origin):

    overmaps = json["overmaps"]
    if json.get("subtype") == "mutable":
        overmaps = json["overmaps"].values()

    for om in overmaps:
        write_text(om.get("camp_name"), origin,
                   comment="Name of NPC faction camp")
