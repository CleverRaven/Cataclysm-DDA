from ..write_text import write_text


def parse_material(json, origin):
    write_text(json["name"], origin, comment="Name of material")

    write_text(json.get("bash_dmg_verb"), origin,
               comment=[
                   "Bash damage verb of material",
                   "ex. 'Your helm is cracked!'"])

    write_text(json.get("cut_dmg_verb"), origin,
               comment=[
                   "Cut damage verb of material",
                   "ex. 'Your backpack is scratched!'"])

    for adj in json.get("dmg_adj", []):
        write_text(adj, origin,
                   comment=[
                       "Damage adjective of material",
                       "ex. 'dented cuirass'"])
