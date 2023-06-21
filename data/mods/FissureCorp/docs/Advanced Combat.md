## Advanced Combat

[Melee](data/mods/FissureCorp/docs/The%20Idea.md#Melee) in the mod is higher fidelity.

Characters can select their attacks. On each turn you are presented with 5 options
* An offence focused tech
* Any tech
* A defense focused tech
* Mulligan
* Disengage

### Combat Stats
Your character will have a few values that they can roll to determine how effective they are at certain moves.
* Balance
* Force
* Dodge
* Skill(?)

### Combat States
Characters in combat can have a number of states attached to themselves which effect what move they can do, what their opponent can do.

| State     | Balance | Dodge | Force | Skill |
| --------- | ------- | ----- | ----- | ----- |
| Moving    | -       | +     |       |       |
| Evasive   | +       | +     | -     |       |
| Readied   | +       |       |       | +     |
| Downed    | -       | 0     | -     | -     |
| Stumbling | -       | -     | -     | -     |
| Panicked  | -       | +     | -     | -     |
| Blocking  | +       | -     | 0     | +     |
| Grappling | -       | 0     | -     |       |


#### Special States
These don't have stats related just effect what moves can happen
In Combat - You are in melee with something and can't move.
Comboing - You are setting up a bigger attack.
Outnumbered - Outnumbered, dangerous attacks can happen.
Close - Enemy is close to you.
Far - Enemy is far from you.
Offhand Occupied - You can't do 2handed attacks or otherwise use your offhand.
Weapon Stuck - You can't use your primary weapon at all.
Moving Direction - Which way you are traveling, towards, away, perpendicular.

### General Martial Art
No moves work while downed unless specified

| Move     | Type | Required Tags  | Self Tags | Enemy Tags         | Description                                                                                |
| -------- | ---- | -------------- | --------- | ------------------ | ------------------------------------------------------------------------------------------ |
| Roll Up  | Def  | Downed         | Evasive   |                    | Move to an open space and stop being on the ground.                                        |
| Tackle   | Off  | Moving Towards | Downed    | Downed             | Knock an enemy to the ground, potentially falling over in the process                      |
| Grab     | Off  |                | Grappling | Grappling          | You get in close and try to grab the enemy                                                 |
| Jab      | Off  | No Weapon,     |           | Close              | Get in close and jab at the enemy                                                          |
| Spacing  | Def  | Close          | Readied   | Far                | Space away from the enemy, with an arm outstretched ready to strike if they close distance |
| Haymaker | Off  | No Weapon, Far |           | Stumbling / Downed | Step forward crushing the enemy with a massive hit                                         | 
| Knee     | Off  | Grappling      |           | Stumbling          |                                                                                            |



#### Tackle
*Offensive* 


