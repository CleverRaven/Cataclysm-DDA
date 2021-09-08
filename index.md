---
layout: page
title: "Cataclysm: Dark Days Ahead"
---

Cataclysm: Dark Days Ahead is a turn-based survival game set in a post-apocalyptic world. Struggle to survive in a harsh, persistent, procedurally generated world. Scavenge the remnants of a dead civilization for food, equipment, or, if you are lucky, a vehicle with a full tank of gas to get you the hell out of Dodge. Fight to defeat or escape from a wide variety of powerful monstrosities, from zombies to giant insects to killer robots and things far stranger and deadlier, and against the others like yourself, that want what you have...

<figure>
  <img src="assets/images/showcase-ultica.png" alt="Showcase image. Tileset: UltiCa">
  <figcaption>
    <em>Tileset: UltiCa</em>
  </figcaption>
</figure>

## Gameplay

As your game begins, you awaken with hazy memories of violence and terror from when the world suddenly unravelled around you.  Now you need to explore your surroundings, and secure food, water and safety.  After that, who knows?  Long term survival will mean tapping abilities you haven't used before, learning to survive in this new environment, and developing new skills.

## Project Managed Resources
These sites are owned and managed by the project directly.
* [GitHub repository](https://github.com/CleverRaven/Cataclysm-DDA)
* [Forums](https://discourse.cataclysmdda.org)
* [Development-oriented Discord](https://discord.gg/jFEc7Yp)
* IRC channel: irc.libera.chat channel: #CataclysmDDA [webchat](https://kiwiirc.com/nextclient/irc.libera.chat#CataclysmDDA)

## Community Managed Resources
These are resources provided by third parties that the project sees as helpful to users.
* [Wiki](http://cddawiki.chezzo.com/cdda_wiki/index.php)
* [Item Browser](http://cdda-trunk.chezzo.com/)
* [Hitchhiker's Guide to the Cataclysm](https://nornagon.github.io/cdda-guide/)
* [Game Launcher](https://discourse.cataclysmdda.org/t/cdda-game-launcher-automatic-updates-and-more)

## System Requirements
### Minimum
"I played catadda 0.B on Lego EV3 brick through ssh about a year ago. Every simple move took a noticeable fraction of a second (like 0.3-0.5 second), crafting and such was very slow as well. The specs were - 300 MhZ CPU, 64 Mb Ram (+ another 64 Mb swap on microsd flash) running ev3dev (stripped ubuntu basically), ssh over USB CDC connection." -burgerpro

"I used to test the game on an Asus EEE 700. That's a 900MHz mobile cpu with 500MB of ram." -Kevin Granade

Tilesets increase the amount of RAM required dramatically. I recommend at least 1GB of ram for tilesets, and there are reports of instability due to RAM exhaustion when certain tilesets are used. (The larger the size of the individual tiles, the more RAM is used)

### Recommended
"I regularly test the game now on a Windows netbook with a sub 2GHz single core CPU, 1GB of ram, and no graphics acceleration." -Kevin Granade

Graphical builds run much faster with some kind of graphics acceleration.  2GB of RAM pretty much guarantees you will never have trouble with memory shortages.

## Downloads

Cataclysm has official builds for Windows, Linux, OSX and Android.  
The Linux and OSX builds are available in both terminal and graphical flavors.  
Windows and Android builds are graphical only, though there is a "text rendering" mode in the graphical build.  
There is a very old and unsupported fork for iOS.

### Latest Stable
The most recent stable release is {{ site.github.latest_release.tag_name }} {{ site.github.latest_release.name }}.  We recommend stable for the most bug-free experience available.  You can download it from the [Releases Page]({{ site.baseurl }}{% link releases.md %}).

### Latest Experimental
Cataclysm: DDA is under constant development. As such, stable can lag behind with features. If you would like access to the bleeding edge of features and any bugs that may come along with them, you can download the [latest experimental builds]({{ site.baseurl }}{% link pages/experimental.md %}).

## Frequently Asked Questions

#### Is there a tutorial?

Yes, you can find the tutorial in the **Special** menu at the main menu. You can also access documentation in-game via the `?` key.

#### How can I change the key bindings?

Press the `?` key, followed by the `1` key to see the full list of key commands. Press the `+` key to add a key binding, select which action with the corresponding letter key `a-w`, and then the key you wish to assign to that action.

#### How can I start a new world?

**World** on the main menu will generate a fresh world for you. Select **Create World**.

#### I've found a bug / I would like to make a suggestion. What should I do?

Please submit an issue on [our GitHub page](https://github.com/CleverRaven/Cataclysm-DDA/issues/). If you're not able to, send an email to `kevin.granade@gmail.com`.  In either case, please consider reading our [issue guidelines]({{ site.baseurl }}{% link pages/issues.md %}) first.

#### I'd like to contribute financially to Cataclysm: Dark Days Ahead

Check out [our page on BountySource](https://www.bountysource.com/teams/cataclysm-dda) and consider sponsoring an issue.
