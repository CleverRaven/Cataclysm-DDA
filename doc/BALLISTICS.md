# Ballistics in Cataclysm DDA

## Dispersion

> "The normal, a.k.a. Gaussian, distribution is the broadly accepted model of a random variable like the dispersion of a physical gunshot from its center point."
> - [http://ballistipedia.com/index.php?title=Precision_Models](http://ballistipedia.com/index.php?title=Precision_Models)

When shooting at a target, the chance to hit is calculated based on the distance to the target and the accuracy of the shot.  The accuracy of the shot is limited by the firearm and by several other factors, including the skill of the character, encumbrance, the dexterity and perception attributes, etc.  "Dispersion" is the spread or deviation from the intended target caused by these factors.

In the real world dispersion can have a random error component and also a systematic bias.  For example, if someone always pull to the right when they squeeze the trigger, that would be a systematic error that causes dispersion in the same direction every time.  Cataclysm DDA only models random dispersion centered around the intended target.

There are many contributing factors to why shots do not hit the same place every time.  The steadiness of the aim, the shooter's ability to see clearly, and the limitations of the gun itself in how it handles.  Whatever the sources, when they are all combined, the result is a normal distribution.  Shots can miss high or low, left or right, and the "cluster" of shots gets denser closer to the middle.  This follows a normal distribution or Gaussian distribution.  Cataclysm DDA currently doesn't model high or low when shooting, but left and right are captured and do follow a normal distribution.  Normal distributions are defined by a "mean" and a "standard deviation", and they look like this:

![Normal Distribution][nd]

68% of the time, shots will land within 1 standard deviation of the target.  So the width of the distribution, and thus the accuracy of the shot, is defined by how big the standard deviation is.  In Cataclysm, the units for measuring the standard deviation are "minutes of arc" (MoA).  60 MoA is equal to 1 degree.  Put in another way, at a distance of 100 yards from a target, one MoA corresponds to just over 1 inch.

### Calculating a hit

Whenever a character in Cataclysm DDA fires a weapon, the process starts first by taking aim.  The mechanics of how aiming works are not covered in detail here, but the jist is that there is a game varialbe called "recoil" that starts out large and goes down to zero the longer you aim.  "Recoil", like all other dispersion related variables discussed here, has units of minutes of arc (MoA).  Once the character has finished aiming and decides to shoot, any remaining "recoil" is added to the other sources of dispersion (discussed below), and that sum becomes the standard deviation of the normal distribution.  A random number is chosen against that normal distribution to represent the trajectory of this specific shot.  Finally, the game looks at the geometry of how far away the target is, how big the target is, and the dispersion value for this specific shot to decide if it is a hit.  A shot that hits the center 10% of the target is considered a "critical hit".  Shots that barely hit are "grazing hits".

The sources of dispersion for calculating shot trajectory are:

* recoil - Recoil in Cataclysm DDA does not follow the dictionary meaning of the word.  Recoil here is the variable name representing how well the shooter has taken aim.  After every shot or when drawing a weapon, this value starts as a high number, and it essentially limits the rate of fire.  Taking aim can reduce this number to zero.
* weapon dispersion - the firearm the shooter is weilding will have a dispersion value as a property of the weapon.  Smaller numbers are better.  Modifications, such as scopes, can reduce the dispersion of the weapon.  At the time of writing, the code actually divides the weapon dispersion by ~15 before making shot calculations as part of a game balance effort.  While the specifics may change, the gameplay effect is that firearms with higher dispersion will be limited to shorter effective ranges.
* dexterity - Dexterity attributes below 20 result in a very small penalty to dispersion.  At the time of writing, it is small enough that it would only effect very long-range shots by very skilled shooters.
* arm encumbrance - Another very small source of dispersion, there is a penalty of `(left arm encumbrance + right arm encumbrance) / 5` that is added to the standard deviation of the shot.  Keep in mind that it always considers both arms, so carrying a swag bag over one arm and a pistol in the other hand is not a good idea in the cataclysm.
* skill - Weapon skill is a large contributer to dispersion.  The marksmanship skill level and the skill level for the specific weapon used (e.g. pistols) are averaged together.  At an average of level 10, there is no penalty.  At level 0, there is a pretty large penalty that would preclude long-range shots of any kind.


[nd]: https://upload.wikimedia.org/wikipedia/commons/a/a9/Empirical_Rule.PNG "Dan Kernler [CC BY-SA 4.0 (https://creativecommons.org/licenses/by-sa/4.0)]"