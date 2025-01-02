The way that batteries, power storage, and electricity use are handled is a pile of compromises, so this document outlines the decisions about these topics and some of the rationales for them.

## Voltage, Current, Capacity
The short of it is, we are not handling voltage and current explicitly, only capacity.
As we are not designing systems but rather representing them, it's reasonable to assume that a particular class of battery is generally fit for purpose.
For example, we assume that battery cells for handheld devices are able to activate those handheld devices, even though in reality some devices require higher current than average batteries will provide.  Once you scale up to e.g. electric vehicle battery packs, we assume that a car-scale battery pack is fit for purpose, even though in reality moving a battery from one electric vehicle to another would likely be an extremely difficult endeavor.
There is some possiblility of making different batteries or other power sources not fit for purpose for certain tasks, but this would be realized via some labelling like "high-draw power source" rather than attempting to model voltage and current limits explicitly.

## Battery representation
Matching batteries to a particular electrical device can be a pain, but is it a fun or interesting kind of pain? I assert not, and as such we are not handling a wide variety of different battery types and sizes that have to be matched to the device, but abstracting battery packs to a near-minimal representative set of size classes to cover all our devices.
Likewise, many electronic devices accept some number of standard sized battery cells, but this isn't a particularly interesting detail, so instead we are abstracting battery compartments to holding a single battery cell of an appropriate capacity and not handling multiple cells per device.
In order to make reasoning about cells a bit easier, the capacities and dimensions of cells correlate with standard battery form factors when appropriate and they have comments indicating this relationship, but the ids, names and descriptions of the items do not because indicating that a particular battery is similar to for exampe an AA cell would be more confusing than informative since they are not used in the same way. Using a small LED flashlight as an example, it is likely to have a battery compartment that accepts three AAA battery cells, but in-game we would not use 3 AAA cells but rather a single cell about the size of a C or D battery. If we called that battery a D cell, it's quite likely that players would ignore it because IRL D cells are relatively niche, while in DDA they would be extremely common.

## Matching batteries to devices
When adding a battery powered device to the game, the most important aspect to get right for representativeness is overall runtime. Get an estimate for how long the device would be able to operate on standard batteries and try to set the power consumption and battery type it accepts to roughly match that runtime.  This is a highly subjective process because device manufacturers are highly incentivised to be misleading when they make claims about battery life, so be skeptical of manufacturer claims, but if they aren't too out there or if it's not that impactful, don't worry about it too much. Generally higher draw and shorter the battery life of a device is an indication to be more careful about accuracy, if it's very low draw and/or the IRL device legitimately has much greater capacity than it needs, tolerances are very loose, but if the device chronically runs out of power quicky IRL it becomes more important to not under- or over- estimate how long it will be able to operate.

Table of currently existing batteries

| ID                          | IRL Counterpart      | Type                 | Energy |
|-----------------------------|----------------------|----------------------|--------|
| light_minus_battery_cell    | Button               | BATTERY_ULTRA_LIGHT  | 2 kJ   |
| light_minus_disposable_cell | Button               | BATTERY_ULTRA_LIGHT  | 2 kJ   |
| light_cell_rechargeable     | AA                   | BATTERY_LIGHT        | 10 kJ  |
| light_battery_cell          | AA                   | BATTERY_LIGHT        | 16 kJ  |
| medium_battery_cell         | 18650                | BATTERY_MEDIUM       | 56 kJ  |
| heavy_battery_cell          | Small makita battery | BATTERY_HEAVY        | 259 kJ |
| heavy_plus_battery_cell     | Big makita battery   | BATTERY_HEAVY        | 503 kJ |

As you can see, each battery level is roughly five times more energy dense than previous one, meaning if your tool require, for example, 5 AA batteries, the game would represent it as having a single medium battery. Edge cases should be resolved in the next way: if irl counterpart require two AA battery, just use a single light battery, and decrease power consumption to match expected work duration; if irl counterpart require three or four AA batteries, use medium battery and increase power consumption to, again, match expected work duration.
Alternatively, you can try to find a counterpart that uses amount of batteries that are easier to match in each particular case.