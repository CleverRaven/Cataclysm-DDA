# Exosuit Guide

# Getting started

The exosuit is multirole powered exoskeleton. Like other exoskeletons, it is worn and activated to provide a bonus to the pilot's strength. 

On it's own, the frame is nothing more than that, a frame. Operation requires a power supply, of which there are several options that can be mounted on the frame. These range from large battery packs to compact ICE generators.

Various types of armor plate can be attached to the frame at the head, torso, arms, and legs. An additional armor plate can be installed on the large power supplies, protecting them from damage.

Modules are equipment that enhance the exosuit in various ways. Some are passive, others draw (or provide) power, some are utility tools, others are powerful combat gear.

All of this is managed in the exosuit menu, by 'a'ctivating the item in your inventory. Two options will appear: 'Manage exosuit modules' or 'Turn on'/'Turn off'.

### Background
### Frame
The frame of the exosuit is a relatively simple design, easy to manufacture.  A titanium frame lined with artificial muscles, studded with connection points allowing quick installation and swapping of plating and modules without specialized equipment. A helmet can be attached to the frame, and the weight is supported by the frame without restricting the pilot's field of view.

### Underlayer
These are comfortable jumpsuits specially designed to integrate with the frame.  Underlayers are not required for operation, but prevent accidental injury as the frame moves.  Specialized undersuits are built for various environments or combat.

### PSU
Exosuits typically use a single large power supply unit, attached to the back.  Auxiliary batteries can be attached as well across the frame, extending the runtime. 

| Name          | Capacity | Fuel      | Loaded Weight | Volume | Notes
| ---           | ---      | ---       | ---           | ---    | ---
| Battery PSU   |   40000  | Battery   | 15 kg         | 20 L   | Standard battery unit
| ICE PSU       |   10000  | Gasoline  | 20 kg         | 25 L   | Only runs when needed
| Multifuel PSU |    5000  | Multiple  | 25 kg         | 25 L   | Burns a variety of liquid fuels
| ASRG PSU      |   10000  | Unlimited | 50 kg         | 25 L   | Provides unlimited power at a slow trickle


### Plating
Each plating has a minimum weight of 500 g to represent the connection points.  The steel and plastic are based on 6mm thickness, while the VCC is 4mm thick to keep weight manageable.  Exosuits can accept a wide range of plating.  Plastic, steel, and vacuum-cast carbide are common.  Improvised armor has been found on frontiers and in the underworld.  The plates are rounded, using a ball-and-joint system somewhat resembling a hard diving or EVA suit.

| Plating              | Weight   
| ---                  | ---
| Plastic              | 11.01 kg
| Tempered Steel       | 44.05 kg
| Vacuum-cast Carbide  | 65.41 kg 
   
### Modules
A variety of modules are available, ranging from sensor packages to batteries to load-bearing modules to jetpacks.  These are built in a swappable black box configuration, requiring only that the module be mounted on the suit's connection points. 

Modules come in Small, Medium, Large, Helmet, and PSU. The helmet modules can only be mounted in the exosuit helmet, and the PSU has a unique slot off of the torso.  Modules cannot exceed the volume or weight of the module class. Small and medium modules can fit into larger module slots.

# Guidelines for module capacity
For reference when creating new modules. It is assumed that all modules are black-box modules, meaning that they only need to have the appropriate connectors to interface with the frame.
| Module Class | Volume | Weight
| ---          | ---    | ---   
| Large        |   10 L |  10 kg
| Medium       |  2.5 L |   5 kg
| Small        |  0.5 L | 0.5 kg
| Helmet       | 0.25 L | 0.5 kg
| PSU          |   25 L | 140 kg

# Slot locations
Each location has limited slots for modules.
| Location | Large | Medium | Small | Helmet | PSU
| ---      | ---   | ---    | ---   | ---    | ---
| Helmet   | ---   | ---    | ---   | 4      | ---
| Torso    | 2     | ---    | 2     | ---    | ---
| Arm      | ---   | 2      | 2     | ---    | ---
| Leg      | ---   | 2      | 2     | ---    | ---
| PSU      | ---   | ---    | ---   | ---    | 1 

# General Modules
General purpose utility modules
| Module         | Class  | Power Draw | Weight | Description
| ---            | ---    | ---        | ---    | ---
| Jetpack        | Large  | ---        |  10 kg | An ammonia-fueled jetpack provides a dash across terrain.
| Aux Battery    | Medium | Battery    |   5 kg | Stores an additional 6000 battery.
| Support        | Medium | 1500       |   5 kg | Adds an additional 30 kg of carry weight while active.
| Air Supply     | Medium | ---        |   2 kg | A regenerating oxygen supply, activates in emergencies.
| Climb Assist   | Medium | ---        |   5 kg | Allows the user to climb walls.
| Forcefield     | Medium | 5000       |   5 kg | Toggles between 25 bash/cut or 40 ballistic/pierce.
| Collapsed Axe  | Medium | ---        | 3500 g | Activate to extend into a boarding axe.
| Small Aux      | Small  | Battery    |  500 g | Stores an additional 2500 battery.
| Small Support  | Small  | 1000       |  500 g | Adds an additional 10 kg of carry weight while active.
| Mounted Lamp   | Small  | 1000/2000  |  500 g | A sturdy lamp with a low and high setting.
| Recoil Control | Small  | 2000       |  500 g | Reduces weapon dispersion while active.
| Med Injector   | Small  | ---        |  500 g | Injects RA21E to treat injuries.
| Zoom Lenses    | Helmet | ---        |  500 g | Corrects vision and provides optical zoom.
| Imager         | Helmet | 50 per use |  500 g | As the handheld tool, sees through walls.
| AR Research    | Helmet | 250        | 500 g  | Adds 15 learning focus while active.

# Exosuit Ranged Weapons
Purpose built ranged weapons
| Ranged Weapon      | Power Source     | Weight | Description
| ---                | ---              | ---    | ---
| Heavy Flamethrower | Flammable Liquid | 7300 g | A large flamethrower with long range.
| Sonic Cannon       | Power Cartridge  | 6000 g | Deals a large amount of bash damage in a cone.
| Microwave Cannon   | Power Cartridge  | 7500 g | Deals a variable amount of pure damage.
| Electrolaser       | Power Cartridge  | 9900 g | A laser followed by a massive energy discharge.

# Exosult Melee Weapons
Purpose built or improvised melee weapons
| Melee Weapon | Power Source | Weight | Description
| ---          | ---          | ---    | ---
| Boarding Axe | ---          | 3500 g | An oversized axe. Collapses into a medium module.
| Scrap Axe    | ---          | 4250 g | An oversized axe. Made from scraps.
| Heavy Drill  | Battery      |  45 kg | A massive drill. Activate to really do some damage.
| Exosuit Claw | ---          |   3 kg | Powerful hydraulic gauntlets tipped with claws.
| Plasma Torch | Plasma       |   8 kg | A large plasma torch with a devastating short range attack.
| Power Cutter | Liquid Fuel  |  10 kg | Cuts through just about anything.

# Exosuit Storage Modules
Assorted storage options
| Storage Module    | Class  | Volume Cap | Weight Cap | Weight | Description
| ---               |---     | ---        | ---        | ---    | ---
| Rifle Storage     | Large  |     9 L    | 10 kg      |  500 g | As item Back Holster.
| Large Ammo Pouch  | Large  |  Multiple  |  Multiple  | 1750 g | As item Tac Vest.
| Storage Pack      | Large  |  Multiple  |  Multiple  | 2250 g | As item Large Tactical Backpack.
| Melee Storage     | Medium | 3750 ml    |  5 kg      |  500 g | As item Back Scabbard.
| Medium Pouch      | Medium | 2500 ml    |  5 kg      |  275 g | As item Fanny Pack.
| Small Ammo Pouch  | Medium |     2 L    |  4 kg      | 1050 g | As item Drop Leg Pouches.
| Small Pouch       | Small  | 1500 ml    |  5 kg      |  125 g | As item Leather Pouch.
| Quickdraw Holster | Medium | 1500 ml    |  5 kg      |  500 g | Assisted draw holster.
| Quickdraw Sheath  | Medium | 1500 ml    |  5 kg      |  500 g | Assisted draw sheath.

### Operation
The basic operation is based on the frame mirroring the pilot's motions.  This works well for movement, though fine manipulation takes practice as pilots must adapt to the suit's feedback.  Once certified, a pilot receives additional training in their field, learning to use their specialized equipment.  The exosuit is easily outfitted for hazardous environments ranging from frozen tundra to burning deserts, from underwater to orbit.

### Repairs
The frame itself is somewhat difficulty to repair due to the complex components and the titanium used.
Plating (aside from carbide) can be field repaired as long as the plate has not been completely destroyed.  Destroyed plates can have their connectors salvaged and used to create new plates with the right equipment.  Modules are complicated electronics packed into very tight containers, making repairs extremely difficult.  Specialized toolkits are issued for each frame allowing field repairs, and powered gantries are available to support the suit for more intensive repairs while recharging.
See Exosuit Repair Kit, Exosuit Maintenance Gantry


# Balance
### Power Consumption
The initial power draw for the frame is based on the Aftershock suit operation time, 45 hours for the frame alone.  Adding plating and modules increases the weight, requiring power hungry support modules for all but the strongest pilots.  With the frame, helmet, battery PSU and carbide plating, the entire suit weighs just over 125 kg, before any additional equipment is added.  For a default character, this means they would need several support modules just to move, and the minimum number would cut the runtime down to 21 hours.  Further modules add additional weight and potentially power draw.

### Inspirations
BattleTech, Beam Saber, Jovian Chronicles, Silent Storm, Fallout, Patlabor.