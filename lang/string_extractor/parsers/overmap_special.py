from ..write_text import write_text


def parse_camp_name(overmap, origin):
    if "camp_name" in overmap:
        write_text(overmap["camp_name"], origin,
                   comment="Name of NPC faction camp")


def parse_overmap_special(json, origin):
    if "subtype" in json and json["subtype"] == "mutable":
        for mutable_overmap, overmap in json["overmaps"].items():
            parse_camp_name(overmap, origin)
    else:
        for overmap in json["overmaps"]:
            parse_camp_name(overmap, origin)
