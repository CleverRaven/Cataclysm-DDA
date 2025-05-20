from ..write_text import write_text


def parse_profession(json, origin):
    name_male = ""
    name_female = ""
    if "name" not in json:
        name_male = name_female = json["id"]
    elif type(json["name"]) is dict:
        name_male = json["name"]["male"]
        name_female = json["name"]["female"]
    elif type(json["name"]) is str:
        name_male = name_female = json["name"]

    desc_male = ""
    desc_female = ""
    has_description = "description" in json
    if has_description:
        if type(json["description"]) is dict:
            if "male" in json["description"]:
                desc_male = json["description"]["male"]
                desc_female = json["description"]["female"]
            elif "str" in json["description"]:
                desc_male = desc_female = json["description"]["str"]
        else:
            desc_male = desc_female = json["description"]

    write_text(name_male, origin, context="profession_male",
               comment="Profession name for male")
    if has_description:
        write_text(desc_male, origin, context="prof_desc_male",
                   comment="Description of profession \"{}\" for male".
                   format(name_male))

    write_text(name_female, origin, context="profession_female",
               comment="Profession name for female")
    if has_description:
        write_text(desc_female, origin, context="prof_desc_female",
                   comment="Description of profession \"{}\" for female".
                   format(name_female))
