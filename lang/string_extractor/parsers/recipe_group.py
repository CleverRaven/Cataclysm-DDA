from ..write_text import write_text


def parse_recipe_group(json, origin):
    for recipe in json.get("recipes", []):
        write_text(recipe["description"], origin,
                   comment="Description for recipe group")
