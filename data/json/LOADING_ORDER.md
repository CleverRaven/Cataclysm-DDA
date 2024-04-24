# JSON Loading Order #

All files here in data/json are read eventually, but the order in which they're
read can be important for objects with dependencies on other kinds of objects
(e.g. recipes depend on skills). Ensuring the proper loading order will prevent
surprises that, most often, manifest as crash-to-desktop with segfault (a very
bad thing).

The way Cataclysm finds and loads json files is by running a breadth-first
search in the tree data/json/. This means `data/json/whatever.json` will
**always** be read before `data/json/subdir/whatever.json`. This tells us how to
ensure dependency loading order.

For instance, if you have scenarios that depend on professions that depend on
skills, you'll want a directory structure such as the following:

```
data/json/
  skills.json
  professions/
    professions.json
    scenarios/
      scenarios.json
```

Which results in a loading order of: `skills.json` then `professions.json` and
then `scenarios.json`.

## Same-depth loading order ##

Note that, when files (or directories) are at the same depth
(i.e. all in `data/json/`), they will be read in lexical order, which is
more or less equivalent to alphabetical order for file names that use only
ascii characters. For UTF-8 or otherwise non-ascii file names, the names will be
ordered by codepoint.
