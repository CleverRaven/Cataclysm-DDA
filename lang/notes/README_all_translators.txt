Instructions for Cataclysm-DDA translators
==========================================


Translation file format
-----------------------

Translations are stored in ".po" files, named with a language code specific to each language and country. So for example the translations for the Spanish spoken in Spain would be found in "es_ES.po" and for Spanish spoken in Mexico would be found in "es_MX.po".

It is a plain-text filetype, so you can edit it however you choose, but translators often prefer to use purpose-built translation editors (such as Poedit from poedit.net), or web-based translation tools (such as translations.launchpad.net).

The format of .po files is a list of entries, with the english phrase to be translated, followed by the local translation. The english phrase is on the line or lines beginning with "msgid", and the translated phrase goes on the line or lines beginning with "msgstr".

Before the "msgid" line there will be a comment line indicating where in the source code the word or phrase came from. This can often help when the meaning of the english is not obvious. There may also be comments left by the developers to make translation easier.

Most entries will look something like this:

    #: action.cpp:421
    msgid "Construct Terrain"
    msgstr "niarreT tcurtsnoC"

The english phrase here is "Construct Terrain", and it comes from line 421 of the file "action.cpp". The example translation is just a reversal of the english letters. With this, in stead of "Construct Terrain", the game will display "niarreT tcurtsnoC".

Another exmple is:

    #: action.cpp:425 defense.cpp:635 defense.cpp:701 npcmove.cpp:2049
    msgid "Sleep"
    msgstr "pleeS"

This is similar to the last example, except it is a more common phrase. It is used in the files action.cpp, defense.cpp (twice) and npcmove.cpp. The translation will replace every usage.




Translation file header
-----------------------

The header at the top of the .po file is the only part that differs from the comment/msgid/msgstr format.

If you are working on an already established translation you will not have to modify it.

For a new translation, it should be mostly set up for you, either by the editor you are using or by the "msginit" program which is the recommended way of initializing a translation (see TRANSLATING.md).

If you are starting from another translation file however, you might need to change a few things. Just fill it in as best you are able.

The header will look something like:

    # French translations for Cataclysm-DDA package.
    # Copyright (C) 2013 CleverRaven and Cataclysm-DDA contributors.
    # This file is distributed under the same license as the Cataclysm-DDA package.
    # Administrator <EMAIL@ADDRESS>, 2013.
    #
    msgid ""
    msgstr ""
    "Project-Id-Version: 0.7-git\n"
    "Report-Msgid-Bugs-To: http://github.com/CleverRaven/Cataclysm-DDA\n"
    "POT-Creation-Date: 2013-08-01 13:44+0800\n"
    "PO-Revision-Date: 2013-08-01 14:02+0800\n"
    "Last-Translator: YOUR NAME <your@email.address>\n"
    "Language-Team: French\n"
    "Language: fr\n"
    "MIME-Version: 1.0\n"
    "Content-Type: text/plain; charset=UTF-8\n"
    "Content-Transfer-Encoding: 8bit\n"
    "Plural-Forms: nplurals=2; plural=(n > 1);\n"

If you are starting a new translation, or you are in charge of the existing translation, it is helpful if you include your name and e-mail address so that you can be contacted with any questions or issues regarding the translation.

The only important part that cannot be easily filled out manually is the "Plural-Forms" section. It determines how different numbers of things are handled in your language. More on that later.




Format strings and newlines
---------------------------

Some strings will have special terms such as "%s", "%2$d" and "\n".

"\n" represents a linebreak. Mostly these are unnecessary as the code wraps lines where it can, but sometimes these are used for placing things on different lines. Just use "\n" in your translation wherever a new line should go.

"%s" and other similar terms are replaced with something else when the game is running. You might need to move them around, depending on the translation. It is important that every term beginning with "%" is kept in the translation.

Here is an example which replaces a "%d" with a number:

    #: addiction.cpp:224
    #, c-format
    msgid ""
    "Strength - %d;   Perception - 1;   Dexterity - 1;\n"
    "Depression and physical pain to some degree.  Frequent cravings.  Vomiting."
    msgstr ""
    ";1 - ytiretxeD   ;1 - noitpecreP   ;%d - htgnertS\n"
    ".gnitimoV  .sgnivarc tneuqerF  .eerged emos ot niap lacisyhp dna noisserpeD"

Here it is important that the "%d" was not reversed, and that the "\n" remained at the end of the line. In this case, "%d" will be replaced with the character's strength modifier when the message is displayed.

In some cases it might be necessary to change the order of terms. This can confuse the game. If the order of the "%" terms changes, you must add numbers to all of them, so that the game knows which was which. Some strings will already have these numbers, but some might not.

As an example, if there is a string with "%s shoots %s!", it might change in translation. Perhaps it will become something like "%s is shot by %s!". But now it is the wrong way around, the shooter has swapped with the shootee.

In this case, each "%s" should be numbered with a digit (1-9) then a dollar sign ($) between the "%" and the "s". For example "%1$s shoots %2$s!" would be equivalent to "%s shoots %s!". So the example translation above could be "%2$s is shot by %1$s!", and this would work correctly.

The game can figure out these "%1$s" "%2$s" parameters automatically, but you must make sure that (A): all of the "%" terms in the translation are numbered; and (B): the numbers are correct in terms of the original ordering in the english text.

For example:

    #: map.cpp:680
    #, c-format
    msgid "%s loses control of the %s."
    msgstr "%2$s eht fo lortnoc sesol %1$s"

would be displayed in-game as "kcurt eht fo lortnoc sesol liagibA", assuming "Abigail" was driving a "truck".




Special tags in strings
-----------------------

Some strings in the translation may have special tags in front of or inside them. These tags should be left as-is, and only the rest of the string translated.

For example, the NPC and city names from "data/raw/names.json" are prefixed with "<name>" so as to avoid conflicts with words (such as "Wood" the material, and "Wood" the last name). For these, the "<name>" part should be left in.

For example:

#. ~ proper name; gender=female; usage=given
#: lang/json/json_names.py:6
msgid "<name>Abigail"
msgstr "<name>liagibA"

Names also have a comment above them, indicating what the name is used for in-game. In this case, "Abigail" is a possible first name for a female NPC.




Plural forms
------------

Many languages use different terms for things depending on how many of them there are. These are supported using plural forms, defined by the "Plural-Form" line in the .po file header.

For these, there will be multiple "msgstr" lines, intended for the different forms depending on number. The game will automatically choose the correct form depending on the number of things.

For example:

    #: melee.cpp:913
    #, c-format
    msgid "%d enemy hit!"
    msgid_plural "%d enemies hit!"
    msgstr[0] "!tih ymene %d"
    msgstr[1] "!tih seimene %d"

Here the first entry is for when there is only one "enemy", the second is for when there are more than one "enemies". The rules differ wildly between languages.

