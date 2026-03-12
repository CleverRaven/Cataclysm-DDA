from ..write_text import write_text


def parse_recipe_category(json, origin):
    ident = json["id"]
    if ident == 'CC_NONCRAFT':
        return

    cat_name = ident.split("_")[1]
    write_text(cat_name, origin, comment="Crafting recipes category")

    for subcat in json.get("recipe_subcategories", []):
        if subcat == 'CSC_ALL':
            write_text("ALL", origin,
                       comment="Crafting recipes subcategory 'all'")
        else:
            subcat_name = subcat.split('_')[-1]
            write_text(subcat_name, origin,
                       comment=f"Crafting recipes subcategory of '{cat_name}'")
