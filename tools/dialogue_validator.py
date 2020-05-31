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
                                           "topic is referenced in at least one response.\n"
                                           "Also checks that every dialogue path reaches "
                                           "TALK_DONE and every topic is reachable from an NPC's"
                                           "starting topic.  Reports nothing on success.")
args.add_argument("dialogue_json", nargs="+", action="store",
                  help="specify json folder to validate.  The valdiator will walk the folder's "
                  "tree and validate all JSON files in it.  Use 'data/json/npcs/ "
                  "to validate the dialogue in the vanilla game.")
argsDict = vars(args.parse_args())

def get_dialogue_from_json():
    dialogue = []

    for arg_path in argsDict.get("dialogue_json", []):
        if arg_path.endswith("/"):
            arg_path = arg_path[:-1]
        for subdir_path, dirnames, filenames in os.walk(arg_path):
           for filename in filenames:
               path = subdir_path + "/" + filename
               if path == "data/json/npcs/TALK_TEST.json":
                   continue
               if path.endswith(".json"):
                   with open(path) as dialogue_file:
                       dialogue += json.load(dialogue_file)
 
    return dialogue


def add_topic_by_chat(topics, topic_id, topic_branches=None, this_ids=None):
    topics.setdefault(topic_id, {})
    if topic_branches is not None:
        topic_branches.setdefault(topic_id, {"responses": [], "ends": False, "parent": None})
    if this_ids is not None:
        this_ids.append(topic_id)
    topics[topic_id]["in_response"] = True


def add_topic_by_id(topics, topic_id, topic_branches=None, this_ids=None):
    if topic_id in OBSOLETE_TOPICS:
        return
    if topic_branches is not None:
        topic_branches.setdefault(topic_id, {"responses": [], "ends": False, "parent": None})
    if this_ids is not None:
        this_ids.append(topic_id)
    topics.setdefault(topic_id, {})
    topics[topic_id]["valid"] = True


def add_topic_by_response(topics, response, topic_branches=None, this_ids=None):
    topic_id = response.get("topic")
    if not topic_id:
        return
    topics.setdefault(topic_id, {})
    topics[topic_id]["in_response"] = True
    if not topic_branches:
        return
    for parent_id in this_ids:
        if topic_id == "TALK_DONE":
            topic_branches[parent_id]["ends"] = True
        elif topic_id == "TALK_NONE":
            for previous_id in topic_branches:
                if parent_id in topic_branches[previous_id]["responses"]:
                    topic_branches[parent_id]["responses"].append(previous_id)
        else:
            topic_branches[parent_id]["responses"].append(topic_id)


def parse_response(topics, response, topic_branches=None, this_ids=None):
    if response.get("topic"):
        add_topic_by_response(topics, response, topic_branches, this_ids)
    else:
        success_r = response.get("success", {})
        add_topic_by_response(topics, success_r, topic_branches, this_ids)
        failure_r = response.get("failure", {})
        add_topic_by_response(topics, failure_r, topic_branches, this_ids)

    
def validate(dialogue):
    topics = {}
    topic_branches = {
        "TALK_TRAIN": {"responses": ["TALK_TRAIN_START"], "ends": False, "parent": None},
        "TALK_HOW_MUCH_FURTHER": {"responses": ["TALK_DONE"], "ends": True, "parent": None},
        "TALK_SEDATED": {"responses": ["TALK_DONE"], "ends": True, "parent": None},
    }
    # defined in src/npctalk.cpp
    defined_ids = [ "TALK_NONE", "TALK_DONE", "TALK_TRAIN", "TALK_HOW_MUCH_FURTHER",
                    "TALK_SEDATED" ]
    for topic_id in defined_ids:
        add_topic_by_id(topics, topic_id)

    # referenced in src/npctalk.cpp
    refered_ids = [ "TALK_WAKE_UP", "TALK_RADIO", "TALK_MISSION_DESCRIBE_URGENT",
                    "TALK_MISSION_DESCRIBE", "TALK_SHELTER", "TALK_SIZE_UP", "TALK_LOOK_AT",
                    "TALK_OPINION", "TALK_SHOUT", "TALK_STRANGER_AGGRESSIVE", "TALK_LEADER",
                    "TALK_TRAIN_START", "TALK_STOLE_ITEM", "TALK_SEDATED" ]
    for topic_id in refered_ids:
        add_topic_by_chat(topics, topic_id)

    # TALK_TRAIN_START is technically not a start id
    start_topics = [ "TALK_RADIO", "TALK_LEADER", "TALK_FRIEND", "TALK_STOLE_ITEM",
                     "TALK_MISSION_DESCRIBE_URGENT", "TALK_SEDATED", "TALK_WAKE_UP",
                     "TALK_MUG", "TALK_STRANGER_AGGRESSIVE", "TALK_STRANGER_SCARED",
                     "TALK_STRANGER_WARY", "TALK_STRANGER_FRIENDLY", "TALK_STRANGER_NEUTRAL",
                     "TALK_SHELTER", "TALK_CAMP_OVERSEER" ]
    for talk_topic in dialogue:
        if not isinstance(talk_topic, dict):
            continue
        if talk_topic.get("type") == "npc":
            chat = talk_topic.get("chat")
            if chat:
                start_topics.append(chat)
                add_topic_by_chat(topics, chat, topic_branches)
            continue
        if talk_topic.get("type") != "talk_topic":
            continue

        topic_id = talk_topic.get("id")
        this_ids = []
        if isinstance(topic_id, list):
            for loop_topic_id in topic_id:
                add_topic_by_id(topics, loop_topic_id, topic_branches, this_ids)
        else:
            add_topic_by_id(topics, topic_id, topic_branches, this_ids)
        for response in talk_topic.get("responses", []):
            parse_response(topics, response, topic_branches, this_ids)
        repeated = talk_topic.get("repeat_responses", [])
        if isinstance(repeated, list):
            for repeated in talk_topic.get("repeat_responses", []):
                response = repeated.get("response", {})
                parse_response(topics, response, topic_branches, this_ids)
        else:
            response = repeated.get("response", {})
            parse_response(topics, response, topic_branches, this_ids)

    all_topics_valid = True
    for topic_id, topic_record in topics.items():
        if not topic_record.get("valid", False):
            all_topics_valid = False
            if topic_id in start_topics:
                print("talk topic {} referenced in an NPC chat but not defined".format(topic_id))
            else:
                print("talk topic {} referenced in a response but not defined".format(topic_id))
        if not topic_record.get("in_response", False):
            all_topics_valid = False
            print("talk topic {} defined but not referenced in a response".format(topic_id))
        if topic_id in OBSOLETE_TOPICS:
            all_topics_valid = False
            print("talk topic {} referenced despite being listed as obsolete.".format(topic_id))

    no_change = False
    passes = 0
    while not no_change and passes < len(topic_branches):
        no_change = True
        passes += 1
        for topic_id in topic_branches:
            branch_record = topic_branches[topic_id]
            if branch_record["ends"] or not topics[topic_id].get("valid", False):
                continue
            for response_id in branch_record["responses"]:
                if topic_branches.get(response_id, {}).get("ends", False):
                    # useful debug statement that I'm leaving here
                    # print("pass {} {} terminates by {}".format(passes, topic_id, response_id))
                    branch_record["ends"] = True
                    no_change = False
                    break

    no_change = False
    passes = 0
    while not no_change and passes < len(topic_branches):
        no_change = True
        passes += 1
        for topic_id in topic_branches:
            # skip undefined topics
            if not topics[topic_id].get("valid", False):
                continue
            branch_record = topic_branches[topic_id]
            if branch_record["parent"] not in start_topics and topic_id in start_topics:
                branch_record["parent"] = topic_id
                no_change = False
            if not branch_record["parent"]:
                continue
            for response_id in branch_record["responses"]:
                response_record = topic_branches.get(response_id, {})
                if response_record.get("parent", False) in start_topics:
                    continue
                response_record["parent"] = branch_record["parent"]
                no_change = False

    # print(json.dumps(topic_branches, indent=2))
    skip_topics = ["TALK_DONE", "TALK_SIZE_UP", "TALK_LOOK_AT", "TALK_OPINION", "TALK_SHOUT"]

    for topic_id in topic_branches:
        if topic_id in skip_topics or not topics[topic_id].get("valid"):
            continue
        branch_record = topic_branches[topic_id]
        if not branch_record["ends"]:
            print("{} does not reach TALK_DONE".format(topic_id))
        if not branch_record["parent"] in start_topics:
            print("no path from a start topic to {}".format(topic_id))

validate(get_dialogue_from_json())
