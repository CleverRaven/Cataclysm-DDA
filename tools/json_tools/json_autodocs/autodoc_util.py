import os
import json
## import ctags
from collections import Counter
from typing import List

def load_data(path):
    '''
    Takes a path and returns a dict of all the JSON information contained within the directory, with the keys
    representing the files and values representing their raw JSON information.  Simple prototype.  TODO: Read
    directories.
    '''
    data = {}
    for filename in os.listdir(path):
        if filename.endswith(".json") == True and filename.startswith("obsolete") == False and filename.startswith('test') == False and filename.startswith('regional') == False:
            with open(os.path.join(path, filename), encoding='utf8') as raw:
                data[filename] = json.loads(raw.read())
    return data


def expand_data(JSON_list):
    '''
        Takes a list of JSON objects consisting of all similar types, and removes all copy-from keys, replacing
        it with all the values to be copied from the abstract or parent object.  The JSON_List is assumed to 
        contain the parent items or abstracts that the original object is copying from.  TODO: Proportions and
        other JSON inheritance.
    '''
    for object in JSON_list:
        parent_object = object
        if 'copy-from' in object.keys():
            ## maybe use Ctags if I can figure it out
            ##Ctags_file.find(value, "find", ctags.TAG_IGNORECASE | ctags.TAG_FULLMATCH)
            defined = False
            for compare_object in JSON_list:
                for cKey, cValue in compare_object.items():
                    ## only two keys that can be used for inheritance, I think.
                    if cKey == 'abstract' or cKey == 'id': 
                        if cValue == object['copy-from']:
                            parent_object = compare_object
                            defined = True
                            break
                if defined == True:
                    break
            if defined == False:
                print("Error, couldn't find parent.")
            object.pop('copy-from')
        ## Stopgap comment remover.  I know //~ is used for some localization, need to check for other pertinent 
        ## keys that might get confused with "//".
        for i in range(11):
            if "//" in object.keys() and i == 0:
                object.pop("//")
            if "//" + str(i) in object.keys():
                object.pop("//" + str(i))
        ## So now we have the parent_object, if it exists.  If it isn't the same as the original object,
        ## then we want take out the copy-from key and replace those entries with
        if parent_object != object:
            object = parent_object | object
        
        ## No one would be sick enough to copy-from a item that had already been copied-from something else right?  
        # Research required.

    ## all done, we can return the expanded JSON_List
    return JSON_list

def flat_recursion(key, value):
    '''
        Takes a key and value from a dictionary and checks whether the value is a list or dictionary,
        if so, convert the values into a new key value pair, recursively checking the value until we
        return a new 'flat' dictionary with all expandable values enumerated as a key.  Delimiters are 
        ":" for lists and ";" for dicts.  TODO: Find ideal delimiters, to avoid problems parsing keys that
        contain ":" and ";".
    '''
    flat_dict = {}
    if type(value) == list:
        index = -1
        for item in value:
            ## Index was getting flagged as unreachable when it was at the end of this line,
            ## so for safety we'll just start it at -1 and immediately add 1 to start at index zero.
            index += 1
            new_key = key + ":" + str(index)
            flat_dict = flat_dict | flat_recursion(new_key, item)
    elif type(value) == dict:
        for key2, value2 in value.items():
            new_key = key + ";" + key2
            flat_dict = flat_dict | flat_recursion(new_key, value2)
    else:
        flat_dict[key] = value

    return flat_dict    


def flatten_JSON_list(JSON_list):
    '''
        Takes a list of JSON objects and flattens all nested values into their simplest key:value form.  Simpifies
        arrays into a simple ":#" notation and returns the list of JSON objects in alphabetical order.  TODO: Better
        delimiter than ":#", use a more representative value for an array (At the moment selects the first value and dumps the rest).
    '''
    new_list = []
    for i in range(len(JSON_list)):
        new_list.append({})
        for key, value in JSON_list[i].items():
            new_list[i] = new_list[i] | flat_recursion(key, value)

    ## Need to compare array keys more closely, right now we just take the first
    ## value of first array and call it a day.
    final_list = []
    index = 0
    for entry in new_list:
        final_list.append({})
        collapsables = []
        for key,value in entry.items():
            array_check = key.split(":", -1)
            if len(array_check) > 1:
                collapsables.append(key)

        for key in collapsables:
            split = key.split(":", -1)
            new_key = split[0]
            for i in range(1, len(split)):
                new_key += ":#" + split[i].lstrip('0123456789')

            if new_key not in entry.keys():
                entry[new_key] = entry[key]
            entry.pop(key)

        ## Ugh... always be sorting.
        new_keys = sorted(entry, key=str.lower)
        for raw_key in new_keys:
            ## for easier end parsing, with arrays now signified by ":#", replace all semis with full colons.
            key = raw_key.replace(";", ":")
            final_list[index][key] = entry[raw_key]
        index += 1

    return final_list

def clean_key(key, JSON_list):
    ## assume no one is passing empty lists
    value_type = type(JSON_list[0][key])
    for entry in JSON_list:
        if type(entry[key]) != value_type:
            return False
    return True


def example_value_generator(JSON_list):
    '''
        example value generator would read through the expanded JSON list, grabbing all possible values,
        then using Counter to sort possible values by frequency.  We use those possible values, and the typing 
        to provide a hint to the user on what the value should look like.
    '''
    ## return a sample for each key, along with its type. 
    example_dict = {}

    ## Well, let's round up our possible values
    possible_key_values = {}
    for entry in JSON_list:
        for key, value in entry.items():
            if key not in possible_key_values.keys():
                possible_key_values[key] = []
            possible_key_values[key].append(value)
    
    ## Counter good.  We use the most popular keys to provide a few example values.  This is a super
    ## naive approach, for stuff like numbers we might want to include a range, and for longer strings
    ## like descriptions we'd likely want to limit our examples.
    for key in possible_key_values.keys():
        freq_sort = Counter(possible_key_values[key])
        if len(freq_sort.most_common()) > 1:
            most_common = freq_sort.most_common(2)
            example_dict[key] = "{} value, examples: {}, {}".format(
                type(most_common[0][0]).__name__,
                most_common[0][0],
                most_common[1][0])
        else:
            most_common = freq_sort.most_common(1)
            example_dict[key] = "{} value, example: {}".format(
                type(most_common[0][0]).__name__,
                most_common[0][0])

    return example_dict

## Okay, we have a way to explore a single layer of a JSON and extract some relevant data.  Now we want to integrate this across
## every layer for each expanded JSON list.  After exploring, we want to return an example dict for every possible key value as well
## as a mandatory/optional list for every 'layer'.
def explore_JSON(flat_JSON_list):
    # So what does an explored JSON look like?  Well a dict of all eligible keys, with a list or tuple containing example values,
    # the mandatory or optional nature of the key.  This information can then be used to build
    # a model of the JSON data based entirely on the structure of the data.
    '''
        explore_JSON takes a flattened JSON list and returns a dictionary containing all possible keys for the entry, combined
        with a string that describes what value is expected for the key.  TODO: Value strings are very rough atm, and I'm very 
        certain I've got some redundant code sitting around.
    '''
    explored_JSON = {}

    ## All keys are mandatory until we rule them out.
    mandatory = set()
    optional = set()

    ## We want to track lineage, as well as counts for how many times a key is referenced, both as a standalone
    ## and as a parent.
    parent_child = {}
    counter = {}
    total_entries = len(flat_JSON_list)
    for entry in flat_JSON_list:
        ## silly me, counting things over and over again.  A key can only show up once per entry (...Right?).
        entry_keys_counted = set()

        if 'abstract' in entry.keys():
            total_entries -= 1
            continue
        ## Why did we sort in flat_expanded_JSON_list?  Makes doing this parent_child key thing a whole lot easier.
        for key in entry.keys():
            ## We need to check if a key is clean, and doesn't have multiple input types.
            if key not in parent_child:
                parent_child[key] = set()
            if key not in counter.keys():
                counter[key] = 0
            counter[key] += 1
            entry_keys_counted.add(key)
            split_key = key.split(":", -1)
            if len(split_key) > 1:
                child = key
                ## Get the lineage of the key.
                for i in range(len(split_key)-1, 0, -1):
                    parent = ":".join(split_key[:i])
                    if parent not in parent_child.keys():
                        parent_child[parent] = set()
                    if parent not in counter.keys():
                        counter[parent] = 0
                    if parent not in entry_keys_counted:     
                        counter[parent] += 1
                        entry_keys_counted.add(parent)
                    parent_child[parent].add(child)
                    child = parent

        mandatory |= parent_child.keys()

    ## Alright, now we have counts for every key, as well as a way to track child keys.  Now we just have to find out
    ## if they are optional.
    for key, _ in parent_child.items():
        ## Parentless
        if len(key.split(":", -1)) == 1:
            if counter[key] < total_entries:
                optional.add(key)
            continue

        split_key = key.split(":", -1)
        parent = ":".join(split_key[:len(split_key)-1])
        ## compare counter to parents counter
        if counter[key] < counter[parent]:
            optional.add(key)

    ## update mandatory
    mandatory = mandatory - optional

    examples = example_value_generator(flat_JSON_list)

    ## use examples, our parent_child dictionaries and our list of mandatory & optional
    ## lists to generate a readable string describing the key.  A Very Goofy prototype.
    for key in mandatory:
        if key in examples.keys():
            if len(parent_child[key]) == 0:
                explored_JSON[key] = f'{examples[key]}\n Required'
            else:
                clause = ""
                if key + ":#" in mandatory | optional:
                    clause += "list"
                else:
                    clause += "dict"
                explored_JSON[key] = f"{examples[key]} \n\n-OR-\n\n {clause}\n Required" 
            continue
        if key + ":#" in mandatory | optional:
            explored_JSON[key] = f'list see "{key}:#"\n Required'
        else:    
            explored_JSON[key] = f'dict see "{key}:" values\n Required' 

    for key in optional:
        if key in examples.keys():
            if len(parent_child[key]) == 0:
                explored_JSON[key] = f'{examples[key]}\n Optional'
            else:
                clause = ""
                if key + ":#" in mandatory | optional:
                    clause += f"list, see {key}:#"
                else:
                    clause += f"dict, see {key}:"
                explored_JSON[key] = f"{examples[key]} \n\n-OR-\n\n {clause}\n Optional"
            continue
    
        if key + ":#" in mandatory | optional:
            explored_JSON[key] = f'list see "{key}:#"\n Optional'
        else:    
            explored_JSON[key] = f'dict, see "{key}:" values\n Optional'

    ## Now with even more sorting.
    sorted_JSON = {}
    new_keys = sorted(explored_JSON, key=str.lower)
    for key in new_keys:
        sorted_JSON[key] = explored_JSON[key]

    return sorted_JSON
