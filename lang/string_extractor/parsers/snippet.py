from ..write_text import write_text


def parse_snippet(json, origin):
    text = json["text"]
    if type(text) is not list:
        text = [text]
    comment = ["Snippet in category \"{}\"".format(json["category"])]
    if "//" in json:
        comment.append(json["//"])
    c_format = "schizophrenia" in json
    for snip in text:
        if type(snip) is str:
            write_text(snip, origin, comment=comment, c_format=c_format)
        elif type(snip) is dict:
            write_text(snip["text"], origin, comment=comment,
                       c_format=c_format)
