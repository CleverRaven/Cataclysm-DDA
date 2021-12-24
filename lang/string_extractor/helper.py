import re


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


tag_pattern = re.compile(r'^<[a-z0-9_]*>$')


def is_tag(text):
    return tag_pattern.match(text)
