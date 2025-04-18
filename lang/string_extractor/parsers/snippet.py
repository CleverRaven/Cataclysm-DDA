from ..write_text import write_text, append_comment


def parse_snippet(json, origin):
    text = json["text"]
    if type(text) is not list:
        text = [text]
    comment = json.get("//") or []
    comment_snip = f"Snippet in category \"{json['category']}\""
    comment_name = f"Name of a snippet in category \"{json['category']}\""
    comment_snip = append_comment([comment_snip], comment)
    comment_name = append_comment([comment_name], comment)
    c_format = "schizophrenia" in json
    for snip in text:
        if type(snip) is str:
            write_text(snip, origin, comment=comment_snip, c_format=c_format)
        elif type(snip) is dict:
            if "name" in snip:
                write_text(snip["name"], origin, comment=comment_name,
                           c_format=False)
            write_text(snip["text"], origin, comment=comment_snip,
                       c_format=c_format)
