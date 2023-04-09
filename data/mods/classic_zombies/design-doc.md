# The DDotDDD - Dark Days of the Dead Design Document

## Mission Statement
Dark days of the dead is a Cataclysm full conversion mod meant to turn the kitchen sink of Cataclysm: DDA into a classic Romero-style zombie story.

## Gameplay
DDotD, like CDDA, aims for verisimilitude and the feeling of being a survivor in a Romero zombie movie. Like Dawn of the Dead, zombies should represent less of an individual deadly threat, and more of an unstoppable glacial force. Survivors should be poorly prepared and weak.

### Desired changes to monster stats
Only classic zombies are allowed in DDotD. These zombies are slow moving shamblers. They have an enormous pool of hit points, but no armor. They are walking corpses, feeling no pain and continuing on in the face of incredible injuries. They can be destroyed with catastrophic damage, by completely destroying their bodies (represented by depleting their very large HP pool) or by a headshot. A deep bite from a zombie should have a very high (about 80%) chance of developing Conversion, an effect that kills you in about 3 days. (We should add an achievement for things accomplished in the time after getting Conversion). Scratches that pierce armor should have a low (about 10%) chance of causing Conversion, and gives an alternate form that takes longer, about 5 days, to kill you.

Killed zombies do not revive. Some, but not all, human corpses should revive as zombies though. These zombies do not evolve. However it should be possible to destroy body parts and get things like crawling zombies as a result.

DDotD zombies *are* capable of learning. At some point we may add our own late stage evolution to introduce zombies that can open doors or use simple tools.

There are no aliens or interdimensional threats in DDotD.

## Setting
The setting of DDotD should be vaguely "in the past", some time between 1980 and 2000. DDotD takes place in Alberta, Canada, in a fictionalized version of the Red Deer area. Why Alberta? Because the current CDDA mapgen of small clustered towns surrounded by fields and patches of forests on mostly flat land suits Alberta perfectly. This also allows us to have harsher winters to survive. Most buildings and loot in CDDA core will be appropriate, but guns and gun stores (with the exception of hunting rifles) are somewhat less common, and military presence substantially less so.

### Changes to in-game stuff for setting reasons
#### Items and spawns
Most changes will be for setting reasons, not balance. We will mostly use the CDDA philosophy that real life is fairly well balanced, and go from there. However as our setting is in some vague timeframe between the 80's and early 2000's, and set in Canada, we will need a number of changes to all sorts of spawns to bring them in line. Some examples would include:
- Electric cars should not exist, and cordless tools need substantially less battery life or shouldn't exist at all.
- Solar power is much rarer and less efficient.
- Cell phones exist but do nothing now that the grid is down. They have no flashlight, no camera.
- Laptops are considerably rarer.
- Assault weapons and pistols are considerably rarer. Gun stores are extremely rare although sporting goods stores are not.
- Police are less well armed.
- The military has much less presence.
- Healthcare and health insurance costs are not a concern for most people.
- SUVs and hatchbacks are less common. Compact cars are less common due to the location. Pickup trucks are much more common.
- Many location names should be changed to Canadian equivalents, or sometimes removed to be later replaced with Canadian versions as appropriate. For example, the cdda "state park" map is a tiny fenced-off area with no resemblance to a Canadian provincial park.
- Where possible we should try to audit use of terrible American spelling and use Canadian instead. Luckily, Canada formally accepts both spellings so we don't have to be complete about that, but using the proper spelling of things will color the setting nicely.
- Plant frequencies should be adjusted to suit hardiness zone 4a. By comparison, Massachusetts is a 7a hardiness zone, almost tropical. You can't grow peanuts in Red Deer. As well, certain plants are just rarer in Canada: even though Kentucky coffee could technically grow in Alberta, I don't believe you'd likely ever find a tree of it. As the area is less developed, fruit trees growing wild in forests are a lot less common as well. At some point some plants should be refluffed with minimal changes - for example, the cottonwood tree should be changed to the Balsam Poplar, a close relative found throughout Alberta. We should add wild roses and wild strawberries, and make them very common. Cherries should be changed to sour cherries, the Evans cherry is a similar Alberta-friendly variant. We'll need crabapples too. And spruce trees. It's going to be a whole thing.

#### Professions and Scenarios
The "default start" should put you in a cabin in the woods with one other survivor that already knows you and can tell you a bit about the setting. The rest of the start options should fit one of the two following categories:

##### Movie-inspired starts
These should be based on characters and starting scenarios from classic zombie movies, particularly the "of the dead" franchise, but also similar ones like Zombieland. For example, a Zombieland set of professions would have you starting in one of three groups: A nerdy survivor, a hardened hick survivor, or a cunning older sister with her younger sister starting as a faction member. These starts will not generally have any associated story and will have the current open world gameplay, although later we may add short starting storylines to get you to do things like gather up other survivors that match the movie setting you are from.

##### Story starts
These starts are a bit more like current cdda, with a wider option of starting classes, but they will be more heavily curated with a specific questline from the very beginning. You will start with a quest and proceed from there along a storyline. Of course, this being CDDA at heart, you can just ignore that storyline and go do other stuff.

### Zombification Mechanics
The zombie affliction is infectious, possibly viral or fungal. Survivors may never find out. It comes in two forms: it transmits by airborne spores, which have a low infection rate and a long latency. After infection, carriers are very quickly able to infect others, but do not develop symptoms for 2-3 weeks. Once symptomatic it progresses quickly, with death usually occurring in another week. The infectious symptoms are vague and flu-like: fever, chills, severe nausea and vomiting, muscle aches, muscle spasms, and eventually delirium and death. Due to the relatively low infection rate of the airborne form, only around half of the population were susceptible to this form, and all survivors are clearly among the immune group. However, the much higher dose of infectious agent shared by bite or even scratch from a reanimated zombie carries an increased risk of infection and generates a much faster progressing form of the illness.

During the infection process and especially after death, the victim's body is being infiltrated by an expanding web of loosely connected parasitic tissue that spreads through muscles and other organs. This creates a new superorganism which eventually functions as a new nervous system, controlling muscles and using the sensory input from the brain and attached organs. It laces through the victim's digestive tract and will continue to digest things ingested by the victim. It does the work of keeping the reanimated body intact and "alive". Destroying the victim's brain removes any way for the parasite to get sensory input, rendering it inert.

(Note that in my head, this infectious agent is too perfect, so it probably has to be some kind of alien invader. However, there's no way the survivors will ever find out, and I'd rather not establish it within game.)

### History
The source of the zombie plague is unclear. About a year ago, rumors arose of a horrible rabies variant arising in Australia and carried by wombats. This strain had jumped to humans and was baffling and frightening doctors. However, those stories died down, and about three months later it was announced to be a novel strain that couldn't transmit significantly in humans.

Four months ago, new reports of the same effect came up almost simultaneously in Morocco, Belarus, Vietnam, and Portugal. The WHO expressed concern and gave advice for avoiding wild animals. Within a month, the plague had spread to most of the planet. The initial caution about wild animals proved a dangerous red herring, causing people to cluster in urban centers, where the actual vector - other humans and some infected vermin - spread the zombie affliction rapidly. Unfortunately the long latency of the agent meant that it was another month before the ubiquity of the infection became apparent. By the time the second wave of infections became clear, millions of people had been infected, and already passed it to the currently unaware third wave. Hospitals were rapidly overwhelmed, before the revivification risk was even clear. Critically, police and military were among those affected by the unrecognized first wave, and their ranks were as decimated as civilians.

The first wave of zombies further spread the illness due to unreadiness and the population already being overwhelmed by the new pandemic. The death toll was catastrophic, particularly among medical personnel caring for those infected with the illness. It isn't known exactly what went wrong with military efforts, but they were undoubtedly stymied by their own ranks falling ill with the airborne form of the pandemic even as they tried to fight the zombies.

Within three months, the world was collapsing. In the game area of Central Alberta, a wave of refugees from overrun urban centers in the South, especially the US, began flooding the highways, attempting to get away from the hell occurring in more populous regions. Instead of escaping, they brought it with them. Survivors banded together in small groups, now aware of the signs and symptoms of the illness and the danger of a bite. You are one of those survivors. For the last few weeks you've been somewhere safe, holed up, waiting for things to die down, but it's clear now that things aren't going to die down.

### Factions
The default CDDA factions do not fit with DDotD without significant change. It is possible that some, like Isherwood, could be rewritten and readded.

#### The Montana Army National Guard
An early faction I'd like to see: the Montana National Guard should have a forward base in Alberta. This group was sent up in the later part of the apocalypse to aid Canada's self-defense and try (ineffectively) to corral Americans fleeing North. Within a few days, they lost contact with command. They have chosen to establish a reinforced base, and are fighting off zombies and attempting to rally survivors. They may, at some point, change their name once their makeup is less than half American. They have been here for weeks and are running low on supplies.

#### The Central Alberta Historical Re-Enactors Army
CAHREA was a local club of creative anachronists. By some coincidence, a large number of their members were immune to the initial zombie infection. Their access to armor that is additionally protective against zombie bites gave them an edge, and they began helping evacuate and protect survivors in a rural community center. Although the original CAHREA group is now a small minority, they have maintained the name.

#### The Wild Rose Survivors
This is a group of rural Albertans less hard-hit by the pandemic portion of the plague due to their remote and spaced-out living. They were still eventually affected as the plague became widespread. They tend to be well equipped with pickup trucks and hunting rifles, somewhat insular, but welcoming - if a bit suspicious - of other survivors.
