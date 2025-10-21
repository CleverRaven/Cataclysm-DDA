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
    for snip in text:
        if type(snip) is str:
            write_text(snip, origin, comment=comment_snip)
        elif type(snip) is dict:
            if "name" in snip:
                write_text(snip["name"], origin, comment=comment_name)
            write_text(snip["text"], origin, comment=comment_snip)
