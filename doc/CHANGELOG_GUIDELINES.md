These are the categories used by Pull Request Summary lines as outlined in the [PR Template](../.github/pull_request_template.md).
In the end, it's up to the author to decide where *they* want their change to be listed, these are only guidelines, not rules.

#### Features
If it adds something new that the player can do, or that can happen to the player, it's a feature.
These will generally be edits to the C++ code in the game, though new json entities may be involved as well.
#### Content
If it adds new monsters, new map areas, new items, new vehicles, new doohickeys, it's content.
These will generally be dominated by json edits, though there may be some new C++ code to support the new entities.
#### Interface
If it adjusts how the player interacts with the game, adding/adjusting menus, changing shortcuts, streamlining workflows, it's interface.
These will generally be C++ edits, though some kinds of interface changes my be done in json.
#### Mods
If a change is either contained within a mod, or extends what is capable within a mod, it goes in this category.
#### Balance
Sometimes a change doesn't add or remove anything, but it makes everything work together better.
These are probably evenly split between JSON and C++.
#### Bugfixes
If it was broken before, and it isn't anymore, that's a bugfix.
This applies equally C++ and JSON changes.
#### Performance
When there's no change at all except for less waiting, it's performance.
These will almost always happen in C++.
#### Infrastructure
These are changes for programmers, by programmers.
Most players will never even know these happened, but they'll appreciate the improved stability and features that this kind of things enables.
All manner of refactor and overhaul oriented at making the game easier to work on fit in this category.
These will frequently be C++ changes, but json reorganization also fits, as does development of tools that run outside the game.
#### Build
If you can't build the game, you can't play the game.
These are the changes that make building the game better, faster, stronger.
#### I18N
We want everyone to play, so we do translations, and we have code to support translations.
Changes oriented at letting everyone play no matter what their language go here.
