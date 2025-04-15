# Translating MOD

This guide demonstrates how to translate a mod through an example. Suppose you have made a mod adding a book item to the game, and you have put your mod in "./mods/demo" folder:

```jsonc
[
  {
    "type": "MOD_INFO",
    "id": "demo",
    "name": "Demo MOD",
    "authors": [ "..." ],
    "description": "This mod adds a book.",
    "category": "content",
    "dependencies": [ "cdda" ]
  },
  {
    "type": "BOOK",
    "id": "demo_item",
    "name": { "str": "Guide to Translate a MOD", "str_pl": "copies of Guide to Translate a MOD" },
    "description": "A thin book teaching how to translate a mod."
  }
]
```

The first step is to generate the translation template for your mod.

```sh
# In Cataclysm DDA root directory
mkdir -p ./mods/demo/lang/po
python3 ./lang/extract_json_strings.py -i ./mods/demo -o ./mods/demo/lang/po/demo.pot
```

This extracts all translatable text from your mod and stores this information to `./mods/demo/lang/po/demo.pot`. This file in `.pot` format is called translation template. It contains all text in the original language that is to be translated:

```po
msgid ""
msgstr ""
"Project-Id-Version: None\n"
"POT-Creation-Date: 1970-01-01 00:00+0000\n"
"PO-Revision-Date: 1970-01-01 00:00+0000\n"
"Last-Translator: None\n"
"Language-Team: None\n"
"Language: en\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
"Plural-Forms: nplurals=2; plural=(n > 1);\n"

#. ~ MOD name
#: mods/demo/modinfo.json
msgid "Demo MOD"
msgstr ""

#. ~ Description of MOD "Demo MOD"
#: mods/demo/modinfo.json
msgid "This mod adds a book."
msgstr ""

#. ~ Item name
#: mods/demo/modinfo.json
msgid "Guide to Translate a MOD"
msgid_plural "copies of Guide to Translate a MOD"
msgstr[0] ""
msgstr[1] ""

#. ~ Description of "Guide to Translate a MOD"
#: mods/demo/modinfo.json
msgid "A thin book teaching how to translate a mod."
msgstr ""
```

Next, you can upload this translation template to an online translation platform for translators to translate; alternatively, you can also create translation files for a target language from this translation template locally on your computer, for example, Russian:

```sh
msginit -o mods/demo/lang/po/ru.po -i mods/demo/lang/po/translation.pot -l ru
```

Now you get `./mods/demo/lang/po/ru.po`. Translations files in `.po` format are human readable text files that contain mappings between text in the original language and in the target language. Translators work on `.po` files and fill in translations:

```po
msgid ""
msgstr ""
"Project-Id-Version: None\n"
"POT-Creation-Date: 1970-01-01 00:00+0000\n"
"PO-Revision-Date: 1970-01-01 00:00+00000\n"
"Last-Translator: None\n"
"Language-Team: Russian <gnu@d07.ru>\n"
"Language: ru\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
"Plural-Forms: nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n"
"%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);\n"

#. ~ MOD name
#: mods/demo/modinfo.json
msgid "Demo MOD"
msgstr "<fill in translations here>"

#. ~ Description of MOD "Demo MOD"
#: mods/demo/modinfo.json
msgid "This mod adds a book."
msgstr "<fill in translations here>"

#. ~ Item name
#: mods/demo/modinfo.json
msgid "Guide to Translate a MOD"
msgid_plural "copies of Guide to Translate a MOD"
msgstr[0] "<fill in translations here for the first plural form>"
msgstr[1] "<fill in translations here for the second plural form>"
msgstr[2] "<fill in translations here for the third plural form>"

#. ~ Description of "Guide to Translate a MOD"
#: mods/demo/modinfo.json
msgid "A thin book teaching how to translate a mod."
msgstr "<fill in translations here>"
```

At this stage, the directory structure of your mod looks like this:
```
demo
├── items.json
├── lang
│   └── po
│       ├── es_AR.po <- Spanish (Argentina) translation
│       ├── es_ES.po <- Spanish (Spain) translation
│       ├── ja.po <- Japanese translation
│       ├── ru.po <- Russian translation
│       └── translation.pot <- translation template in English
├── modinfo.json
└── your_mod_content.json
```

After the translations are completed, compile translation files from human-readable `.po` format to game-usable `.mo` format:

```sh
mkdir -p mods/demo/lang/mo/ru/LC_MESSAGES/
msgfmt -o mods/demo/lang/mo/ru/LC_MESSAGES/demo.mo mods/demo/lang/po/ru.po
```

The translation template in `.pot` format and translations files in `.po` format are only used by yourself and translators. You only need to include translation data in `.mo` format in your mod release:

```
demo
├── lang
│   └── mo
│       ├── es_AR
│       │   └── LC_MESSAGES
│       │       └── demo.mo
│       ├── es_ES
│       │   └── LC_MESSAGES
│       │       └── demo.mo
│       ├── ja
│       │   └── LC_MESSAGES
│       │       └── demo.mo
│       └── ru
│           └── LC_MESSAGES
│               └── demo.mo
├── modinfo.json
└── your_mod_content.json
```

When the player plays the Russian version of Cataclysm DDA, the Russian translation of your mod in `lang/mo/ru/LC_MESSAGES/demo.mo` is automatically loaded, and the player will see translated text in Russian in the game.
