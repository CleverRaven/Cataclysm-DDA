from ..helper import get_singular_name
from ..write_text import write_text
from .monster_attack import parse_monster_attack


def parse_monster_concrete(json, origin, name):
    if "description" in json:
        write_text(json["description"], origin, c_format=False,
                   comment="Description of monster \"{}\"".format(name))

    if "death_function" in json:
        if "message" in json["death_function"]:
            write_text(json["death_function"]["message"], origin,
                       comment="Death message of monster \"{}\"".format(name))

    if "special_attacks" in json:
        for attack in json["special_attacks"]:
            if "type" in attack and attack["type"] == "monster_attack":
                parse_monster_attack(attack, origin)
                continue
            if "description" in attack:
                write_text(attack["description"], origin,
                           comment="Description of special attack of monster "
                           "\"{}\"".format(name))
            if "monster_message" in attack:
                write_text(attack["monster_message"], origin,
                           comment="Monster message of special attack of "
                           "monster \"{}\"".format(name))
            if "targeting_sound" in attack:
                write_text(attack["targeting_sound"], origin,
                           comment="Targeting sound of special attack of "
                           "monster \"{}\"".format(name))
            if "no_ammo_sound" in attack:
                write_text(attack["no_ammo_sound"], origin,
                           comment="No ammo sound of special attack of "
                           "monster \"{}\"".format(name))

    if "weakpoints" in json:
        for weakpoint in json["weakpoints"]:
            if "name" in weakpoint:
                write_text(weakpoint["name"], origin,
                           comment="Sentence fragment describing a "
                           "weakpoint for monster \"{}\"".format(name))


def parse_monster(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Monster name", plural=True)
    elif "id" in json:
        name = json["id"]

    parse_monster_concrete(json, origin, name)

    if "extend" in json:
        parse_monster_concrete(json["extend"], origin, name)
