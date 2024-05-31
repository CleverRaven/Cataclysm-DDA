from ..write_text import write_text


def parse_overmap_special(json, origin):
    for overmap in json["overmaps"]:
        if "camp_name" in overmap:
            write_text(overmap["camp_name"], origin,
                       comment="Name of NPC faction camp")
