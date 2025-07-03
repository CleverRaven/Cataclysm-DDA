---
layout: page
title: ""
---

## Code Style

Both c++ and JSON files within the repository have their style validated as part of the build process.
See doc/CODE_STYLE.txt and doc/JSON_STYLE.md for details

## Translations

The game uses [gettext](https://www.gnu.org/software/gettext/) to display translated texts.

This requires two actions:
- Marking strings that should be translated in the source.
- Calling translation functions at run time.

The first action allows automatically extracting those strings, which are given to the translator team. They return a file that maps the original string (usually in English) as it appears in the code to the translated string. This is used at run time by the translation functions (the second action).

Note that only extracted strings can get translated. The original string is used to request the translation, in other words: the original string is the identifier used for a specific translation. If the function doesn't know the translation, it returns the original string.

The empty string is always translated into some debug information, not into an empty string. In most situations, the string can be expected to be non-empty, so you can always translate it. If the string *could* be empty (and it needs to remain empty after the translation), you'll have to check for this and only translate if non-empty.

Error messages (which indicate a bug in the program) and debug messages should not be translated. If they appear, the player is expected to report them *exactly* as they were printed by the game.

In practice, you should use one of the following three functions in the source code to get a translation.

String *literals* that are used in any of these functions are automatically extracted. Non-literal strings are translated at run time, but they won't get extracted.
```C++
const char *translated = _("text"); // marked for extraction
const char *untranslated = "text"; // same string as the original above.
// This translates because the input string "text" has been marked
// and a translation for it will be available
const char *translated_again = _(untranslated);
untranslated = "other text";
// This does not translate, the string "other text" was never marked for translation
// and the translation system therefor doesn't know how to translate it.
translated_again = _(untranslated);
```

Strings from the JSON data files are extracted via the `lang/extract_json_strings.py` script. They can be translated at run time using `_()`.

### Using `_()`

For simple C-strings:
```c++
const char *translated = _( "original text" );
// also works directly:
add_msg( _( "You drop the %s." ), the_item_name );
```

### Using `pgettext()`

If the original string is short or its meaning in isolation is ambiguous (e.g. "blue" can be a color or an emotion), one can add a context to the string. The context is provided to the translators, but is not part of the translated string itself. This is done by calling `pgettext`.  This function's first parameter is the context, the second is the string to be translated.
```c++
const char *translated = pgettext("The direction: East", "E");
```

### Using `ngettext()`

Many languages have plural forms.  Some have complex rules for how to form these. To translate the plural form correctly, use `ngettext`.  Its first parameter is the untranslated string in singular, the second parameter is the untranslated string in plural form and the third is used to determine the required plural form.
```c++
const char *translated = ngettext( "one zombie.", "many zombies", num_zombies );
```
Some classes (like `itype` and `mtype`) provide a wrapper for these functions, named `nname`.

## Doxygen Comments

Extensive documentation of classes and class members will make the code more readable to new contributors. New doxygen comments for existing classes are a welcomed contribution.

Use the following template for commenting classes:
```c++
/**
 * Brief description
 *
 * Lengthy description with many words. (optional)
 */
class foo {
```

Use the following template for commenting functions:
```c++
/**
 * Brief description
 *
 * Lengthy description with many words. (optional)
 * @param param1 Description of param1 (optional)
 * @return Description of return (optional)
 */
int foo(int param1);
```

Use the following template for commenting member variables:
```c++
/** Brief description **/
int foo;
```

Helpful pages:
https://www.stack.nl/~dimitri/doxygen/manual/commands.html
https://www.stack.nl/~dimitri/doxygen/manual/markdown.html#markdown_std
https://www.stack.nl/~dimitri/doxygen/manual/faq.html

### Guidelines for adding documentation
* Doxygen comments should describe behavior towards the outside, not implementation, but since many classes in Cataclysm are intertwined, it's often necessary to describe implementation.
* Describe things that aren't obvious to newcomers just from the name.
* Don't describe redundantly: `/** Map **/; map* map;` is not a helpful comment.
* When documenting X, describe how X interacts with other components, not just what X itself does.

### Building the documentation for viewing it locally
* Install doxygen
* `doxygen doxygen_doc/doxygen_conf.txt `
* `firefox doxygen_doc/html/index.html` (replace firefox with your browser of choice)

## Advanced Techniques

These guidelines aren't essential, but they can make keeping things in order much easier.

#### Using remote tracking branches

Remote tracking branches allow you to easily stay in touch with this repository's `master` branch, as they automatically know which remote branch to get changes from.

    $ git branch -vv
    * master      xxxx [origin/master] ....
      new_feature xxxx ....

Here you can see we have two branches: `master`, which is tracking `origin/master`, and `new_feature`, which isn't tracking any branch. In practice, what this means is that git won't know where to get changes from.

    $ git checkout new_feature
    Switched to branch 'new_feature'
    $ git pull
    There is no tracking information for the current branch.
    Please specify which branch you want to merge with.

In order to easily pull changes from `upstream/master` into the `new_feature` branch, we can tell git which branch it should track. (You can even do this for your local master branch.)

    $ git branch -u upstream/master new_feature
    Branch new_feature set up to track remote branch master from upstream.
    $ git pull
    Updating xxxx..xxxx
    ....

You can also set the tracking information at the same time as creating the branch.

    $ git branch new_feature_2 --track upstream/master
    Branch new_feature_2 set up to track remote branch master from upstream.

 * Note: Although this makes it easier to pull from `upstream/master`, it doesn't change anything with regards to pushing. `git push` fails because you don't have permission to push to `upstream/master`.

        $ git push
        error: The requested URL returned error: 403 while accessing https://github.com/CleverRaven/Cataclysm-DDA.git
        fatal: HTTP request failed
        $ git push origin
        ....
        To https://github.com/YOUR_USERNAME/Cataclysm-DDA.git
        xxxx..xxxx  new_feature -> new_feature


