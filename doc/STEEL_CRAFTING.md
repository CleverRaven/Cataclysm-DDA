# Making and working with steel grades



# Defining Steel Grades

Steel is an extremely versatile resource. In Cataclysm: Dark Days Ahead, we use a very simplified system of steel grades, roughly based on the SAE International grading. In general, a carbon steel is iron with a specific amount of carbon added. While other steel alloys may add additional materials or treat the steel in various ways to produce different results, carbon steels are the workhorses of the modern world.

### Slag

Slag is a byproduct of metalworking, generally representing impurities in the ore used to create the steel. It does have some uses IRL, but is generally an unpleasant toxic substance that is difficult to get rid of.

### Budget Steel

Budget Steel represents poorly made steel that contains impurities, rusts easily, fractures, and generally isn't very useful. This should be used to represent things like a Genuine Katana From Nippon that was sold on craigslist for $50, or to represent things like armor that has been sitting in a mansion for two hundred years without maintenance. 

### Mild Carbon Steel

Approximates SAE 1010, a 0.1% carbon steel with no slag content. Mild (or low) carbon steel is the simplest version of steel that a player can craft. Despite the name, it is a tough substance with many uses. Due to the relative ease of production, many weapons and armors can use mild steel. This is also the easiest version to work with.

### Medium Carbon Steel

Approximates SAE 1050, a 0.5% carbon steel with no slag. This is commonly used in industrial applications, large machinery, automobile parts, and softer metalworking tools.

### High Carbon Steel

Approximates SAE 1085, a 0.85% carbon steel with no slag. This is a particularly hard steel, often used in metalworking and high strength applications like bladed tools, heavy-duty wires and cables. Higher carbon content than this becomes extremely difficult to work outside of a survivor's garage workshop. High carbon steel is the most difficult of the mild/medium/high steels to work as it is extremely hard.

### Case Hardened Carbon Steel

Case hardening (also known as face hardening) is an old process used to improve mild steels. The steel is sealed in a container which is packed tightly with high carbon substances (ground charcoal or coal is ideal). It is then heated to ~900°C for several hours, releasing carbon monoxide which binds with the steel which is in a transitioning phase at that temperature. The process forms a higher carbon case around the softer steel core, allowing for a hard surface that can hold an edge well with a softer center to prevent fracturing.

### Quench Tempered Carbon Steel

Tempering (specifically, Martensitic transformation) is a process of hardening the steel by heating it to just under ~900°C and then rapidly cooled in water. This results in an extremely hard crystalline structure forming within the steel, making this one of the toughest substance a survivor can create.

# Making Steel

### Tools

Aside from standard tools for working and shaping metal, steel requires not only high heat, but carefully controlled heat, maintained for hours in some cases. This partially accounts for the long craft times of steel items. Gas or electric forges are ideal for a survivor.

A large amount of water is used to rapidly quench hot steel. IRL this could also be done with smaller quantities of steel but that requires refreshing with cold water.

Because it has come up in discussion before: oils are also used for quenching, but this is done deliberately for a slightly different result of a softer steel. 

### Resources

While surface iron deposits are effectively exhausted by strip mining, there is a convenient source of metal to work with: the American automobile. Many other sources of steel exist, and reusing existing steel is a much better use of a survivor's time than digging in the dirt.

At the time of this writing, most items are the old 'steel', but over time these will be converted to appropriate grades. Meanwhile, recipes exist to convert 'steel' to mild carbon steel.

Not surprisingly, *carbon* is a critical component of carbon steels, but is readily available as long as one has trees to convert to charcoal. 

### Books

Studies in Historic Armorsmithing: Useful as a reference for armor, but also for metalworking techniques used at small scale.

Machinery's Handbook: Advanced metalworking techniques for harder steels.

Welding and Metallurgy: Assembly and repairs, as well as crafting hardened/tempered steels.

# Scrap to swords

### Skills

Making steel is an old practice, in place for several hundred years. Recently, it has become extremely industrialized outside of custom tool shops and hobbyist professions. This means that a great deal of information is available about the mass production of steel by the ton, but less about making small batches to produce a single sword. The minimum skill for a mild steel item should be at least 5 points of fabrication, increasing larger, more complicated crafts. Hardening and tempering should also increase the skill requirements.

| Thickness                              | Skill requirement |
| -------------------------------------- | ----------------- |
| Mild, medium, or high steel up to 2 mm |                 5 |
| Up to 4 mm                             |                +1 |
| Up to 6 mm                             |                +1 |
| Any thickness, hardened                |                +1 |
| Any thickness, tempered                |                +2 |

| Item       | Thickness   | Steel Grade | Skill | Factor                                            |
| ---------- | ----------- | ----------- | ----- | ------------------------------------------------- |
| Chestplate |    2 mm     | Mild        |     5 |                                                   |
| Chestplate |    4 mm     | Medium      |     6 | Increases due to thicker plate                    |
| Chestplate |    6 mm     | Tempered    |     9 | Increases due to both thicker plate and tempering |

### Time

One of the longest parts of any metalworking project is the heating and cooling. Even heating prevents warping of metal and ensures consistency in the item's durability. Cooling in particular must be carefully managed or it can result in brittle material that cracks and shatters. For some crafts, the metal must be allowed to cool and be reheated, especially with large or complicated projects. **A large number of steel crafts in Cataclysm are an order of magnitude too quick.** This sets unrealistic expectations among players and can only be corrected by updating more and more recipes to use longer craft times, which should be incorporated while creating graded steel versions.

### Upgrades

Mild steel can be 'raised' to medium steel, and medium to high steel. These recipes exist in the game for lumps and chunks. Mild steel can be hardened and medium steel can be tempered, but neither of these would be done to an ingot of metal that would then later be worked. Instead a hardened or tempered item should be crafted from scratch or by using a mild/medium steel item and upgrading it to a higher type of steel.

### Case Hardening Requirements

Case hardening does not require a significant increase in material consumption as only a small amount of coal/charcoal is needed to pack each item, so the costs of steel remains the same as a mild/medium/high steel version and a small amount of the material requirement "carbon" should be used: 5 carbon should be sufficient for a complete front and back torso plate. The case hardening process is longer, crafting time should increase by a minimum of 12 hours to represent packing the item in carbon, heating, 'cooking' and cooling. Multiple items can be hardened at the same time, so if you're making something like a steel gauntlet with lots of small parts, each can be separately packed and then all heated together. Consider additional crafting time for the disassembly of complex items. The fuel costs for the craft should increase to represent keeping the forge hot for a long period of time, but that is difficult to model with the variety of forges we have. This also does not consume materials to represent a container for the item as those can typically be made from miscellaneous scraps of steel and can often be reused but vary in size.

### Tempering Requirements

Similar to hardening, the material costs do not dramatically increase. The biggest increase is in crafting time as each item is repeatedly heated and quenched. This should add a minimum of 16 hours over a mild/medium/high steel version. The fuel costs should increase to represent the repeated heating, but that is difficult to model with the variety of forges we have. 


### Repairing

Most metal repairs in Cataclysm simply use a welder with the associated fuel costs and an amount of a similar material, representing cutting and replacing or layering new material over old. While this is highly abstracted, we don't have a more detailed repair model. For mild, medium, and high steel, this is sufficient. Case hardened steel should properly be re-cased after a repair, which is difficult to manage IRL and often it would be easier to reforge the item. Tempered steel can be welded, but is a much more complicated process: the metal must be heated to the temperature it was tempered at, the new material added, then the entire area is wrapped in insulation for equal cooling. To this effect, an insulated welding blanket has been added, allowing the wrapped metal to evenly cool. However, the repair time for this process is greatly condensed as the cooling process alone can take hours.

