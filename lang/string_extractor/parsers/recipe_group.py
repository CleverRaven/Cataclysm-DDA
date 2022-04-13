from ..write_text import write_text


def parse_recipe_group(json, origin):
    if "recipes" in json:
        for recipe in json["recipes"]:
            write_text(recipe["description"], origin,
                       comment="Description for recipe group")
