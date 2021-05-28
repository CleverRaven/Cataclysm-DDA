import autodoc_util
import sys
import os

'''
    Generates autodocs, a glossary of keys in a directory of JSONs.

    usage: py autodoc.py <path> 
        path should a directory of JSON files.

    Creates example.md, the human readable form of the aformentioned glossary of keys.

    TODO: so much.  Considering the size of some individual json files, we'll likely want to break this down into
    a directory of glossaries, and autogenerate a table of contents.  Also definitely can improve in how sub-keys
    are displayed.
'''


def autodoc_generator(output, explored_JSON):
    '''
        autodoc_generator creates a .md file that acts as a glossary for keys in a JSON file.  Required fields display
        a codeblock with the expected value, an example of the expected value and the word "Required" or "Optional" depending
        on whether those keys appear in every entry.  Sub-keys and their "Required" tag are based on how often they show
        up when their parent is invoked.  TODO:  Make it better. 
    '''
    ## so first, we need to corral our data into a string.
    if output not in os.listdir():
        os.mkdir(output, mode=0o777)
    for lot, entry in explored_JSON.items():
        generatedString = f"\n## {lot}:\n\n" 
        last_key = ""
        for key, value in entry.items():
            if last_key == "":
                generatedString += f"### {key} \n ```\n {value} \n```\n"
                last_key = key
                continue
            if len(key.split(":")) == 1:
                if len(last_key.split(":")) != 1:
                    for _ in range(len(last_key.split(":", -1))):
                        generatedString += f"\n\n </details>\n</summary>\n"
                    generatedString += f"\n ### {key} \n\n ```\n {value} \n```\n\n"
                else:
                    generatedString += f"\n ### {key} \n\n ```\n {value} \n```\n\n"
                last_key = key
                continue
            split_key = key.split(":", -1)
            if last_key == ":".join(split_key[:(len(split_key)-1)]):
                generatedString += f"\n <details> \n <summary> children of {last_key} </summary> \n\n ### {key} \n\n ```\n {value} \n```\n\n"
            else: 
                generatedString += f"\n\n ### {key} \n\n ```\n {value} \n```\n\n"
            last_key = key

        with open(f"{output}/{lot}.md", "w", encoding='utf8') as file:
            print(generatedString, file=file)

def main(args):
    '''
        run 'py autodoc.py <path>' (or equivalent) to generate an example.md output.
    '''
    ## do testing stuff
    raw_data = autodoc_util.load_data(args[0])
    explored = {}
    for entry, value in raw_data.items():
        data1 = autodoc_util.expand_data(value)
        data = autodoc_util.flatten_JSON_list(data1)
        explored[entry] =  autodoc_util.explore_JSON(data)
    autodoc_generator("example", explored)
    
if __name__ == "__main__":
    main(sys.argv[1:])