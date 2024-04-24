from ..helper import get_singular_name
from ..write_text import write_text


def parse_effect_type(json, origin):
    # Using a list to reserve order and ensure stability across runs
    effect_name = []

    if "name" in json:
        for name in json["name"]:
            write_text(name, origin,
                       comment="Name of effect type id \"{}\""
                       .format(json["id"]))
            singular = get_singular_name(name)
            if len(singular) > 0 and singular not in effect_name:
                effect_name.append(singular)

    if len(effect_name) > 0:
        effect_name = ", ".join(effect_name)
    else:
        effect_name = json["id"]

    if "desc" in json:
        # See effect.cpp
        use_desc_ints = json.get("max_intensity", 1) <= len(json["desc"])
        for idx, desc in enumerate(json["desc"]):
            if use_desc_ints:
                names = json.get("name", [])
                if len(names) > 0:
                    singular = get_singular_name(
                        names[min(len(names) - 1, idx)])
                    if len(singular) == 0:
                        singular = json["id"]
                else:
                    singular = json["id"]
            else:
                singular = effect_name
            write_text(desc, origin,
                       comment="Description of effect type \"{}\""
                       .format(singular))

    if "speed_name" in json:
        write_text(json["speed_name"], origin,
                   comment="Speed name of effect type \"{}\""
                   .format(effect_name))

    if "apply_message" in json:
        if type(json["apply_message"]) is list:
            for msg in json["apply_message"]:
                write_text(msg[0], origin,
                           comment="Apply message of effect type \"{}\""
                           .format(effect_name))
        elif type(json["apply_message"]) is str:
            write_text(json["apply_message"], origin,
                       comment="Apply message of effect type \"{}\""
                       .format(effect_name))

    if "remove_message" in json:
        write_text(json["remove_message"], origin,
                   comment="Remove message of effect type \"{}\""
                   .format(effect_name))

    if "death_msg" in json:
        write_text(json["death_msg"], origin,
                   comment="Death message of effect type \"{}\""
                   .format(effect_name))

    if "miss_messages" in json:
        for msg in json["miss_messages"]:
            write_text(msg[0], origin,
                       comment="Miss message of effect type \"{}\""
                       .format(effect_name))

    if "decay_messages" in json:
        for msg in json["decay_messages"]:
            write_text(msg[0], origin,
                       comment="Decay message of effect type \"{}\""
                       .format(effect_name))

    if "apply_memorial_log" in json:
        write_text(json["apply_memorial_log"], origin,
                   context="memorial_male",
                   comment="Male memorial apply log of effect type \"{}\""
                   .format(effect_name))
        write_text(json["apply_memorial_log"], origin,
                   context="memorial_female",
                   comment="Female memorial apply log of effect type \"{}\""
                   .format(effect_name))

    if "remove_memorial_log" in json:
        write_text(json["remove_memorial_log"], origin,
                   context="memorial_male",
                   comment="Male memorial remove log of effect type \"{}\""
                   .format(effect_name))
        write_text(json["remove_memorial_log"], origin,
                   context="memorial_female",
                   comment="Female memorial remove log of effect type \"{}\""
                   .format(effect_name))
