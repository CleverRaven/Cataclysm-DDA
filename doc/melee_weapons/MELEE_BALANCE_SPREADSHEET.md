## Overview
This is documentation for the 'Melee Weapons Evaluation.md' spreadsheet.  This is the spreadsheet that drove the 0.E experimental melee weapon rebalance project.  The original spreadsheet is available as a [google doc](https://docs.google.com/spreadsheets/d/14eQIe4AO_f6OxCt1XcB4NLAs6-5R1wQW-ydZG0orYdY/edit#gid=1787713396) but the static copy is preserved in case stuff moves.

This is a complicated spreadsheet, with a lot of moving parts. The following are the tabs and the data flow:

### Raw
Raw was the initial data dump, taken from some October 2019 version of the 0.D experimental using mlangsdorf's usual mods (hence all the Blazemod stuff).  Raw was slightly annoyed with some extra categories:
* Weapon Class - this is a simple numeric rating.  0 is not a weapon, 1 is an explosive device, 2 is a ranged weapon or gun, 3 is a tool that makes a poor weapon like a cooking pot, 4 is a helmet, 5 is a tool that makes a good weapon like a sledgehammer, 6 is an improvised or fake weapon, and 7 is an actual dedicated weapon
* Category - this is a categorization of items as weapons, to make it easier to compare similar weapons.  Melee weapon categories are Ax, Ax2, Club, Club2, Flail, Flail2, Knife, Polearm, Spear, Staff, Shortsword, Sword, and Sword2.  Categories that end with 2 are two-handed weapons.
* Acc - this is a recaluclation of weapon accuracy based on the accuracy factors below, because review of the data showed that a lot of weapons have accuracy values that are not supported by the criteria in GAME_BALANCE.md.
* Grip - the item's grip, as described in GAME_BALANCE.md
* Leng - the item's leng, as described in GAME_BALANCE.md
* Surf - the item's striking surface, as described in GAME_BALANCE.md.
* Bal - the item's balance, as described in GAME_BALANCE.md.  Additional categories beyond Clumsy were added and assigned by eye.

### Filter
Filter takes the initial data from raw and a weapon class (in cell B1) and filters out items with a lower weapon class to make further analysis easier.

### New Formula
This calculates the new weapon evaluation formaula on the inital stats.  This is complicated.

The first 7 rows have some header data.  The bulk of the calculation starts on row 8.

The weapons were evaluated using a Strength 10, Dexterity 10, Perception 10 survivor with skill 4 in all weapons.  The base hit, stat crit, skill crit, bash mult, cut mult, and stab mult values are derived from the relevant bits of src/melee.cpp.

* Columnn A "Average" is the new weapon evaluation value for the weapon, on the edge of the sheet for easy reference.
* Columns B-J are the weapon's original stats, taken from the Raw tab via the Filter tab.
* Column K "roll_hit" is sum of the evaluator's base_hit and the weapon's acc.
* Column L "Wpn Crit" is weapon's contribution to critical hits.
* Columns M-N "3 Crit" and "2 Crit" are the chances of a triple and double critical hit occuring, based on skill, accuracy, and stats.
* Columns O-Q "Average Non-Crit" damage are the weapon's calculated average damage before armor for non-critical hits in each of the 3 categories.

All that is pretty straightforward.  The next three blocks are where it gets complicated.  Columns S-Z are repeated as AB-AI and AK-AR with different monster stats.
* Row S "Hits" is the expected number of hits per 1000 attacks, calculated against a normal distribution with a mean value of 5 * ( roll_hit - monster Dodge ) and a standard deviation of 25.
* Row T "Crit %s" is the percentage of those hits that should be critical hits.
* Row U "Dmg" is the average expected damage past armor.  This is simply the sum of max( 0, damage type - armor amount ) for each of the 3 damage types.
* Row V "Crit" is the average expected critical damage past armor.  This is the sum of the critical damage - armor amount for each of the three damage amounts, but the formula is substantially more complicated because the 3 damage types have different critical damage multipliers and reduce effective armor by different amounts on a critical.
* Rows W-Y repeat damage and critical damage, but for rapid strikes where the base damage is multiplied by 2/3rds.
* Row Z "Dmg/Turn" is either:
** 100 * ( Dmg * ( Hits - num crits ) + Crit Dmg * ( num hits ) ) / ( 1000 * Moves ) - ie, sum of damage per hit for each normal hit + crit damage for each critical hit divided by the number of moves in 1000 attacks, multiplied by 100 moves/second, OR
** 100 * ( Dmg * ( Hits - num crits ) / 2 + Crit Dmg * ( num hits ) / 2 + rapid strike Dmg * ( Hits - num crits ) / 2 + rapid strike Crit Dmg * ( num hits ) / 2 ) / ( 1000 - ( hits / 2 ) * Moves + Hits * 0.33 * Moves ) - same as above, but accounting for rapid strike reduced damage and movement cost.

Finally,
* Column AT "Weapon" is a repeat of the weapon name for refernce
* Column Au "Value" is the average of columns Z, AI, and AR, multiplied by 1.5 for Reach 2 weapons and 1.75 for Reach 3 weapons.

### New Formula Sorted
This compares the old weapon values versus the values from the new formula, and sorts weapons by decreasing new value by weapon category to make it easier to spot weapons that are unsually good (I'm looking at you, broadsword) or bad for their category.

### Proposed Values
This repeats the New Formula tab, except that Columns C-J were copied over and pasted as values, and then adjusted to make the numbers nice.

* Accuracy was adjusted to the new values based on grip, length, striking surface, and balance from the Raw tab.  That means a bunch of previously accurate weapons like bionic claws got a nerf, and inappropriately inaccurate weapons like katanas got a buff.
* Weapons in the same category were generally adjusted to have roughly the same evaluation, though with different values.  In general, European weapons get more of their damage from bash and less from cut, and are often heavier and slower, than Asian weapons.  This is a little arbitrary and not entirely realistic, but everyone seems to expect it.  As a case in point, the Japanese two-handed sword "nodachi" has 164 attack speed, bash 6, and cut 47, while the European two-handed sword zweihander has 169 attack speed, base 18, cut 39.  They both have values of about 26.
* Inferior weapons got a 1 step penalty in balance compared to the real weapon, and half of the cut/pierce damage is moved to bash, and the cut/pierce damage is 1/4th's the real weapons.  Ie, an inferior pike does Bash 30, Stab 11 to a pike's Bash 8, Stab 44.
* Fake weapons are as badly balanced as the inferior version of a weapon, but are basically blunt so the striking surface usually got upgraded to "every", ironically making them as accurate or more accurate than the real version of the weapon.  Bash damage was halved from the inferior version, and cut/pierce damage was again reduced to 1/4th of the inferior version, which is 1/16th the real version.  A fake pike does Bash 15, Stab 3.
* A lot of weapons got damage boosts to bring up their evaluated value.  A lot of low damage weapons with rapid strike were being drastically overvalued, but the damage past armor tests showed that rapid strike often just lets you bounce off armor twice as fast.
* Spears got some rough formula for pierce damage based on weight that I can't recover anymore, but in general the differences between spears are more minor than they used to be.
* Polearms got the same rough damage as Ax2 at range 2, but got a separate balance line at range 1 with the raw damage reduced by 0.7.  This makes polearms very impressive at range, but slightly worse than quarterstaffs against adjacent targets.

### Poposed Values Sorted
This is another comparison tab like New Formula Sorted, but used the data from Proposed Values.

### Comparison
This is a summary tab.

In columns A-P, each weapon from the filter tab gets it's current and proposed stats, it's original value, value under the new formula, and value under the new formula using the proposed values.  For proposed values, values that improved are highlighted in green and those that got worse are in red.  Ideally, this will make it easy to compare changes and catch mistakes.

Columns S-AH repeat the process, but cells S1, T1, U1, and V1 can be used to specify category names, and then the subtotal shows a selected subset of weapons that match those categories.  This simplifies comparing things.
