from ..write_text import write_text


def parse_npc(json, origin):
    gender = "an"
    if "gender" in json:
        gender = "a {}".format(json["gender"])
    comment = json.get("//", None)
    if "name_unique" in json:
        write_text(json["name_unique"], origin,
                   comment=["Unique name of {} NPC".format(gender), comment])
    if "name_suffix" in json:
        write_text(json["name_suffix"], origin,
                   comment=["Name suffix of {} NPC".format(gender), comment])
