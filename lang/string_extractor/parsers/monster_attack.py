from ..write_text import write_text


def parse_monster_attack(json, origin):
    for key in [
        "hit_dmg_u",
        "hit_dmg_npc",
        "miss_msg_u",
        "miss_msg_npc",
        "no_dmg_msg_u",
        "no_dmg_msg_npc",
    ]:
        if key in json:
            write_text(
                json[key],
                origin,
                comment='Monster attack "{}" message'.format(json["id"]),
            )
