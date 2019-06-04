#!/usr/bin/env python3

import json
import argparse
import os

# no one references these, but we may want them again some day
OBSOLETE_TOPICS = [
    "TALK_DENY_GUARD", "TALK_FRIEND_UNCOMFORTABLE", "TALK_USE_ITEM", "TALK_GIVE_ITEM"
]


args = argparse.ArgumentParser(description="Confirm that every talk topic in every response in a "
                                           "dialogue JSON is defined, and that every defined talk "
                                           "topic is referenced in at least one response.")
args.add_argument("dialogue_json", nargs="+", action="store",
                  help="specify json file or files to validate.  Use 'data/json/npcs/* "
                  "data/json/npcs/Backgrounds/* data/json/npcs/refugee_center/*' to validate the "
                  "dialogue in the vanilla game.")
argsDict = vars(args.parse_args())

def get_dialogue_from_json():
    dialogue = []

    for path in argsDict.get("dialogue_json", []):
        if path == "data/json/npcs/TALK_TEST.json":
           continue
        if path.endswith(".json"):
            with open(path) as dialogue_file:
                dialogue += json.load(dialogue_file)
 
    return dialogue


def add_topic_by_chat(topics, topic_id):
    topics.setdefault(topic_id, {})
    topics[topic_id]["in_response"] = True


def add_topic_by_id(topics, topic_id):
    if topic_id in OBSOLETE_TOPICS:
        return
    topics.setdefault(topic_id, {})
    topics[topic_id]["valid"] = True


def add_topic_by_response(topics, response):
    topic_id = response.get("topic")
    if not topic_id:
        return
    topics.setdefault(topic_id, {})
    topics[topic_id]["in_response"] = True


def parse_response(topics, response):
    if response.get("topic"):
        add_topic_by_response(topics, response)
    else:
        success_r = response.get("success", {})
        add_topic_by_response(topics, success_r)
        failure_r = response.get("failure", {})
        add_topic_by_response(topics, failure_r)

    
def validate(dialogue):
    topics = {}
    # defined in src/npctalk.cpp
    defined_ids = [ "TALK_NONE", "TALK_DONE", "TALK_TRAIN", "TALK_HOW_MUCH_FURTHER" ]
    for topic_id in defined_ids:
        add_topic_by_id(topics, topic_id)

    # referenced in src/npctalk.cpp
    refered_ids = [ "TALK_WAKE_UP", "TALK_RADIO", "TALK_MISSION_DESCRIBE_URGENT",
                    "TALK_MISSION_DESCRIBE", "TALK_SHELTER", "TALK_SIZE_UP", "TALK_LOOK_AT",
                    "TALK_OPINION", "TALK_SHOUT", "TALK_STRANGER_AGGRESSIVE", "TALK_LEADER",
                    "TALK_TRAIN_START" ]
    for topic_id in refered_ids:
        add_topic_by_chat(topics, topic_id)

    for talk_topic in dialogue:
        if not isinstance(talk_topic, dict):
            continue
        if talk_topic.get("type") == "npc":
            chat = talk_topic.get("chat")
            if chat:
                add_topic_by_chat(topics, chat)
            continue
        if talk_topic.get("type") != "talk_topic":
            continue

        topic_id = talk_topic.get("id")
        if isinstance(topic_id, list):
            for loop_topic_id in topic_id:
                add_topic_by_id(topics, loop_topic_id)
        else:
            add_topic_by_id(topics, topic_id)
        for response in talk_topic.get("responses", []):
            parse_response(topics, response)
        repeated = talk_topic.get("repeat_responses", [])
        if isinstance(repeated, list):
            for repeated in talk_topic.get("repeat_responses", []):
                response = repeated.get("response", {})
                parse_response(topics, response)
        else:
            response = repeated.get("response", {})
            parse_response(topics, response)

    all_topics_valid = True
    for topic_id, topic_record in topics.items():
        if not topic_record.get("valid", False):
            all_topics_valid = False
            print("talk topic {} referenced in a response but not defined".format(topic_id))
        if not topic_record.get("in_response", False):
            all_topics_valid = False
            print("talk topic {} defined but not referenced in a response".format(topic_id))
        if topic_id in OBSOLETE_TOPICS:
            all_topics_valid = False
            print("talk topic {} referenced despite being listed as obsolete.".format(topic_id))
    if all_topics_valid:
        print("all topics referenced and defined.")


validate(get_dialogue_from_json())
