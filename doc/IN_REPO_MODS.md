<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
*Contents*

- [Getting Mods Into The Repository](#getting-mods-into-the-repository)
  - [Why have in-repo mods?](#why-have-in-repo-mods)
  - [What kind of mods can be in the CleverRaven repository?](#what-kind-of-mods-can-be-in-the-cleverraven-repository)
  - [What is necessary for a mod to be included in the repository?](#what-is-necessary-for-a-mod-to-be-included-in-the-repository)
  - [What are the responsibilities of a mod curator?](#what-are-the-responsibilities-of-a-mod-curator)
  - [When do mods get removed?](#when-do-mods-get-removed)
  - [Why do mods get removed?](#why-do-mods-get-removed)
  - [How can mods be rescued from removal?](#how-can-mods-be-rescued-from-removal)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Getting Mods Into The Repository

Cataclysm: Dark Days Ahead is not only moddable, but ships with a number of mods available for users to select from even without having to obtain mods from a third party source.

## Why have in-repo mods?

The benefits of having mods in the repository include:

*  Visibility to a large number of users and potential contributors
*  The ability to use the CleverRaven issue tracker to report problems, as well as manage pull requests for adding to or fixing mods in the repository
*  Mods in the CleverRaven repository that demonstrate usage of a game feature which exists only for mods (i.e. not used by the core DDA game itself) help ensure that feature stays present in the game.

There are however things that might be expected but are not guaranteed:

*  Guarantee of maintenance of content -- core devs are mostly focused on the main game itself; although some might pick up mods as a side task, people who continue working on the core game tend to like the feel of the core game.
*  Guarantee of maintenance of used features -- over the course of development it may become impossible to keep a feature, due to performance needs or maintenance burden.
*  Guarantee of sole authorship -- once in the repository, the mod is considered community content. While the developers might defer to an active curator of a mod, if they become inactive anyone else can step forward.

## What kind of mods can be in the CleverRaven repository?

There are three primary categories of mods:

*  Content mods, which provide some kind of distinct experience from the core Dark Days Ahead game. This could be a change to lore, a change to a specific part of gameplay.
*  User Experience (UX) mods, which alter the look and feel of the game interface itself. Accessibility mods, which make it possible for someone to play the game when they otherwise couldn't, fall under this category. Mods which simply adjust game elements for preferences and taste do not.
*  Development mods, which aren't "mods" in the typical sense but are instead there to ease the transition between "incomplete feature" and "complete feature", when a feature in the core game is sufficiently incomplete that the developers believe it needs to be optional to minimize disruption to players.

## What is necessary for a mod to be included in the repository?

The most crucial criteria for a mod to be in the CleverRaven repository is that it has someone acting as a curator. This ensures that there is someone who is keeping an eye out for possible problems with the mod, and helping steer its development so that it continues to develop in accordance with its design purpose.
In some rare cases we might treat a mod as an important but optional extension of the game itself, such as for some accessibility or development mods. When that happens, the 'curator' is the developer and contributor team collectively, just as with Dark Days Ahead itself. We try to do this sparingly, due to the obvious difficulty of pushing volunteers towards tasks they aren't personally interested in.

Furthermore, there are additional criteria:

*  Content mods need to be providing some kind of curated experience. Either they introduce a set of locations, encounters, equipment, or progression mechanisms that do not fit the style of the core game, or they in some fashion modify the game according to some well-defined concept.
*  *  Mods which do not qualify as curated experiences: Grab bag mods with no defined purpose (i.e. adding a bunch of random guns), "settings" mods that just turn off a possibly undesired but working feature (acid ants).
*  Development mods need to have an associated work-in-progress feature that they exist to mitigate.

## What are the responsibilities of a mod curator?

A mod curator's role is to curate additions to mods and ensure they fit the concept of the mod.  Curators are also responsible for at least acknowledging bug reports related to the mod: while curators don't have to personally manage every bugfix, if a bug is reported, the curator should be around to comment and help to find someone to fix it.

It's important to understand that if a mod is in the repository, it is expected that people other than the curator are going to contribute to it, as with anything.  On GitHub, mod curators should Approve or Request Changes on PRs to the mod.  If the curator has approved it, developers with merge permissions will treat that as a request to merge the changes.

## When do mods get removed?

Mods that have no curator or have proven to be consistently some type of maintenance burden are potentially subject to removal. Additionally, mods which had a purpose which is no longer relevant (development mods whose accompanying feature has been finished) will generally be removed once they're no longer deemed necessary.  If a curator can't be reached for comment on a bugfix or review on a PR'd change for about a month, and hasn't left any indication of when they'll be back, developers will assume they've left the project, and the mod is orphaned.

Whenever a stable release occurs, mods which no longer qualify for inclusion will be marked 'obsolete'. This doesn't remove them from the repository, but it does prevent it from being used in new worlds (barring additional effort on the part of the player).

## Why do mods get removed?

While it may be counterintuitive, simply having a mod in the repository increases maintenance burden for the team, even for the people who don't use the mods itself.

Furthermore, mods which are shipped with the game but are not working correctly for whatever reason reflect poorly on the project on the whole. While it might seem 'easy' to fix a mod, the development team is mostly focused on the core game itself. This is why mod curators are necessary.

## How can mods be rescued from removal?

If the mod otherwise meets inclusion criteria but lacks a curator (i.e. has been declared orphaned), it's as simple as having someone else step forward as the new curator.

Otherwise, it needs to either be made to meet the criteria, or it simply isn't going to be staying in the CleverRaven repository.
