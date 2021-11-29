def get_singular_name(name):
    if type(name) is dict:
        if "str_sp" in name:
            return name["str_sp"]
        elif "str" in name:
            return name["str"]
        else:
            raise Exception("Cannot find singular name in {}".format(name))
    elif type(name) is str:
        return name
    else:
        raise Exception("Cannot find singular name in {}".format(name))
