from dataclasses import dataclass


@dataclass
class Message:
    comments: list
    origin: str
    format_tag: str
    context: str
    text: str
    text_plural: str


messages = dict()
occurrences = []
