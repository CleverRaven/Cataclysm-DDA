## JSON Autodocs

### What?
The imaginatively named JSON Autodocs is a utility to generate a human readable glossary of possible keys in a given list of JSON objects.  Further iterations would see to generate both a glossary and update specified documentation after any changes to the given JSON data. 

Autodoc parses a directory containing JSON files, then for each file it expands all JSON objects subject to inheritance rules (not yet...).  Once expanded, it flattens all nested structures and places them in a dictionary with values that explain what type the most common value is, along with examples and finally a guess at whether the key is required, based on whether it appears every time it is eligible to appear.  With this dictionary, "example.py" is generated, containing the dictionary's contents in a .md file which is intended to be readable by humans. (Eventually.)

### How?

Navigate to the json_autodocs directory and run:
```
    py autodoc.py <path of JSON directory> 
```

This will generate example.py, which is a glossary of the keys in individual JSON files.  (Directories are not read, yet.)


### Why?

Documentation is difficult, takes a lot of time and goes out of date quickly.  The idea here is to derive rules about correct JSON structure from the structure itself, and use that information to automatically update parts of Cataclysm: DDA's documentation.   If fully realized, it would be a tool that removes a fair bit of boilerplate editing of documentation based on changes, a possible tool for detecting divergent JSON objects and a basis for a beginner friendly GUI tool for creating new JSON entries into the game without having to wrangle with an IDE or refer to documentation for every value.  
