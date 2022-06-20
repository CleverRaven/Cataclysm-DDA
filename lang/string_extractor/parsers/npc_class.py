from ..helper import get_singular_name
from ..write_text import write_text


def parse_npc_class(json, origin):
    comment = json.get("//", None)
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin,
                   comment=["Name of an NPC class", comment])
    elif "id" in json:
        name = json["id"]
    if "job_description" in json:
        write_text(json["job_description"], origin,
                   comment=["Job description of \"{}\" NPC class".format(name),
                            comment])
    if "shopkeeper_item_group" in json:
        for group in json["shopkeeper_item_group"]:
            if "refusal" in group:
                commentg = group.get("//", None)
                write_text(group["refusal"], origin,
                           comment=["Refusal reason for a shop item group",
                                    commentg])
