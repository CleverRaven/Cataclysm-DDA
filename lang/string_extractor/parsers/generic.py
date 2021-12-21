from ..helper import get_singular_name
from .use_action import parse_use_action
from ..write_text import write_text


def parse_generic(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Item name",
                   plural=True, c_format=False)
    elif "id" in json:
        name = json["id"]

    if "description" in json:
        write_text(json["description"], origin, c_format=False,
                   comment="Description of \"{}\"".format(name))

    if "use_action" in json:
        parse_use_action(json["use_action"], origin, name)

    for cname in json.get("conditional_names", []):
        write_text(cname["name"], origin,
                   comment="Conditional name for \"{}\" when {} matches {}"
                   .format(name, cname["type"], cname["condition"]),
                   plural=True)

    if "snippet_category" in json and type(json["snippet_category"]) is list:
        # snippet_category is either a simple string (the category ident)
        # which is not translated, or an array of snippet texts.
        for entry in json["snippet_category"]:
            if type(entry) is str:
                write_text(entry, origin,
                           comment="Snippet of item \"{}\"".format(name))
            elif type(entry) is dict:
                write_text(entry["text"], origin,
                           comment="Snippet of item \"{}\"".format(name))

    if "seed_data" in json:
        write_text(json["seed_data"]["plant_name"], origin,
                   comment="Plant name of seed \"{}\"".format(name))

    if "revert_msg" in json:
        write_text(json["revert_msg"], origin,
                   comment="Dying message of tool \"{}\"".format(name))


#     if "message" in item:
#         writestr(outfile, item["message"], format_strings=True,
#                     comment="Message for {} '{}'".format(object_type, name),
#                     **kwargs)
#         wrote = True
#     if "messages" in item:
#         for message in item["messages"]:
#             writestr(outfile, message, **kwargs)
#             wrote = True
#     if "valid_mod_locations" in item:
#         for mod_loc in item["valid_mod_locations"]:
#             writestr(outfile, mod_loc[0], **kwargs)
#             wrote = True
