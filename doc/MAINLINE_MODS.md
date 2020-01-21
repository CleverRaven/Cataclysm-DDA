# Getting Mods Into The Repository

Cataclysm: Dark Days Ahead is not only moddable, but ships with a number of mods available for users to select from even without having to obtain mods from a third party source.

## Why have mainlined mods?

The benefits of having mods mainlined include:

*  Visibility to a large number of users and potential contributors
*  The ability to use the CleverRaven issue tracker to report problems, as well as manage pull requests for adding to or fixing mods in the repository
*  Mods in mainline that demonstrate usage of a game feature which exists only for mods (i.e. not used by the core DDA game itself) help ensure that feature stays present in the game.

There are however things that might be expected but are not guaranteed:

*  Guarantee of maintenance of content -- core devs are mostly focused on the main game itself; although some might pick up mods as a side task, people who continue working on the core game tend to like the feel of the core game.
*  Guarantee of maintenance of used features -- over the course of development it may become impossible to keep a feature, due to performance needs or maintenance burden.
*  Guarantee of sole authorship -- once in the repository, the mod is considered community content. While the developers might defer to an active maintainer of a mod, if they become inactive anyone else can step forward.

## What kind of mods can be in the repository as mainlined mods?

There are three primary categories of mods:

*  Content mods, which provide some kind of distinct experience from the core Dark Days Ahead game. This could be a change to lore, a change to a specific part of gameplay.
*  User Experience (UX) mods, which alter the look and feel of the game interface itself.
*  Development mods, which aren't "mods" in the typical sense but are instead there to ease the transition between "incomplete feature" and "complete feature", when a feature in the core game is sufficiently incomplete that the developers believe it needs to be optional to minimize disruption to players.

## What is necessary for a mod to be included in the repository?

The most crucial criteria for a mod to be in the CleverRaven repository is that it has someone acting as a maintainer. This ensures that there is someone who is keeping an eye out for possible problems with the mod, and helping steer its development so that it continues to develop in accordance with its design purpose.

Furthermore, there are additional criteria:

*  Content mods need to be providing some kind of curated experience. Either they introduce a set of locations, encounters, equipment, or progression mechanisms that do not fit the style of the core game, or they in some fashion modify the game according to some well-defined concept.
*  *  Mods which do not qualify as curated experiences: Grab bag mods with no defined purpose (i.e. adding a bunch of random guns), "settings" mods that just turn off a possibly undesired but working feature (acid ants).
*  Development mods need to have an associated work-in-progress feature that they exist to mitigate. 

## What are the responsibilities of a mod maintainer?

A mod maintainer's role is to curate additions to mods and ensure they fit the concept of the mod.  Maintainers are also responsible for at least acknowledging bug reports related to the mod: while maintainers don't have to personally manage every bugfix, if a bug is reported, the maintainer should be around to comment and help to find someone to fix it.

It's important to understand that if a mod is part of mainline, it is expected that people other than the maintainer are going to contribute to it, as with anything.  On github, mod maintainers should Approve or Request Changes on PRs to the mod.  If the maintainer has approved it, developers with merge permissions will treat that as a request to merge the changes.

## When do mods get removed?

Mods that have no maintainer or have proven to be consistently some type of maintenance burden are potentially subject to removal. Additionally, mods which had a purpose which is no longer relevant (development mods whose accompanying feature has been finished) will generally be removed once they're no longer deemed necessary.  If a maintainer can't be reached for comment on a bugfix or review on a PR'd change for about a month, and hasn't left any indication of when they'll be back, developers will assume they've left the project, and the mod is orphaned.

Whenever a stable release occurs, mods which no longer qualify for inclusion will be marked 'obsolete'. This doesn't remove them from the repository, but it does prevent it from being used in new worlds (barring additional effort on the part of the player).

## Why do mods get removed?

While it may be counterintuitive, simply having a mod in the repository increases maintenance burden for the team, even for the people who don't use the mods itself.

Furthermore, mods which are in mainline but are not working correctly for whatever reason reflect poorly on the project on the whole. While it might seem 'easy' to fix a mod, the development team is mostly focused on the core game itself. This is why mod maintainers are necessary.

## How can mods be rescued from removal?

If the mod otherwise meets inclusion criteria but lacks a maintainer (ie. has been declared orphaned), it's as simple as having someone else step forward as the new maintainer.

Otherwise, it needs to either be made to meet the criteria, or it simply isn't going to be staying in mainline.
