TODO: document the "npc" structure, used to load NPC template

# Writing dialogues
Dialogues work like state machines, it starts with a certain topic (the NPC says something), the player character can respond (choosing one of several responses), and that responses sets the new talk topic. This goes on until the dialogue is finished, or the NPC turns hostile.

Note that it is perfectly fine to have a response that switches the topic back to itself.

Two topics are special:
- "TALK_DONE" ends the dialogue immediately.
- "TALK_NONE" goes to the previously talked about topic.

## Talk topics

Each topic consists of:
1. a topic id (e.g. "TALK_ARSONIST")
2. a dynamic line, spoken by the NPC.
3. a list of responses that can be spoken by the player character.

One can specify those topic in json. It is currently not possible to define the starting topic, so you have to add a response to some of the default topics (e.g. "TALK_STRANGER_FRIENDLY" or "TALK_STRANGER_NEUTRAL") or to topics that can be reached somehow.

Format:
```JSON
{
    "type": "talk_topic",
    "id": "TALK_ARSONIST",
    "dynamic_line": "What now?",
    "responses": [
        {
            "text": "I don't know either",
            "topic": "TALK_DONE"
        }
    ],
    "replace_built_in_responses": true
}
```
### type
Must always be there and must always be "talk_topic".

### id
The topic id, can be one of the build in topics, but if several talk topics *in json* have the same id, the last topic definition will override the previous ones.

### dynamic_line
This is the line spoken by the NPC. It can be a simple string, or an array of strings. If it's an array, a line will be chosen randomly every time the NPC needs it. Each entry has the same probability. It is optional, if it's not defined and this topic has the same id as a built in topic, the `dynamic_line` from that built in topic will be used. Otherwise the NPC will say nothing.

### response
An array with possible responses, it must not be empty. Each entry must be a response object, see below.

### replace_built_in_responses
A boolean (optional, default is `false`) that defines whether to dismiss the build in responses for that topic. If there are no built in responses, this won't do anything. If `true`, the built in responses are ignored and only those from this definition in json are used. If `false`, the responses from json are used *and* the built in responses (if any).

---

## Responses
A response contains at least a text, which is display to the user and "spoken" by the player character (its content has no meaning for the game) and a topic to which the dialogue will switch to. It can also have a trial object which can be used to either lie, persuade or intimidate the NPC, see `npctalk.cpp` for details. There can be different results, used either when the trial succeeds and when it fails.

Format:
```JSON
{
    "text": "I, the player, say to you...",
    "trial": {
        "type": "PERSUADE",
        "difficulty": 10
    },
    "success": {
        "topic": "TALK_DONE",
        "opinion": {
            "trust": 0,
            "fear": 0,
            "value": 0,
            "anger": 0,
            "owed": 0,
            "favors": 0
        }
    },
    "failure": {
        "topic": "TALK_DONE"
    }
}
```

Alternatively a short format:
```JSON
{
    "text": "I, the player, say to you...",
    "topic": "TALK_WHATEVER"
}
```
The short format is equivalent to (an unconditional switching of the topic):
```JSON
{
    "text": "I, the player, say to you...",
    "trial": {
        "type": "NONE"
    },
    "success": {
        "topic": "TALK_WHATEVER"
    }
}
```


### text
Will be shown to the user, no further meaning.

### trial
Optional, if not defined, "NONE" is used. Otherwise one of "NONE", "LIE", "PERSUADE" or "INTIMIDATE". If "NONE" is used, the `failure` object is not read, otherwise it's mandatory.
The `difficulty` is only required if type is not "NONE" and specifies the success chance in percent (it is however modified by various things like mutations).

### success and failure
Both objects have the same structure. `topic` defines which topic the dialogue will switch to. `opinion` is optional, if given it defines how the opinion of the NPC will change. The given values are *added* to the opinion of the NPC, they are all optional and default to 0.

The `failure` object is used if the trial fails, the `success` object is used otherwise.



