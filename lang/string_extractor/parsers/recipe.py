from ..write_text import write_text


def result_hint(json):
    hint = json.get("result", json.get("result_eocs", None))
    if type(hint) is list:
        hint = ", ".join(hint)
    if hint is None:
        raise Exception("no result hint for recipe translation,"
                        " needs 'result' or 'result_eocs' defined")
    return hint


def parse_recipe(json, origin):
    if "book_learn" in json and type(json["book_learn"]) is dict:
        for book in json["book_learn"]:
            if "recipe_name" in json["book_learn"][book]:
                write_text(json["book_learn"][book]["recipe_name"], origin,
                           comment="Recipe name learnt from book")
    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of recipe crafting \"{}\""
                   .format(result_hint(json)))
    if "blueprint_name" in json:
        write_text(json["blueprint_name"], origin,
                   comment="Blueprint name of recipe crafting \"{}\""
                   .format(result_hint(json)))
