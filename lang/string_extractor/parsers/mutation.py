from ..helper import get_singular_name
from ..write_text import write_text


def parse_mutation(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Mutation name")

    if "description" in json:
        write_text(json["description"], origin, c_format=False,
                   comment="Description of mutation \"{}\"".format(name))

    if "attacks" in json:
        attacks = json["attacks"]
        if type(attacks) is dict:
            attacks = [attacks]
        for attack in attacks:
            if "attack_text_u" in attack:
                write_text(attack["attack_text_u"], origin,
                           comment="Message when player with mutation "
                           "\"{}\" attacks".format(name))
            if "attack_text_npc" in attack:
                write_text(attack["attack_text_npc"], origin,
                           comment="Message when NPC with mutation "
                           "\"{}\" attacks".format(name))

    if "ranged_mutation" in json:
        if "message" in json["ranged_mutation"]:
            write_text(json["ranged_mutation"]["message"], origin,
                       comment="Message when firing ranged attack with "
                       "mutation \"{}\"".format(name))

    if "spawn_item" in json:
        if "message" in json["spawn_item"]:
            write_text(json["spawn_item"]["message"], origin,
                       comment="Message when spawning item \"{}\" "
                       "with mutation \"{}\""
                       .format(json["spawn_item"]["type"], name))

    if "triggers" in json:
        for arr in json["triggers"]:
            for trigger in arr:
                if "msg_on" in trigger and "text" in trigger["msg_on"]:
                    write_text(trigger["msg_on"]["text"], origin,
                               comment="Trigger message of mutation \"{}\""
                               .format(name))
                if "msg_off" in trigger and "text" in trigger["msg_off"]:
                    write_text(trigger["msg_off"]["text"], origin,
                               comment="Trigger message of mutation \"{}\""
                               .format(name))

    if "variants" in json:
        for variant in json["variants"]:
            variant_name = get_singular_name(variant["name"])
            write_text(variant["name"], origin,
                       comment="Variant name of mutation \"{}\"".format(name))
            write_text(variant["description"], origin,
                       comment="Description of variant " +
                       "\"{1}\" of mutation\"{0}\"".format(name, variant_name))

    if "transform" in json:
        write_text(json["transform"]["msg_transform"], origin,
                   comment="Message when transforming from mutation "
                   " \"{}\" to \"{}\""
                   .format(name, json["transform"]["target"]))
