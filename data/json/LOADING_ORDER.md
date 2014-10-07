# JSON Loading Order #

All files here in data/json are read eventually, but the order in which they're
read can be important for objects with dependencies on other kinds of objects
(e.g. recipes depend on skills). Ensuring the proper loading order will prevent
surprises that, most often, manifest as crash-to-desktop with segfault (a very
bad thing).

The way Cataclysm finds and loads json files is by running a breadth-first
search in the tree data/json/. This means data/json/whatever.json will
**always** be read before data/json/subdir/whatever.json. This tells us how to
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

## Same-depth loading order ##

Note that, when files are at the same depth (i.e. all in data/json/), the order
they will be read in is non-deterministic. Usually, under Windows, they will be
alphabetically ordered, but this is not true for Linux or Mac. Therefore, *be
absolutely sure* that your objects at the same depth do not depend on each
other.

Otherwise, even if on Windows you get `data/json/assets.json` before
`data/json/bullets.json`, on (some) Linux machines this will not be true. The
correct thing to do (assuming bullets.json depends on assets.json) is to ensure
that you actually have a subdirectory in there --
`data/json/bullets/bullets.json`.

