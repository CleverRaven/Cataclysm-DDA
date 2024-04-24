# How You Can Help

CDDA is maintained by thousands of volunteers, organized by a core development team. There are many
ways you can help, both through direct participation in the game's code and content development, and
by finding or reproducing bugs, locating causes, and testing the proposed fixes.

- See [ISSUES.md](../ISSUES.md) for how to report bugs or request new features
- See [CONTRIBUTING.md](../.github/CONTRIBUTING.md) for how to make and and submit changes to the
  game with pull requests
- See the [Guide to adding new content to CDDA for first time
contributors](https://github.com/CleverRaven/Cataclysm-DDA/wiki/Guide-to-adding-new-content-to-CDDA-for-first-time-contributors)
  for more help getting started with contributing


## Other ways to help

This section focuses on ways you can support CDDA development without necessarily knowing how to use
Git, C++, or even JSON.

As a player, you may follow CDDA development and know about the
[issues page on GitHub](https://github.com/CleverRaven/Cataclysm-DDA/issues)
where bugs and feature requests are reported.

If you don't have a GitHub account, you may use the
[Issue Triage Crowdsource](https://forms.gle/FBucNHVH5rJyJCfGA) form to tell us about issues of
particular importance or annoyance to you, which you think should be fixed before the next stable
release.

If you have a GitHub account, you may know you can [open issues](../ISSUES.md) and create pull
requests to [contribute code changes](../.github/CONTRIBUTING.md). But you can also use your GitHub
account to help manage and triage the existing issues and pull requests, and participate in
the discovery, discussion, and testing processes that surround game development.

Ways to help with issues:

- Reproduce issues that [need confirmation](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%28S1+-+Need+confirmation%29%22).
  Finding a procedure that reliably reproduces a problem is an important step toward finding a
  solution. Comment with your results (positive or negative), game version, the steps you followed,
  and any screenshots or other relevant info.

- Determine the first version, pull request, or commit affected by an issue. In the case of newly
  appearing bugs or regressions in game behavior, it may be possible to pinpoint when it first
  started occurring, by manually testing different game versions, or using a more sophisticated
  approach like `git bisect`. If you play experimental and update regularly, you might know right
  off the top of your head which version broke something! This is valuable information. If you can
  determine this and comment on the issue, it will definitely help locate a fix.

- Find closely related or possible duplicate issues. Even when people do search for existing issues
  before opening new ones, with thousands of open issues, they may still be hard to find, and
  duplicates often arise. If you notice two or more issues that are similar, related, or duplicated,
  please comment with the issue number(s) to generate a cross-reference link.

- Check old issues, to see if they are still relevant and reproducible in the current experimental
  version. Comment on any that may be already fixed or obsoleted, and could be closed with no
  further action - or, conversely, any that are still valid and worth fixing.

Ways to help with pull requests:

- Read pull request descriptions, and look at the affected code and/or JSON files. Provide
  constructive feedback in the form of comments or code reviews on the PR.

- Check out, compile, and test pull requests. The "Code" dropdown shown at the top-right
  of each pull request shows how to checkout the PR using [GitHub CLI](https://cli.github.com/), ex.
  `gh pr checkout 12345`. Build and playtest the PR to verify that it does what it should, and share
  your findings with the author.


## Issue labels

Both issues and pull requests are categorized with labels, which can be helpful for organization.
Typically someone with triage permissions will add appropriate labels. Use the links below to search
for issues by label.

If you want to just jump right in, search for the ["Good First Issue"](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Good+First+Issue%22) tag.

If you wish to find something pertaining to your specialty, use the more detailed tags, in any combination.


### Languages

| Tag                                                                                                             | Description
|---                                                                                                              |---
| [[C++]](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%5BC%2B%2B%5D)       | Previously named `Code`
| [[JSON]](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%5BJSON%5D)         | Code made in JSON
| [[Markdown]](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%5BMarkdown%5D) | Markdown issues
| [[Python]](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%5BPython%5D)     | Code made in Python

&nbsp;

### Issue Types

| Tag                                                                                                                                                     | Description
|---                                                                                                                                                      |---
| [\<Bug>](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%3CBug%3E)                                                   | This needs to be fixed
| [\<Bugfix>](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%3CBugfix%3E)                                             | This is a fix for a bug (or closes open issue)
| [<Crash / Freeze>](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%3CCrash+%2F+Freeze%3E%22)                     | Fatal bug that results in hangs or crashes.
| [\<Documentation>](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%3CDocumentation%3E)                               | Design documents, internal info, guides and help.
| [\<Enhancement / Feature>](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%3CEnhancement+%2F+Feature%3E%22)       | New features, or enhancements on existing
| [\<Exploit>](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%3CExploit%3E)                                           | Unintended interactions or behavior.
| [\<Question>](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%3CQuestion%3E)                                         | Answer me please.
| [\<Suggestion / Discussion>](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%3CSuggestion+%2F+Discussion%3E%22)   | Talk it out before implementing

&nbsp;

### Gameplay Categories

| Tag                                                                                                                                                                         | Description
|---                                                                                                                                                                          |---
| [Accessibility](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AAccessibility)                                                           | Issues regarding accessibility
| [Aiming](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AAiming)                                                                         | Aiming, especially aiming balance
| [Ammo](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AAmmo)                                                                             | Ammunition for all kinds of weapons
| [Artifacts](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AArtifacts)                                                                   | Otherworldly items with special effects
| [Battery / UPS](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Battery+%2F+UPS%22)                                                   | Electric power management
| [Bionics](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3ABionics)                                                                       | CBM (Compact Bionic Modules)
| [Character / Player](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Character+%2F+Player%22)                                         | Character / Player mechanics
| [Character / World Generation](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Character+%2F+World+Generation%22)                     | Issues and enhancements concerning stages of creating a character or a world
| [Controls / Input](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Controls+%2F+Input%22)                                             | Keyboard, mouse, keybindings, input UI, etc.
| [Crafting / Construction / Recipes](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Crafting+%2F+Construction+%2F+Recipes%22)         | Includes: Uncrafting / Disassembling
| [Effects / Skills / Stats](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Effects+%2F+Skills+%2F+Stats%22)                           | Effects / Skills / Stats
| [Fields / Furniture / Terrain / Traps](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Fields+%2F+Furniture+%2F+Terrain+%2F+Traps%22) | Objects that are part of the map or it's features.
| [Food / Vitamins](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Food+%2F+Vitamins%22)                                               | Comestibles and drinks
| [Gunmod / Toolmod](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Gunmod+%2F+Toolmod%22)                                             | Weapon and tool attachments, and add-ons
| [Info / User Interface](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Info+%2F+User+Interface%22)                                   | Game - player communication, menus, etc.
| [Inventory / AIM / Zones](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Inventory+%2F+AIM+%2F+Zones%22)                             | Inventory, Advanced Inventory Management or Zones
| [Items / Item Actions / Item Qualities](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Items+%2F+Item+Actions+%2F+Item+Qualities%22) | Items and how they work and interact
| [Lore](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3ALore)                                                                             | Game lore, in-game communication.
| [Magazines](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AMagazines)                                                                   | Ammo holding items and objects.
| [Magiclysm](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AMagiclysm)                                                                   | Anything to do with the Magiclysm mod
| [Map Memory](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Map+Memory%22)                                                           | Performance issues, weird behavior, suggestions on map memory feature
| [Mapgen](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AMapgen)                                                                         | Overmap, Mapgen, Map extras, Map display
| [Melee](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AMelee)                                                                           | Melee weapons, tactics, techniques, reach attack
| [Missions](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AMissions)                                                                     | Quests and missions
| [Mods](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AMods)                                                                             | Issues related to mods
| [Monsters](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AMonsters)                                                                     | Monsters both friendly or unfriendly.
| [Mutations / Traits / Professions](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Mutations+%2F+Traits+%2F+Professions%22)           | Mutations / Traits / Professions
| [NPC / Factions](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22NPC+%2F+Factions%22)                                                 | NPCs, AI, Speech, Factions
| [Organization](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AOrganization)                                                             | General development organization issues
| [Other](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AOther)                                                                           | Things that can't be classified anywhere else
| [Player Faction Base / Camp](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Player+Faction+Base+%2F+Camp%22)                         | All about player faction base/camp/site
| [Quality of Life](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Quality+of+Life%22)                                                 | QoL
| [Ranged](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3ARanged)                                                                         | Ranged (firearms, bows, crossbows, throwing), balance, tactics
| [Scenarios](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AScenarios)                                                                   | New Scenarios, balancing, bugs with scenarios
| [Sound Events](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Sound+Events%22)                                                       | Sound events handling in the game
| [Spawn](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3ASpawn)                                                                           | Monsters/Items appearing on map locations.
| [Temperature](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3ATemperature)                                                               | Freezing, cooling, thawing, heating, etc. mechanics
| [Time / Turns / Duration / Date](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Time+%2F+Turns+%2F+Duration+%2F+Date%22)             | Issues concerning any activities being too fast or too slow. Also issues about time and date ingame
| [Translation](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3ATranslation)                                                               | I18n
| [Vehicles](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AVehicles)                                                                     | Vehicles, parts, mechanics & interactions
| [Weather](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AWeather)                                                                       | Rain, snow, and non-temperature environment
| [Z-levels](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3AZ-levels)                                                                     | Levels below and above ground.

&nbsp;

### Flags

| Tag                                                                                                                                                                                             | Description
|---                                                                                                                                                                                              |---
| [Code: Astyle / Optimization / Static Analysis](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Code%3A+Astyle+%2F+Optimization+%2F+Static+Analysis%22)   | Code internal infrastructure and style
| [Code: Build](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Code%3A+Build%22)                                                                           | Issues regarding different builds and build environments
| [Code: Performance](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Code%3A+Performance%22)                                                               | Performance boosting code (CPU, memory, etc.)
| [Code: Tests](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Code%3A+Tests%22)                                                                           | Measurement, self-control, statistics, balancing.
| [Game: Balance](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Game%3A+Balance%22)                                                                       | Code that balances in-game features.
| [Game: Mechanics Change](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Game%3A+Mechanics+Change%22)                                                     | Code that changes how major features work
| [Organization: Bounty](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Organization%3A+Bounty%22)                                                         | Bounties for the claiming
| [Organization: Sites](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22Organization%3A+Sites%22)                                                           | Issues concerning web-sites that relate to Cataclysm
| [OS: Android](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22OS%3A+Android%22)                                                                           | Issues related to the Android operating system
| [OS: Linux](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22OS%3A+Linux%22)                                                                               | Issues related to the Linux operating system
| [OS: macOS](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22OS%3A+macOS%22)                                                                               | Issues related to the macOS operating system
| [OS: Windows](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22OS%3A+Windows%22)                                                                           | Issues related to the Windows operating system
| [(P1 - Critical)](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%28P1+-+Critical%29%22)                                                                 | Highest priority (for ex. crash fixes)
| [(P2 - High)](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%28P2+-+High%29%22)                                                                         | High priority (for ex. important bugfixes)
| [(P3 - Medium)](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%28P3+-+Medium%29%22)                                                                     | Medium (normal) priority
| [(P4 - Low)](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%28P4+-+Low%29%22)                                                                           | Low priority
| [(P5 - Long-term)](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%28P5+-+Long-term%29%22)                                                               | Long-term WIP, may stay on the list for a while.
| [PUBLIC TEST](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22PUBLIC+TEST%22)                                                                             | </div>
| [(S1 - Need confirmation)](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%28S1+-+Need+confirmation%29%22)                                               | Bug waiting on confirmation of reproducibility
| [(S2 - Confirmed)](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%28S2+-+Confirmed%29%22)                                                               | Bug that's been confirmed to exist
| [(S3 - Duplicate)](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%28S3+-+Duplicate%29%22)                                                               | Bug that is a duplicate of another one
| [(S4 - Invalid)](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22%28S4+-+Invalid%29%22)                                                                   | wontfix / can't reproduce
| [SDL: Tiles / Sound](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3A%22SDL%3A+Tiles+%2F+Sound%22)                                                           | Tiles visual interface and sounds.
| [stale](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3Astale)                                                                                               | Closed for lack of activity, but still valid.
| [weekly-digest](https://github.com/CleverRaven/Cataclysm-DDA/issues?q=is%3Aopen+is%3Aissue+label%3Aweekly-digest)                                                                               | </div>

