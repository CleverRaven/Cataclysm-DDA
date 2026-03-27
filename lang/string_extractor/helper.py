def get_singular_name(name):
    if type(name) is dict:
        if name.get("str_sp"):
            return name["str_sp"]
        elif name.get("str"):
            return name["str"]
        elif name.get("name"):
            return get_singular_name(name["name"])
        elif name.get("id"):
            return name["id"]
        elif name.get("abstract"):
            return name["abstract"]
    elif type(name) is str:
        return name
    elif type(name) is list:
        if len(name) > 0:
            return get_singular_name(name[0])
    return "???"
