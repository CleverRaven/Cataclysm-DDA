# Testing your changes and making sure they work as expected

So you contributed or did something else that warrants testing. Nice work.
Our tests will already cover a good amount of things that could go wrong,
but you are expected to test your non-trivial changes in-game.
Typo fixes and the like usually do not need to be tested.

## Style

The [Manual of Style](MANUAL_OF_STYLE.md), applies to any proposed change,
so read this one first.

For JSON changes, read [JSON_STYLE.md](JSON_STYLE.md).
For C++ changes, read [CODE_STYLE.md](CODE_STYLE.md).

All released builds include a pre-compiled `json_formatter.cgi`, which
is a handy tool to automatically format any JSON file you point it at
to our chosen standards.

## Applying your changes locally

For JSON-only changes, which make up most contributions, a very simple
way is to simply place your Cataclysm executable into your
git repository.
It will automatically use this altered path to load resources.
For more advanced situations, there are commandline parameters such as
`--datadir`, which you can point at your git respository.
Check `--help` for more information.

You should also use an up-to-date version of Cataclysm and ensure that
the git revision matches the Cataclysm executable version to avoid
conflicts due to other people's changes to the JSON handling code.

Exiting to the main menu and loading your save again will re-read most
JSON files, so after making changes, make sure to either relaunch
Cataclysm or reload your save, whatever is more convenient.

For C++ changes, you will need to compile the game yourself.
See [COMPILING.md](COMPILING/COMPILING.md).
You may also want to look at [TESTING.md](TESTING.md) for our unit
tests.

## Using the debug menu

**Tip: If you name your character "Debug" (or anything starting with Debug),
the game will not automatically disable achievements and display the
"The debug menu will not help you get better at the game" anymore.**

The debug menu is a powerful and convenient tool. If you debug frequently,
consider globally binding it to a key of your choice, e.g F12.
Different circumstances need different testing, so messing around with EOCs
needs different testing than adding more item qualities for example.

### Items

With *Spawning > Spawn item* you can conjure up any item you want.
This menu is very self-explanatory. If an item name ends with a yellow
`(S)`, it means that the item description is variable as there are snippets
(hence the `(S)`) manipulating them. You can choose which snippet to apply.
A gray `(V)` means something similar, but it is a variant, which alters
more than just the description. As of writing, these can not yet be
selected via the debug menu and are randomized.

### Mapgen

Many of the options in the `Map` part of the debug menu are self-explanatory.

The *Map editor* and *Overmap editor* are of particular interest though, yet
they may not be as straightforward as the other options.

The *Map editor* can be used to alter small amounts of terrain and also
apply a different overmap terrain without actually touching its ID.

The *Overmap editor* is mainly useful to spawn buildings and other specials.
For spawning entire buildings or other things occupying multiple overmap
tiles, type `s` to spawn a special. Spawning individual pieces of overmap
terrain is usually not as commonly used and less useful in that sense.

Make sure to only spawn overmap specials in the non-highlighted areas, or
they will not be applied.

### Monsters

With *Spawning > Spawn monster*, you can spawn an individual monster
or an entire horde of them.
This is very useful to test e.g monster faction and combat behavior.
Unfortunately there is no way to spawn a monster group (`mongroup`)
yet, as of writing.

For more advanced playtesting, use loadouts from the Standard Combat
Testing Mod (included in the Misc category of built-in mods) and document
your results in the Testing section of your PR.

Important test tips:
 - A spawned and saved monster always stays the same, even if you change the underlying monster definition.  Always use freshly spawned monsters!
 - Evolution, growth, and reproduction happen on monster load, so the sequence of testing is: Spawn monster -> Teleport away to unload it -> Teleport back to load it and start the timers -> Teleport away -> Set time forward via the debug menu -> Teleport back.
 - Activating Debug Mode's monster filter allows you to examine monsters using `x` and then `e` and get additional information.
