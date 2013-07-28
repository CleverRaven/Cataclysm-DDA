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

When you are done translating,
submit the changes back to CleverRaven for inclusion in Cataclysm-DDA.


MAINTAINERS
===========


Step 1: Extract the translatable strings
----------------------------------------

First we have to extract the translatable strings from the .json files.
The python script lang/extract_json_strings.py will do this for us,
storing them in an intermediate form in lang/json for "xgettext" to read.

    python lang/extract_json_strings.py

Now we can use xgettext to collate all the translatable strings,
and store them in the file lang/po/cataclysm-dda.pot.

This needs to be done every time translatable strings are added or modified.
All of the translations depend on this file.

    xgettext -d cataclysm-dda -F -o lang/po/cataclysm-dda.pot --keyword=_ *.cpp *.h lang/json/*.py


Step 2(a): Initialize each language file
----------------------------------------

If we're starting a new translation from scratch,
we have to initialize the translation file.
In this example the translation is into New Zealand English (en_NZ).
For other languages change "en_NZ" to the relevant language identifier.

    msginit -l en_NZ.utf8 -o lang/po/en_NZ.po -i lang/po/cataclysm-dda.pot


Step 2(b): Update an already existing language file
---------------------------------------------------

If we just want to update a translation,
we'll want to keep all the messages that have already been translated.
In this case we use "msgmerge" in stead of "msginit".

    msgmerge -F -U lang/po/en_NZ.po lang/po/cataclysm-dda.pot


Step 3: Translate
-----------------

Now open the .po file in your favorite editor and translate!
The lines beginning with "msgid" must be left in the english form,
only the lines beginning with "msgstr" should be translated.


Step 4: Compile the .po file
----------------------------

If it is a new translation,
you will need to create a subdirectory in lang/mo for it,
and then a subdirectory called "LC_MESSAGES" inside that.
For example:

    mkdir -p lang/mo/en_NZ/LC_MESSAGES

Now run the "msgfmt" program to compile the translations for use in game.

    msgfmt -c -o lang/mo/en_NZ/LC_MESSAGES/cataclysm-dda.mo lang/po/en_NZ.po

Hooray, that's it :).

Testing your changes in game
============================

The game has no menu to change language, so you need to manually set the locale.
This is a different process depending on your OS.

Note: The locale you set doesn't have to be an exact match. For instance, to use the
`de_DE.po` translation, setting your locale to `de_DE.UTF8` will work fine.

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
