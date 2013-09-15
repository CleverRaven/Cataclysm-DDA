Info on translating Cataclysm-DDA into another language.


TRANSLATORS
===========

The current official location for translating Cataclysm-DDA is at
[Launchpad Translations](https://translations.launchpad.net/cdda).

For the moment, there are separate locations for translating
[Chinese](https://translations.launchpad.net/cataclysm)
and [Russian](https://translations.launchpad.net/cataclysm-dda).
We will be merging these once translations have stabalized.

Using Launchpad Translations anyone can help translate.
All you need to do is set up a Launchpad account,
and tell it your preferred language.
After that, your language should show up for translation,
even if you are starting a new translation from scratch.

If you have any questions or comments about translation,
feel free to post in the "Translations Team Discussion" subforum of
[the Cataclysm-DDA forums](http://smf.cataclysmdda.com/).

There are some issues specific to Cataclysm-DDA,
(and some specific to translating computer programs in general,)
which translators should be aware of,
such as the use of terms like `%s` and `%3$d` (leave them as they are),
and the use of tags like `<name>` (don't translate the tags).

Information about these,
and any other issues specific to individual languages,
can be found in Cataclysm-DDA's `lang/notes/` folder.
General notes for all translators are in
`lang/notes/README_all_translators.txt`,
and notes specific to a language may be stored as `<lang_id>.txt`,
for example `lang/notes/de.txt` for German.

Cataclysm-DDA has more than 10,000 translatable strings,
but don't be discouraged.
The more translators there are,
the easier it becomes :).


MAINTAINERS
===========

Several steps need to be done in the correct order,
to correctly merge and maintain the translation files.

There are scripts available for these,
so usually the process will be as follows:

1. Download the translations in .po format from Launchpad.
2. Put them in `lang/incoming/`,
   ensuring they are named consistently with the files in `lang/po/`.
3. Run `lang/update_pot.sh` to update `lang/po/cataclysm-dda.pot`.
4. Run `lang/merge_po.sh` to update `lang/po/*.po`.
   This will also merge the translations from `lang/incoming/`.

These steps should be enough to keep the translation files up-to-date.

To compile the .po files into .mo files for use,
run `lang/compile_mo.sh`.
It will create a directory in `lang/mo/` for each language found.

Also note that both `lang/merge_po.sh` and `lang/compile_mo.sh`
accept arguments specifying which languages to merge or compile.
So to compile only the translation for, say, Traditional Chinese (zh_TW),
one would run `lang/compile_mo.sh zh_TW`.

After compiling the appropriate .mo file,
if your system is using that language,
the translations will be automatically used when you run cataclysm.

If your system locale is different from the one you want to test,
the easiest way to do so is to find out your locale identifier,
compile the translation you want to test,
then rename the directory in `lang/mo/` to your locale identifier.

So for example if your local language is New Zealand English (en_NZ),
and you want to test the Russian (ru) translation,
the steps would be `lang/compile_mo.sh ru`,
`mv lang/mo/ru lang/mo/en_NZ`,
`./cataclysm`.


