# Nether Attunement
Nether attunement is gained primarily from activating or concentrating on powers. Nether attunement is tracked via a vitamin and each rank of nether attunement corresponds to a certain amount of the attunement vitamin. Overall gain is not linear, when you have no attunement ranks, vitamin gain is slower but when you have any ranks of attunement, you gain more vitamin, faster.

## Chance to Gain Attunement
Every time you activate a power, you have a chance to gain attunement vitamin based on the difficulty of the power, how many powers you activated recently and how many powers you are concentrating on. The base chance is the difficulty of the power squared, modified by the other factors. When you have no ranks of attunement, the chance to gain attunement vitamin based on recent power usage is lower.

## Amount of Attunement Gained
If this check passes, you then gain some amount of attunement vitamin that is controlled by a variety of factors. When you have no ranks of attunement, you gain attunement vitamin based on the number of powers you are concentrating on plus a small random amount. When you have any ranks of attunement, you will gain attunement vitamin based on the difficulty of the power, your attunement rank and the number of powers you are concentrating on. This latter part is why you gain later attunement ranks faster, the more attunement ranks you have the more attunement vitamin you gain in a self amplifying cycle.
When you have extended channeling or noetic resilience active, you will gain less attunement vitamin from power usage and the attunement vitamin you do gain is not affected by your current attunement rank, protecting you from the self amplifying effect of attunement.

## Attunement Power Scaling
All powers scale based on your attunement vitamin. When you have no ranks, your powers are 75% as effective. At maximum attunement rank, your powers are 300% as effective. Ranks 1-12 are visible in game on the attunement sidebar widget and in the `@` screen as the `Nether Attunement [#]` effect.
This power scaling applies to the effects of powers as a multiplier and to your attunement vitamin gain when you have any attunement ranks as mentioned above.

| Attunement Rank | Multiplier | with Noetic Resilience | with Torrential Channeling |
| :--: | :--: | :--: | :--: |
|  00  | 0.75 | 1.05 | 1.60 |
|  01  | 1.00 | 1.05 | 1.60 |
|  02  | 1.04 | 1.05 | 1.60 |
|  03  | 1.08 | 1.05 | 1.60 |
|  04  | 1.13 | 1.05 | 1.60 |
|  05  | 1.19 | 1.05 | 1.60 |
|  06  | 1.26 | 1.05 | 1.60 |
|  07  | 1.35 | 1.05 | 1.60 |
|  08  | 1.45 | 1.05 | 1.60 |
|  09  | 1.60 | 1.05 | 1.60 |
|  10  | 1.80 | 1.05 | 1.80 |
|  11  | 2.10 | 1.05 | 2.10 |
|  12  | 3.00 | 1.05 | 3.00 |
|  12* | 4.00 | 1.05 | 4.00 |

There is actually a vitamin level past attunement rank 12 where your attunement power scaling goes up to 400%. You have no visual indicator of reaching this point.

## Attunement Consequences
Whenever you activate a power and you have any ranks of attunement, you will trigger a possible consequence. The specific consequence is pulled from a weighted list, shown below, and if your attunement vitamin is not high enough, a new selection is made from the same list until one is selected that you do have the minimum attunement vitamin for. Some of the consequences are power or power path specific and if you select one that does not apply, a new one will be selected as before.
Once a consequence is selected, it has a chance to actually be applied. There is a base percentage chance plus an increased chance based on two factors: 
- Current attunement vitamin
  - The higher your attunement vitamin, the greater the chance. This chance is scaled at different rates depending on how high your attunement vitamin is and varies from consequence to consequence.
- Difficulty of power used
  - The formula for this `(difficulty ^ 2) / 10` and is the same for every consequence. Stronger powers come with more chance at consequences

| Consequence | Weight |
| :--- | ---: |
| [Headache](#headache) | 12 |
| [Extra Attunement](#extra-attunement) | 9 |
| [Cold Wind](#cold-wind) | 6 |
| [Health Change](#health-change) | 6 |
| [Vomit](#vomit) | 9 | 
| [Nosebleed](#nosebleed) | 12 |
| [Stamina Loss](#stamina-loss) | 8 |
| [Power Surge](#power-surge)+ | 5 |
| [Sleepiness](#sleepiness) | 5 |
| [Nether Conduit](#nether-conduit) | 9 |
| [Feedback](#feedback) | 9 |
| [Observed](#observed)+ | 6 |
| [Incorporeality](#incorporeality) | 4 |
| [Teleport Lock](#teleport-lock) | 5 |
| [Metabolic Inversion](#metabolic-inversion) | 5 |
| [Power Drain](#power-drain) | 5 |
| [Weakness](#weakness) | 5 |
| [KCal Consumption](#kcal-consumption) | 6 |
| [Attenuation](#attenuation)+ | 8 |
| [Short of Breath](#short-of-breath) | 5 |
| [Force Wave](#force-wave) | 5 |
| [Teleport Misjump](#teleport-misjump) | 4 |
| [EMP Blast](#emp-blast) | 3 |
| [Lightning Blast](#lightning-blast)+ | 3 |
| [Crack in Reality](#crack-in-reality)+ | 2 |
| [Nullified](#nullified) | 3 |
| [Hounds of Tindalos](#hounds-of-tindalos)* | 2 |
| [Mutation](#mutation) | 2 |
| [Reality Tear](#reality-tear)* | 1 |

\* require you to be under the effect of Nether Conduit or Observed for them to trigger.  
\+ are possible whenever you have Torrential Channeling active regardless of your attunement level

## Consequence Descriptions
The effect of many consequences are amplified by your current attunement vitamin, in severity and/or in length of the effect. Chances listed below do not take into account the increased chance based on the difficulty of the power used.

### Headache
Minimum Attunement: 1  
Chance: 0.5% to 33%  
Description: You gain a headache that lasts based on the strength of your attunement  
<sub>[Back to List](#attunement-consequences)</sub>

### Extra Attunement
Minimum Attunement: 1  
Chance: 2% to 32%  
Description: You gain a small random amount of attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Cold Wind
Minimum Attunement: 1  
Chance: 3% to 41%  
Description: You emit a blast of cold air, chilling you and your surroundings  
<sub>[Back to List](#attunement-consequences)</sub>

### Health Change
Minimum Attunement: 1  
Chance: 1.5% to 32.5%  
Description: Your health goes up or down by a small amount  
<sub>[Back to List](#attunement-consequences)</sub>

### Vomit
Minimum Attunement: 2  
Chance: 2% to 35.75%  
Description: You vomit on yourself and surroundings  
<sub>[Back to List](#attunement-consequences)</sub>

### Nosebleed
Minimum Attunement: 1  
Chance: 1% to 37.25%  
Description: You develop a nosebleed  
<sub>[Back to List](#attunement-consequences)</sub>

### Stamina Loss
Minimum Attunement: 2  
Chance: 2% to 31%  
Description: You lose a random amount of stamina  
<sub>[Back to List](#attunement-consequences)</sub>

### Power Surge
Minimum Attunement: 4  
Chance: 1.5% to 38%  
Description: Your effective level is increased by three for all powers for a random period of time based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Sleepiness
Minimum Attunement: 3  
Chance: 2% to 48.25%  
Description: You gain a random amount of sleepiness, feeling more tired  
<sub>[Back to List](#attunement-consequences)</sub>

### Nether Conduit
Minimum Attunement: 3  
Chance: 3% to 38.25%  
Description: You gain an effect that unlocks nastier consequences and slowly increases your attunement vitamin for a random period of time based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Feedback
Minimum Attunement: 5  
Chance: 3% to 39.5%  
Description: You gain an effect that causes some amount of pain whenever you use your powers that lasts for a random period of time based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Observed
Minimum Attunement: 4  
Chance: 2% to 29%  
Description: You gain an effect that unlocks nastier consequences, comes with nightmares that increase your attunement vitamin and lasts a long time based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Incorporeality
Minimum Attunement: 4  
Chance: 4% to 37%  
Description: When using the ephemeral walk power, you become incorporeal for a brief period based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Teleport Lock
Minimum Attunement: 4  
Chance: 2% to 33.5%  
Description: You are unable to teleport yourself for a brief period based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Metabolic Inversion
Minimum Attunement: 4  
Chance: 1% to 29%  
Description: Only applicable if you have the Biokinetic power path. The effects of the Efficient System trait and Metabolic Hyperefficiency power are inverted for a random length of time based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Power Drain
Minimum Attunement: 4  
Chance: 1% to 29%  
Description: Your electronics slowly lose power and you cannot activate Electron Overflow for a random period of time based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Weakness
Minimum Attunement: 5  
Chance: 2% to 31%  
Description: You have lowered strength and dexterity along with increased stamina usage in melee for a short period based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### KCal Consumption
Minimum Attunement: 5  
Chance: 4% to 38%  
Description: The KCal consumption of your powers is tripled  
<sub>[Back to List](#attunement-consequences)</sub>

### Attenuation
Minimum Attunement: 6  
Chance: 4% to 25.5%  
Description: Your powers all take four times as long to activate for a short period of time based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Short of Breath
Minimum Attunement: 5  
Chance: 3% to 25.5%  
Description: You have trouble breathing, lowering your breathing and lifting scores and cough occasionally for a short period based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Force Wave
Minimum Attunement: 6  
Chance: 3% to 30%  
Description: When using a telekinetic power, a wave of force knocks you and everything around you down  
<sub>[Back to List](#attunement-consequences)</sub>

### Teleport Misjump
Minimum Attunement: 7  
Chance: 3% to 21%  
Description: When using a power that teleports you, you get sent somewhere far away and gain an effect that will summon the hounds of tindalos if you use a teleportation power before it wears off  
<sub>[Back to List](#attunement-consequences)</sub>

### EMP Blast
Minimum Attunement: 8  
Chance: 2% to 18.75%  
Description: Only applicable if you have the Photokinetic power path. You emit an EMP blast centered on yourself  
<sub>[Back to List](#attunement-consequences)</sub>

### Lightning Blast
Minimum Attunement: 9  
Chance: 3% to 19.25%  
Description: You emit a blast of lighting, electrocuting yourself and your surroundings  
<sub>[Back to List](#attunement-consequences)</sub>

### Crack in Reality
Minimum Attunement: 9  
Chance: 2% to 9.5%  
Description: A temporary reality tear opens up nearby  
<sub>[Back to List](#attunement-consequences)</sub>

### Nullified
Minimum Attunement: 7  
Chance: 2% to 18.25%  
Description: All of your active powers are cancelled and you are unable to activate any more powers for a period of time based on your attunement vitamin  
<sub>[Back to List](#attunement-consequences)</sub>

### Hounds of Tindalos
Minimum Attunement: 10  
Chance: 3% to 8%  
Description: This consequence can only activate if you have the Observed or Nether Conduit effect. You summon the Hounds of Tindalos  
<sub>[Back to List](#attunement-consequences)</sub>
 
### Mutation
Minimum Attunement: 12  
Chance: 3% to 6%  
Description: You gain a random negative mental or psychological mutation  
<sub>[Back to List](#attunement-consequences)</sub>

### Reality Tear
Minimum Attunement: 12  
Chance: 2% to 3.5%  
Description: This consequence can only activate if you have the Observed or Nether Conduit effect. A reality tear opens on top of you  
<sub>[Back to List](#attunement-consequences)</sub>
