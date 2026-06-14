from ..helper import get_singular_name
from ..write_text import write_text
from .monster_attack import parse_monster_attack


def parse_monster_concrete(json, origin, name):
    write_text(json.get("description"), origin,
               comment=f"Description of monster '{name}'")

    if "death_function" in json:
        write_text(json["death_function"].get("message"), origin,
                   comment=f"Death message of monster '{name}'")

    for attack in json.get("special_attacks", []):
        if not type(attack) is dict:
            continue
        if attack.get("type") == "monster_attack":
            parse_monster_attack(attack, origin)
            continue

        write_text(
            attack.get("description"), origin,
            comment=f"Description of special attack of monster '{name}'")
        write_text(
            attack.get("monster_message"), origin,
            comment=f"Message of special attack of monster '{name}'")
        write_text(
            attack.get("targeting_sound"), origin,
            comment=f"Targeting sound of special attack of monster '{name}'")
        write_text(
            attack.get("no_ammo_sound"), origin,
            comment=f"No ammo sound of special attack of monster '{name}'")

    for weakpoint in json.get("weakpoints", []):
        write_text(weakpoint.get("name"), origin,
                   comment="Sentence fragment describing a"
                   f" weakpoint for monster '{name}'")

    if "petfood" in json:
        write_text(json["petfood"].get("feed"), origin,
                   comment=f"Feed message of monster '{name}'")
        write_text(json["petfood"].get("pet"), origin,
                   comment=f"Pet message of monster '{name}'")


def parse_monster(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin, comment="Monster name", plural=True)
    parse_monster_concrete(json, origin, name)

    if "extend" in json:
        parse_monster_concrete(json["extend"], origin, name)
