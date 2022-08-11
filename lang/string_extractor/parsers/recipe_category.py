from ..write_text import write_text


def parse_recipe_category(json, origin):
    cid = json["id"]
    if cid == 'CC_NONCRAFT':
        return
    cat_name = cid.split("_")[1]
    write_text(cat_name, origin, comment="Crafting recipes category name")

    for subcat in json.get("recipe_subcategories", []):
        if subcat == 'CSC_ALL':
            write_text("ALL", origin,
                       comment="Crafting recipes subcategory \"all\"")
        else:
            subcat_name = subcat.split('_')[2]
            write_text(subcat_name, origin,
                       comment="Crafting recipes subcategory of \"{}\" cat."
                       .format(cat_name))
