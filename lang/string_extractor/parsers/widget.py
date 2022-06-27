from ..write_text import write_text


def parse_widget(json, origin):
    id = json["id"]
    if "label" in json:
        write_text(json["label"], origin,
                   comment="Label of UI widget \"{}\"".format(id))
    if "strings" in json:
        for string in json["strings"]:
            write_text(string, origin,
                       comment="Text in UI widget \"{}\"".format(id))
    if "phrases" in json:
        for phrase in json["phrases"]:
            comment = "Text in portion of UI widget \"{}\"".format(id)
            if "text" in phrase:
                write_text(phrase["text"], origin, comment=comment)
