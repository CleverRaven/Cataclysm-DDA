from ..write_text import write_text


def parse_widget(json, origin):
    id = json["id"]
    if "label" in json:
        write_text(json["label"], origin,
                   comment="Label of UI widget \"{}\"".format(id))
    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of UI widget \"{}\"".format(id))
    if "string" in json:
        write_text(json["string"], origin,
                   comment="Text in UI widget \"{}\"".format(id))
    if "phrases" in json:
        for phrase in json["phrases"]:
            comment = "Text in portion of UI widget \"{}\"".format(id)
            if "text" in phrase:
                write_text(phrase["text"], origin, comment=comment)
    if "default_clause" in json and "text" in json["default_clause"]:
        write_text(json["default_clause"]["text"], origin,
                   comment="Default clause of UI widget \"{}\"".format(id))
    if "clauses" in json:
        for clause in json["clauses"]:
            if "text" not in clause:
                continue
            write_text(clause["text"], origin,
                       comment="Clause text \"{}\" of UI widget \"{}\""
                       .format(clause["id"], id))
