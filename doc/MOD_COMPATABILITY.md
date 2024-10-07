# Mod Compatability

## Summary

Mods are capable of dynamically loading directories based on if other mods are loaded in the world.  This will prevent file contents within the mod_interactions folder from being read unless they are part of a folder named after a loaded mod's id.

## Guide

In order to dynamically load mod content, files must be placed within subdirectories named after other mod ids (capitalization is checked) within the mod_interactions folder.

Example:
Mod 1: Mind Over Matter (id:mindovermatter)
Mod 2: Xedra Evolved (id:xedra_evolved)

If Xedra wishes to load a file only when Mind Over Matter is active: mom_compat_data.json, it must be located as such:
Xedra_Evolved/mod_interactions/mindovermatter/mom_compat_data.json

Files located within the mod_interactions folders are always loaded after other mod content for every mod is loaded.

## Limitations

Currently, this functionality only loads / unloads files based on if other mods are active for the particular world.  It does not suppress any other warnings beyond this function.

In particular, when designing mod compatability content an author will likely want to redefine certain ids to have new definitions, flags, etc.  If attempting to do this within the same overall mod folder, this will throw a duplicate definition error.  To get around this, instead of a mod redefining its own content within its own mod_interactions folder, the author should redefine its own content using the other mods mod_interaction folder.

Example:
Mod 1: Mind Over Matter (id:mindovermatter)
Mod 2: Xedra Evolved (id:xedra_evolved)

If xedra wants an item to have extra damage while Mind Over Matter is loaded, the author should place the new definition in the following:
MindOverMatter/mod_interactions/xedra_evolved/xedra_compat_data.json

TODO: remove this limitation entirely by adjusting the check to take into account the mod_interaction id source as well