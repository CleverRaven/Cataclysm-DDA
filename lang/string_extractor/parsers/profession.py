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

    write_text(name_male, origin, context="profession_male",
               comment="Profession name for male")
    write_text(name_female, origin, context="profession_female",
               comment="Profession name for female")

    desc_male = ""
    desc_female = ""

    if "description" in json:
        if type(json["description"]) is dict:
            if "male" in json["description"]:
                desc_male = json["description"]["male"]
                desc_female = json["description"]["female"]
            elif "str" in json["description"]:
                desc_male = desc_female = json["description"]["str"]
        else:
            desc_male = desc_female = json["description"]

        write_text(desc_male, origin, context="prof_desc_male",
                   comment=f"Description of profession '{name_male}'"
                   " for male")
        write_text(desc_female, origin, context="prof_desc_female",
                   comment=f"Description of profession '{name_female}'"
                   " for female")
