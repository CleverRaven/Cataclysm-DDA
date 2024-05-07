# Manual Of Style

Follow these conventions when adding or editing in-game text. Theses conventions apply to the default US English language, translations should follow guidelines from [/lang/notes/](../lang/notes) if there are any for the language in question, which can be different that the ones here.

1. Use US English spelling.
   1. The exception here is for some character dialogs, as some characters can use other than US English dialects. For example, Exodii/Rubik uses [Cockney dialect](https://en.wikipedia.org/wiki/Cockney) in combination of [Early Modern English](https://en.wikipedia.org/wiki/Early_Modern_English).
3. Use double sentence spacing after periods.  This means that a period that ends a sentence should be followed by two spaces.  If the sentence is the last in the block of text, there should be no spaces following it.
   1. Translations may not always use two spaces, so make sure to use a translation wrapper, `_(" ")`, when appending spaces in code.
4. Use second person point of view (eg. "you").
5. The names of stats, traits/mutations, scenarios, professions, backgrounds, proficiencies, martial arts, and Compact Bionics Modules (CBMs) should be in title case.  This means that each word should be capitalized unless it is an article, preposition, or conjunction.
6. Items and entities with proper noun names should also be in title case.
   1. This includes: Cataclysm, Discontinuity (an event in Aftershock), Exodii, Marloss, Mycus, Autodoc, Kevlar and Nomex.
   2. Unique currencies, such as "merch", should be written lowercase unless the name includes a proper noun, such as in "Hubcoin".
7. All other item and entity names should be in all lower-case letters.
8. Use the serial comma (Oxford comma).
9. Use ellipsis character (…) instead of three dots (...).  Replace instances of three periods with the dedicated Unicode character for ellipsis, namely U+2026. As to the specifics of using it:
   1. No spaces before it and one space after it.
   2. This character does not end a sentence, use ellipsis followed by a period `….` when ending a sentence with ellipsis.
10. Brand names do not need to be avoided as we are covered under fair use.  However, as CDDA-Earth is a parallel universe, nonexistent brands are also allowed.
11. Don't avoid using Unicode letters, which includes proper names and alphabets when needed, or symbols as ® or ™.
12. Always make sure that all descriptions follow a sentence case, i.e. they start with a big letter and end with a full stop, even if they are just for testing purposes (they can still appear by mistake and can be seen in debug menus).

For writing NPC dialogues with mentioned conditional checks:

1. When specifying stats, use the abbreviated form, e.g `[PER 10] I can see something weird.`. Avoid: `[PER 10+]` or `[PERCEPTION 10]`.
2. When specifying skills, use regular sentence case, e.g `[Tailoring 2] I can patch it up for you!`
3. When specifying traits, write it in all uppercase, e.g `[SWEET TOOTH] Do you have something more sugary than fruit?`
4. When specifying that an item will be used (not used up), use e.g `[Use Stethoscope] Let's see if this one is still alive.`
5. When specifying an action without any dialogue, use e.g `[Push the button.]`.
