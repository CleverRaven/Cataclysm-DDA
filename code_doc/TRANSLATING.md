Info on translating Cataclysm-DDA into another language.


TRANSLATORS
===========

If there is already a translation available for your language,
and you just want to update or improve it,
you can edit the .po file for your language,
found inside the "lang/po" subdirectory.

After editing this file,
if you are running the dev version and want to test your changes,
perform step 4 from the maintainers section below.

If there is no translation yet,
the .po file will need to be generated.
You can do this yourself by following the instructions below,
or ask one of the CleverRaven devs to do it for you.

Specific instructions and notes for translators can be found in
"lang/translation_notes".
General notes for all translators are in
"lang/translation_notes/README_all_translators.txt".
Notes specific to a language may be stored as "<lang_id>.txt",
for exmple "lang/translation_notes/de_DE.txt".

When you are done translating,
submit the changes back to CleverRaven for inclusion in Cataclysm-DDA.


MAINTAINERS
===========


Step 1: Extract the translatable strings
----------------------------------------

First all the translatable strings in the source code need to be collated.

To do this, run the `lang/update_pot.sh` script.
It requires that you have both Python and the gettext utilities installed.

    lang/update_pot.sh

This needs to be done every time translatable strings are added or modified.
It will create or update the file `lang/po/cataclysm-dda.pot`.
All of the translations depend on this file.


Step 2(a): Initialize each language file
----------------------------------------

If we're starting a new translation from scratch,
we have to initialize the translation file.
In this example the translation is into New Zealand English (en_NZ).
For other languages change `en_NZ` to the relevant language identifier.

    msginit -l en_NZ.UTF-8 -o lang/po/en_NZ.po -i lang/po/cataclysm-dda.pot


Step 2(b): Update an already existing language file
---------------------------------------------------

If we just want to update a translation,
we'll want to keep all the messages that have already been translated.
In this case we use `msgmerge` in stead of `msginit`.

    msgmerge -F -U lang/po/en_NZ.po lang/po/cataclysm-dda.pot


Step 2(c): Update all the .po files at once
-------------------------------------------

To update the .po file for every language at once,
use the `lang/merge_po.sh` script.

    lang/merge_po.sh

This will run the above `msgmerge` command for each file in `lang/po/*.po`.


Step 3: Translate
-----------------

Now open the .po file in your favorite editor and translate!
Detailed instructions for translating can be found in
`lang/translation_notes/README_all_translators.txt`.


Step 4(a): Compile a single .po file
------------------------------------

If it is a new translation,
you will need to create a subdirectory in `lang/mo` for it,
and then a subdirectory called `LC_MESSAGES` inside that.
For example:

    mkdir -p lang/mo/en_NZ/LC_MESSAGES

Now run the `msgfmt` program to compile the translations for use in game.

    msgfmt -f -c -o lang/mo/en_NZ/LC_MESSAGES/cataclysm-dda.mo lang/po/en_NZ.po


Step 4(b): Compile all the .po files
------------------------------------

To compile all the .po files at once,
use the lang/compile_mo.sh script.

    lang/compile_mo.sh

This runs the above `mkdir` and `msgfmt` commands for all available .po files.



Testing your changes in game
============================

The game has no menu to change language,
so you need to manually set the locale.
This is a different process depending on your OS.

Note: The locale you set doesn't have to be an exact match.
For instance, to use the `de_DE.po` translation,
setting your locale to `de_DE.UTF8` will work fine.

Arch Linux
----------

Step 1: Ensure the locale is enabled

Edit `/etc/locale.gen` to include your desired locale(usually a matter of uncommenting),
then run `locale-gen`

Step 2: Set the locale in your current terminal window and run cataclysm

```bash
export LANG=mylocale
./cataclysm
```
