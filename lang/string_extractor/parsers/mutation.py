from .enchant import parse_enchant
from ..helper import get_singular_name
from ..write_text import write_text


def parse_mutation(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin, comment="Mutation name")
    write_text(json.get("description"), origin,
               comment=f"Description of mutation '{name}'")

    attacks = json.get("attacks", [])
    if type(attacks) is dict:
        attacks = [attacks]

    for attack in attacks:
        write_text(attack.get("attack_text_u"), origin,
                   comment="Message when player with mutation"
                   f" '{name}' attacks")
        write_text(attack.get("attack_text_npc"), origin,
                   comment="Message when NPC with mutation"
                   f" '{name}' attacks")

    if "ranged_mutation" in json:
        write_text(json["ranged_mutation"].get("message"), origin,
                   comment="Message when firing ranged attack"
                   f" with mutation '{name}'")

    if "message" in json.get("spawn_item", []):
        spawn_type = json["spawn_item"]["type"]
        write_text(json["spawn_item"]["message"], origin,
                   comment="Message when spawning item"
                   f" '{spawn_type}' with mutation '{name}'")

    for arr in json.get("triggers", []):
        for trigger in arr:
            if "msg_on" in trigger:
                write_text(trigger["msg_on"].get("text"), origin,
                           comment=f"Trigger message of mutation '{name}'")
            if "msg_off" in trigger:
                write_text(trigger["msg_off"].get("text"), origin,
                           comment=f"Trigger message of mutation '{name}'")

    for variant in json.get("variants", []):
        variant_name = get_singular_name(variant)
        write_text(variant["name"], origin,
                   comment=f"Variant name of mutation '{name}'")
        write_text(variant["description"], origin,
                   comment="Description of variant"
                   f" '{name}' of mutation '{variant_name}'")

    if "transform" in json:
        target_id = json["transform"]["target"]
        write_text(json["transform"]["msg_transform"], origin,
                   comment="Message when transforming from mutation"
                   f" '{name}' to '{target_id}'")

    for enchantment in json.get("enchantments", []):
        parse_enchant(enchantment, origin)
