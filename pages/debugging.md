---
layout: page
title: ""
---

## In-game testing, test environment and the debug menu

Whether you are implementing a new feature or whether you are fixing a bug, it is always a good practice to test your changes in-game. It can be a hard task to create the exact conditions by playing a normal game to be able to test your changes, which is why there is a debug menu. There is no default key to bring up the menu so you will need to assign one first.

Bring up the keybindings menu (press `Escape` then `1`), scroll down almost to the bottom and press `+` to add a new key binding. Press the letter that corresponds to the *Debug menu* item, then press the key you want to use to bring up the debug menu. To test your changes, create a new world with a new character. Once you are in that world, press the key you just assigned for the debug menu and you should see something like this:

```
┌────────────────────────────────────────────┐
│ Debug Functions - Using these is CHEATING! │
├────────────────────────────────────────────┤
│ 1 Wish for an item                         │
│ 2 Teleport - Short Range                   │
│ 3 Teleport - Long Range                    │
│ 4 Reveal map                               │
│ 5 Spawn NPC                                │
│ 6 Spawn Monster                            │
│ 7 Check game state...                      │
│ 8 Kill NPCs                                │
│ 9 Mutate                                   │
│ 0 Spawn a vehicle                          │
│ a Change all skills                        │
│ b Learn all melee styles                   │
│ c Unlock all recipes                       │
│ d Edit player/NPC                          │
│ e Spawn Artifact                           │
│ f Spawn Clairvoyance Artifact              │
│ g Map editor                               │
│ h Change weather                           │
│ i Remove all monsters                      │
│ j Display hordes                           │
│ k Test Item Group                          │
│ l Damage Self                              │
│ m Show Sound Clustering                    │
│ n Lua Command                              │
│ o Display weather                          │
│ p Change time                              │
│ q Set automove route                       │
│ r Show mutation category levels            │
│ s Cancel                                   │
└────────────────────────────────────────────┘
```

With these commands, you should be able to recreate the proper conditions to test your changes. You can find some more information about the debug menu on [the official wiki](http://www.wiki.cataclysmdda.com/index.php?title=Cheating#The_Debug_Menu).

