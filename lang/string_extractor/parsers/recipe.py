from .effect import parse_effect_on_condition
from ..write_text import write_text


def extract_eoc_id(json):
    if type(json) is str:
        return json  # eoc id
    elif type(json) is dict and "id" in json:
        return json.get("id")  # eoc defined inline
    else:
        raise Exception("result_eocs member missing id")


def result_hint(json):
    hint = json.get("result", json.get("result_eocs"))

    if not hint:
        return False

    if type(hint) is str:
        return hint
    elif type(hint) is list:
        return ", ".join(map(extract_eoc_id, hint))
    else:
        raise Exception("recipe's result_eocs has unsupported form")


def parse_recipe(json, origin):
    hint = result_hint(json)
    if not hint:
        return

    write_text(json.get("name"), origin,
               comment=f"Custom name for recipe crafting '{hint}'")
    write_text(json.get("description"), origin,
               comment="Description of crafting recipe")

    write_text(json.get("blueprint_name"), origin,
               comment="Blueprint name of crafting recipe")

    if type(json.get("book_learn")) is dict:
        for book in json["book_learn"].values():
            write_text(book.get("recipe_name"), origin,
                       comment="Recipe name learnt from book")

    for eoc in json.get("result_eocs", []):
        parse_effect_on_condition(eoc, origin,
                                  f"result of recipe crafting '{hint}'")
