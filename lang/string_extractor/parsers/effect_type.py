from .enchant import parse_enchant
from ..helper import get_singular_name
from ..write_text import write_text


def parse_effect_type(json, origin):
    # Using a list to reserve order and ensure stability across runs
    ident = json["id"]
    effect_name = []

    for name in json.get("name", []):
        write_text(name, origin,
                   comment=f"Name of effect '{ident}'")

        singular = get_singular_name(name)
        if len(singular) > 0 and singular not in effect_name:
            effect_name.append(singular)

    if len(effect_name) > 0:
        effect_name = ", ".join(effect_name)
    else:
        effect_name = ident

    if "desc" in json:
        # See effect.cpp
        use_desc_ints = json.get("max_intensity", 1) <= len(json["desc"])

        for idx, desc in enumerate(json["desc"]):
            singular = effect_name

            if use_desc_ints:
                names = json.get("name", [])
                if len(names) > 0:
                    singular = get_singular_name(
                        names[min(len(names) - 1, idx)])
                    if len(singular) == 0:
                        singular = ident
                else:
                    singular = ident

            write_text(desc, origin,
                       comment=f"Description of effect '{singular}'")

    write_text(json.get("speed_name"), origin,
               comment=f"Speed name of effect '{effect_name}'")

    if type(json.get("apply_message")) is list:
        for msg in json["apply_message"]:
            write_text(msg[0], origin,
                       comment=f"Apply message of effect '{effect_name}'")
    elif type(json.get("apply_message")) is str:
        write_text(json["apply_message"], origin,
                   comment=f"Apply message of effect '{effect_name}'")

    write_text(json.get("remove_message"), origin,
               comment=f"Remove message of effect '{effect_name}'")

    write_text(json.get("death_msg"), origin,
               comment=f"Death message of effect '{effect_name}'")

    for msg in json.get("miss_messages", []):
        write_text(msg[0], origin,
                   comment=f"Miss message of effect '{effect_name}'")

    for msg in json.get("decay_messages", []):
        write_text(msg[0], origin,
                   comment=f"Decay message of effect '{effect_name}'")

    write_text(json.get("apply_memorial_log"), origin,
               context="memorial_male",
               comment=f"Male memorial apply log of effect '{effect_name}'")
    write_text(json.get("apply_memorial_log"), origin,
               context="memorial_female",
               comment=f"Female memorial apply log of effect '{effect_name}'")

    write_text(json.get("remove_memorial_log"), origin,
               context="memorial_male",
               comment=f"Male memorial remove log of effect '{effect_name}'")
    write_text(json.get("remove_memorial_log"), origin,
               context="memorial_female",
               comment=f"Female memorial remove log of effect '{effect_name}'")

    write_text(json.get("blood_analysis_description"), origin,
               comment=f"Blood analysis description of effect '{effect_name}'")

    for enchantment in json.get("enchantments", []):
        parse_enchant(enchantment, origin)
