Hi, thanks for reading. This is a brief overview of the things I (Renech) am looking to do with this mod, and that might be accepted if you PR them before I do.

This list isn't exclusive. Do you have an idea that you'd like to contribute to megacity? You can contact me on the development discord, or just post an issue on Github. The point here is to make sure we don't waste your time!

Now, the things!

-Some better form of placing the cities so they truly do overlap and every tile is filled in. There should be **zero** fields, forests, or non-urban buildings.

-Better handling of the city "commercial district" vs suburbia. I want small pockets of suburbia buttressed by enormous oceans of high-rise and commercial buildings. The reason for this is simple: Suburbia is pretty safe, "low octane". Megacity is about "high octane", and not being very safe. Too many relatively safe and lootable areas makes for a poor experience.
--Currently this is done by some empirically tested values for park/shop radius and sigma. This is not great, and liable to break with changes to vanilla city generation.

-Some form of "stick" that forces the player to constantly migrate to new untouched areas of the city. Being stuck in an endless city is only interesting if you're actually stuck there. If you've killed everything within a mile, and you can sit with your feet up, it's pointless.
--Possibly, disabling agriculture altogether should be part of this. Food pressure is definitely one way.
--Thirst pressure might be more relevant since there are no rivers.
---Note I am NOT looking for some sort of magical or otherworldly explanation. This is not "liminal space", it's just a big, infinite-enough city. Purposefully no explanation should be given.

-Remove any infinite sources of water (to increase the thirst pressure). This includes backyard pools, parks with water tiles, the ability to construct or use a well etc. This does not include just clean water.

-More messy cities, barricades on the streets, etc. The aim is to make vehicular travel purposefully difficult. Vehicles can still be used as improvised weapons or sources of materials. Vehicle "spawn status" should not be overriden, as that just makes less found vehicles to have a working status, totally tangetial to the purpose of making vehicle travel difficult.
--This can probably go straight into vanilla DDA, so PR it there first. 